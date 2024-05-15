package rss

type Feed struct {
	Channel Channel `xml:"channel"`
}

type Channel struct {
	Title string `xml:"title"`
	Copyright string `xml:"copyright"`
	Hyperlink string `xml:"link"`
	Published string `xml:"pubDate"`
	Updated string `xml:"lastBuildDate"`
	Items []Item `xml:"item"`
}

type Item struct {
	Title string `xml:"title"`
	Hyperlink string `xml:"link"`
	Published string `xml:"pubDate"`
}
