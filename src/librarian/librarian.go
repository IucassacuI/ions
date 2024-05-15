package main

import (
	"crypto/sha256"
	"encoding/xml"
	"flag"
	"fmt"
	"io"
	"librarian/atom"
	"librarian/rss"
	"net/http"
	"os"
	"strings"
)

const (
	INVALID_FEED = 1 + iota
	CONNECTION_FAILED
	READ_FAILED
	DIRECTORY_ERR
	UNMARSHALING_FAILED
	REMOFEED_FAILED
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

func updatelibrary(feed, words string) {
	var hash string = calchash(feed)

	resp, err := http.Get(feed)
	if err != nil {
		os.Exit(CONNECTION_FAILED)
	}

	if resp.StatusCode != 200 {
		os.Exit(resp.StatusCode)
	}

	defer resp.Body.Close()
	body, err := io.ReadAll(resp.Body)

	if err != nil {
		os.Exit(READ_FAILED)
	}

	if words != "" {
		body = filterfeed(body, words)
	}

	if checklibrary(feed) {
		onlibrary, _ := os.ReadFile(hash)
		os.Chdir("..")

		var out, _ = os.OpenFile("out", os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0666)

		fmt.Fprintf(out, "%v", len(body) != len(onlibrary))
		out.Close()

		os.Chdir("library")
	}

	os.WriteFile(hash, body, os.ModePerm)
}

func showinfoatom(feed []byte, metadata bool, item int, items bool) {
	var parsed atom.Feed
	err := xml.Unmarshal(feed, &parsed)

	if err != nil {
		os.Exit(UNMARSHALING_FAILED)
	}

	for _, elm := range []*string{&parsed.Title, &parsed.Author.Name, &parsed.Author.URI, &parsed.Published, &parsed.Updated} {
		if *elm == "" {
			*elm = "N/A"
		}
	}

	var out, _ = os.OpenFile("out", os.O_CREATE|os.O_APPEND|os.O_WRONLY, 0666)

	if metadata {
		fmt.Fprintln(out, parsed.Title)
		fmt.Fprintln(out, parsed.Author.Name)
		fmt.Fprintln(out, parsed.Author.URI)
		fmt.Fprintln(out, parsed.Published)
		fmt.Fprintln(out, parsed.Updated)
	} else if item != -1 {
		entry := parsed.Entries[item]

		for _, elm := range []string{entry.Title, entry.Hyperlink.Href, entry.Published, entry.Updated} {
			if elm == "" {
				fmt.Fprintln(out, "N/A")
			} else {
				fmt.Fprintln(out, elm)
			}
		}

	} else if items {
		for _, elm := range parsed.Entries {
			fmt.Fprintln(out, elm.Title)
		}
	}

	out.Close()
}

func showinforss(data []byte, metadata bool, item int, items bool) {
	var parsed rss.Feed
	err := xml.Unmarshal(data, &parsed)

	if err != nil {
		os.Exit(UNMARSHALING_FAILED)
	}

	var feed rss.Channel = parsed.Channel
	for _, elm := range []*string{&feed.Title, &feed.Copyright, &feed.Hyperlink, &feed.Published, &feed.Updated} {
		if *elm == "" {
			*elm = "N/A"
		}
	}

	var out, _ = os.OpenFile("out", os.O_CREATE|os.O_APPEND|os.O_WRONLY, 0666)

	if metadata {
		fmt.Fprintln(out, feed.Title)
		fmt.Fprintln(out, feed.Copyright)
		fmt.Fprintln(out, feed.Hyperlink)
		fmt.Fprintln(out, feed.Published)
		fmt.Fprintln(out, feed.Updated)
	} else if item != -1 {
		item := feed.Items[item]

		for _, elm := range []string{item.Title, item.Hyperlink, item.Published} {
			if elm == "" {
				fmt.Fprintln(out, "N/A")
			} else {
				fmt.Fprintln(out, elm)
			}
		}
		fmt.Fprintln(out, "N/A")
	} else if items {
		for _, elm := range feed.Items {
			fmt.Fprintln(out, elm.Title)
		}
	}

	out.Close()
}

func removefeed(feed string) {
	var hash string = calchash(feed)

	err := os.Remove(hash)

	if err != nil {
		os.Exit(REMOFEED_FAILED)
	}
}

func filterrss(feed []byte, filter string) []byte {
	var doc rss.Feed
	err := xml.Unmarshal(feed, &doc)

	if err != nil {
		os.Exit(UNMARSHALING_FAILED)
	}

	words := strings.Split(filter, ",")

	var filtered []rss.Item

	for _, item := range doc.Channel.Items {
		for _, word := range words {
			w := strings.TrimSpace(word)
			title := strings.TrimSpace(item.Title)

			if strings.Contains(title, w) {
				goto cont1
			}

		}

		filtered = append(filtered, item)

	cont1:
		continue
	}

	doc.Channel.Items = filtered

	mDoc, err := xml.Marshal(doc)
	if err != nil {
		os.Exit(UNMARSHALING_FAILED)
	}

	mDoc = []byte(strings.Replace(string(mDoc), "Feed>", "rss>", 2))
	return mDoc
}

func filteratom(feed []byte, filter string) []byte {
	var doc atom.Feed
	err := xml.Unmarshal(feed, &doc)

	if err != nil {
		os.Exit(UNMARSHALING_FAILED)
	}

	words := strings.Split(filter, ",")

	var filtered []atom.Entry

	for _, entry := range doc.Entries {
		for _, word := range words {
			title := strings.TrimSpace(entry.Title)
			w := strings.TrimSpace(word)

			if strings.Contains(title, w) {
				goto cont2
			}

		}

		filtered = append(filtered, entry)

	cont2:
		continue
	}

	mDoc, err := xml.Marshal(doc)
	if err != nil {
		os.Exit(UNMARSHALING_FAILED)
	}

	mDoc = []byte(strings.Replace(string(mDoc), "Feed>", "feed>", 2))
	return mDoc
}

func filterfeed(feed []byte, words string) []byte {
	if strings.Contains(string(feed), "<rss") {
		return filterrss(feed, words)
	} else if strings.Contains(string(feed), "<feed") {
		return filteratom(feed, words)
	} else {
		os.Exit(INVALID_FEED)
	}

	return []byte{}
}

func main() {
	var feed = flag.String("feed", "", "")
	var update = flag.Bool("update", false, "")
	var metadata = flag.Bool("metadata", false, "")
	var item = flag.Int("item", -1, "")
	var items = flag.Bool("items", false, "")
	var remove = flag.Bool("remove", false, "")
	var filter = flag.String("filter", "", "")

	_, err := os.Stat("out")
	if err == nil {
		os.Remove("out")
	}

	flag.Parse()

	if *feed == "" {
		os.Exit(INVALID_FEED)
	}

	createlibrary()

	if *remove {
		removefeed(*feed)
		os.Exit(0)
	} else if *update {
		updatelibrary(*feed, *filter)
	}

	if !checklibrary(*feed) {
		os.Exit(INVALID_FEED)
	}

	var data = readfromlibrary(*feed)
	os.Chdir("..")

	if strings.Contains(string(data), "<feed") {
		showinfoatom(data, *metadata, *item, *items)
	} else if strings.Contains(string(data), "<rss") {
		showinforss(data, *metadata, *item, *items)
	} else {
		os.Exit(INVALID_FEED)
	}
}
