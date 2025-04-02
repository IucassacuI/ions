package main

import (
	"crypto/sha256"
	"fmt"
	"github.com/IucassacuI/feeds"
	"io"
	"net"
	"net/http"
	"os"
	"strconv"
	"strings"
)

const (
	INVALID_FEED = 1 + iota
	CONNECTION_FAILED
	READ_FAILED
	DIRECTORY_ERR
	UNMARSHALING_FAILED
	REMOFEED_FAILED
	SOCK_LISTEN_FAILED
	SOCK_ACCEPT_FAILED
)

func createlibrary() {
	if err := os.Chdir("library"); err != nil {
		os.Mkdir("library", os.ModePerm)
	}

	var dir string
	dir, err := os.Getwd()

	if err != nil {
		os.Exit(DIRECTORY_ERR)
	} else if !strings.Contains(dir, "library/") {
		os.Chdir("library")
	}
}

func calchash(url string) string {
	var hashbytes [32]byte = sha256.Sum256([]byte(url))
	var hash string = fmt.Sprintf("%x", hashbytes)

	return hash
}

func checklibrary(feedurl string) bool {
	var hash string = calchash(feedurl)

	_, err := os.ReadFile(hash)
	return err == nil
}

func readfromlibrary(feedurl string) []byte {
	var hash string = calchash(feedurl)

	content, _ := os.ReadFile(hash)
	return content
}

func updatelibrary(conn net.Conn, feed, words string) {
	var hash string = calchash(feed)

	resp, err := http.Get(feed)
	if err != nil {
		conn.Write([]byte("ERROR " + strconv.Itoa(CONNECTION_FAILED) + "\n"))
		return
	}

	if resp.StatusCode != 200 {
		conn.Write([]byte("ERROR " + strconv.Itoa(resp.StatusCode) + "\n"))
		return
	}

	defer resp.Body.Close()
	body, err := io.ReadAll(resp.Body)

	if err != nil {
		conn.Write([]byte("ERROR " + strconv.Itoa(READ_FAILED) + "\n"))
		return
	}

	if words != "" {
		body = filterfeed(conn, body, words)
	}

	if body == nil {
		conn.Write([]byte("ERROR" + strconv.Itoa(INVALID_FEED) + "\n"))
		return
	}

	conn.Write([]byte("OK\n"))

	if checklibrary(feed) {
		onlibrary, _ := os.ReadFile(hash)
		status := fmt.Sprintf("%v\n", len(body) != len(onlibrary))
		conn.Write([]byte(status))
	}

	os.WriteFile(hash, body, os.ModePerm)
}

func showmetadata(conn net.Conn, feed string) {
	if !checklibrary(feed) {
		conn.Write([]byte("ERROR " + strconv.Itoa(INVALID_FEED) + "\n"))
		return
	}

	data := readfromlibrary(feed)
	doc := feeds.Parse(data)

	if doc.Title == "" {
		conn.Write([]byte("ERROR " + strconv.Itoa(INVALID_FEED) + "\n"))
		return
	}

	conn.Write([]byte("OK\n"))
	conn.Write([]byte(doc.Title + "\n"))
	conn.Write([]byte(doc.Author + "\n"))
	conn.Write([]byte(doc.Hyperlink + "\n"))
	conn.Write([]byte(doc.Published + "\n"))
	conn.Write([]byte(doc.Updated + "\n"))
}

func showitem(conn net.Conn, feed string, number int) {
	if !checklibrary(feed) {
		conn.Write([]byte("ERROR " + strconv.Itoa(INVALID_FEED) + "\n"))
		return
	}

	data := readfromlibrary(feed)
	doc := feeds.Parse(data)

	if doc.Title == "" {
		conn.Write([]byte("ERROR " + strconv.Itoa(INVALID_FEED) + "\n"))
		return
	}

	item := doc.Items[number]

	conn.Write([]byte("OK\n"))
	conn.Write([]byte(item.Title + "\n"))
	conn.Write([]byte(item.Published + "\n"))
	conn.Write([]byte(item.Updated + "\n"))
	conn.Write([]byte(item.Hyperlink + "\n"))
}

func showitems(conn net.Conn, feed string) {
	if !checklibrary(feed) {
		conn.Write([]byte("ERROR " + strconv.Itoa(INVALID_FEED) + "\n"))
		return
	}

	data := readfromlibrary(feed)
	doc := feeds.Parse(data)

	if doc.Title == "" {
		conn.Write([]byte("ERROR " + strconv.Itoa(INVALID_FEED) + "\n"))
		return
	}

	conn.Write([]byte("OK\n"))

	for _, item := range doc.Items {
		conn.Write([]byte(item.Title + "\n"))
	}
}

func removefeed(conn net.Conn, feed string) {
	var hash string = calchash(feed)

	err := os.Remove(hash)

	if err == nil {
		conn.Write([]byte("OK\n"))
	} else {
		conn.Write([]byte("ERROR " + strconv.Itoa(REMOFEED_FAILED) + "\n"))
	}
}

func filterfeed(conn net.Conn, feed []byte, filter string) []byte {
	var parsed feeds.Feed = feeds.Parse(feed)

	if parsed.Title == "" {
		conn.Write([]byte("ERROR" + strconv.Itoa(UNMARSHALING_FAILED) + "\n"))
		return nil
	}

	var words []string = strings.Split(filter, ",")
	var filtered []feeds.Item

	for _, item := range parsed.Items {
		title := strings.ToLower(item.Title)

		for _, word := range words {
			w := strings.ToLower(word)

			if strings.Contains(title, " "+w) || strings.Contains(title, w+" ") {
				goto outer
			}
		}

		filtered = append(filtered, item)

	outer:
	}

	parsed.Items = filtered
	return feeds.Marshal(parsed)
}

func main() {

	socket, err := net.Listen("unix", "./sock")
	if err != nil {
		os.Exit(SOCK_LISTEN_FAILED)
	}

	createlibrary()

	for {
		conn, err := socket.Accept()
		if err != nil {
			os.Exit(SOCK_ACCEPT_FAILED)
		}

		buffer := make([]byte, 4096)

		total_bytes, err := conn.Read(buffer)
		if err != nil {
			os.Exit(READ_FAILED)
		}

		line := strings.Split(string(buffer[:total_bytes]), " ")

		if line[0] == "QUIT" {
			socket.Close()
			break
		}

		switch line[0] {
		case "UPDATE":
			if len(line) == 3 {
				updatelibrary(conn, line[1], line[2])
			} else {
				updatelibrary(conn, line[1], "")
			}
		case "REMOVE":
			removefeed(conn, line[1])
		case "METADATA":
			showmetadata(conn, line[1])
		case "ITEMS":
			showitems(conn, line[1])
		case "ITEM":
			number, err := strconv.Atoi(line[2])
			if err != nil {
				number = 0
			}

			showitem(conn, line[1], number)
		}

		conn.Close()
	}

	os.Remove("../sock")
}
