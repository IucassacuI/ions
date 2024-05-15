package atom

type Feed struct {
	Title string `xml:"title"`
	Author Author `xml:"author"`
	Published string `xml:"published"`
	Updated string `xml:"updated"`
	Entries []Entry `xml:"entry"`
}

type Entry struct {
	Title string `xml:"title"`
	Hyperlink Hyperlink `xml:"link"`
	Published string `xml:"published"`
	Updated string `xml:"updated"`
}

type Hyperlink struct {
	Href string `xml:"href,attr"`
}

type Author struct {
	Name string `xml:"name"`
	URI string `xml:"uri"`
}
