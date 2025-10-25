package themes

import (
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"sync"
)

type Scheme struct {
	ID          string
	DisplayName string
	Variant     string
	Data        string
}

type palette struct {
	Background   string
	Foreground   string
	Selection    string
	CurrentLine  string
	LineNumber   string
	Comment      string
	String       string
	Keyword      string
	Function     string
	Type         string
	Constant     string
	Number       string
	Operator     string
	Builtin      string
	Preprocessor string
}

var (
	installOnce sync.Once
	installDir  string
	installErr  error
)

var schemes = map[string]Scheme{}

func init() {
	register("catppuccin-mocha", "Catppuccin Mocha", "dark", palette{
		Background:   "#1e1e2e",
		Foreground:   "#cdd6f4",
		Selection:    "#45475a",
		CurrentLine:  "#313244",
		LineNumber:   "#7f849c",
		Comment:      "#6c7086",
		String:       "#a6e3a1",
		Keyword:      "#cba6f7",
		Function:     "#89b4fa",
		Type:         "#fab387",
		Constant:     "#f38ba8",
		Number:       "#fab387",
		Operator:     "#89b4fa",
		Builtin:      "#94e2d5",
		Preprocessor: "#f38ba8",
	})
	register("catppuccin-latte", "Catppuccin Latte", "light", palette{
		Background:   "#eff1f5",
		Foreground:   "#4c4f69",
		Selection:    "#ccd0da",
		CurrentLine:  "#dce0e8",
		LineNumber:   "#8c8fa1",
		Comment:      "#9ca0b0",
		String:       "#40a02b",
		Keyword:      "#8839ef",
		Function:     "#1e66f5",
		Type:         "#df8e1d",
		Constant:     "#d20f39",
		Number:       "#fe640b",
		Operator:     "#1e66f5",
		Builtin:      "#179299",
		Preprocessor: "#d20f39",
	})
	register("everblush", "Everblush", "dark", palette{
		Background:   "#141b1e",
		Foreground:   "#dadada",
		Selection:    "#1f2a30",
		CurrentLine:  "#1c252c",
		LineNumber:   "#59626f",
		Comment:      "#59626f",
		String:       "#9ccfd8",
		Keyword:      "#c47fd5",
		Function:     "#d6d6d6",
		Type:         "#f2b482",
		Constant:     "#ef8891",
		Number:       "#f2b482",
		Operator:     "#91d7e3",
		Builtin:      "#a3be8c",
		Preprocessor: "#ef8891",
	})
	register("gruvbox-dark", "Gruvbox Dark", "dark", palette{
		Background:   "#282828",
		Foreground:   "#ebdbb2",
		Selection:    "#3c3836",
		CurrentLine:  "#32302f",
		LineNumber:   "#928374",
		Comment:      "#928374",
		String:       "#b8bb26",
		Keyword:      "#fb4934",
		Function:     "#83a598",
		Type:         "#fabd2f",
		Constant:     "#d3869b",
		Number:       "#d79921",
		Operator:     "#fe8019",
		Builtin:      "#8ec07c",
		Preprocessor: "#fb4934",
	})
	register("gruvbox-light", "Gruvbox Light", "light", palette{
		Background:   "#fbf1c7",
		Foreground:   "#3c3836",
		Selection:    "#ebdbb2",
		CurrentLine:  "#f2e5bc",
		LineNumber:   "#928374",
		Comment:      "#928374",
		String:       "#79740e",
		Keyword:      "#9d0006",
		Function:     "#076678",
		Type:         "#b57614",
		Constant:     "#8f3f71",
		Number:       "#8f3f71",
		Operator:     "#af3a03",
		Builtin:      "#427b58",
		Preprocessor: "#9d0006",
	})
	register("solarized-dark", "Solarized Dark", "dark", palette{
		Background:   "#002b36",
		Foreground:   "#839496",
		Selection:    "#073642",
		CurrentLine:  "#073642",
		LineNumber:   "#586e75",
		Comment:      "#586e75",
		String:       "#2aa198",
		Keyword:      "#859900",
		Function:     "#268bd2",
		Type:         "#b58900",
		Constant:     "#d33682",
		Number:       "#d33682",
		Operator:     "#6c71c4",
		Builtin:      "#cb4b16",
		Preprocessor: "#cb4b16",
	})
	register("solarized-light", "Solarized Light", "light", palette{
		Background:   "#fdf6e3",
		Foreground:   "#657b83",
		Selection:    "#eee8d5",
		CurrentLine:  "#eee8d5",
		LineNumber:   "#93a1a1",
		Comment:      "#93a1a1",
		String:       "#2aa198",
		Keyword:      "#859900",
		Function:     "#268bd2",
		Type:         "#b58900",
		Constant:     "#d33682",
		Number:       "#d33682",
		Operator:     "#6c71c4",
		Builtin:      "#cb4b16",
		Preprocessor: "#cb4b16",
	})
	register("cyberdream", "Cyberdream", "dark", palette{
		Background:   "#0c0f1a",
		Foreground:   "#e0f1ff",
		Selection:    "#1b1f3b",
		CurrentLine:  "#161a2e",
		LineNumber:   "#5a6d8a",
		Comment:      "#5a6d8a",
		String:       "#7fffd4",
		Keyword:      "#ff6ac1",
		Function:     "#7aa2f7",
		Type:         "#ffd166",
		Constant:     "#ff8c42",
		Number:       "#ffd166",
		Operator:     "#7aa2f7",
		Builtin:      "#70d6ff",
		Preprocessor: "#ff6ac1",
	})
	register("nord-dark", "Nord Dark", "dark", palette{
		Background:   "#2e3440",
		Foreground:   "#d8dee9",
		Selection:    "#3b4252",
		CurrentLine:  "#353c4a",
		LineNumber:   "#81a1c1",
		Comment:      "#616e88",
		String:       "#a3be8c",
		Keyword:      "#81a1c1",
		Function:     "#88c0d0",
		Type:         "#8fbcbb",
		Constant:     "#bf616a",
		Number:       "#d08770",
		Operator:     "#81a1c1",
		Builtin:      "#5e81ac",
		Preprocessor: "#bf616a",
	})
	register("dracula", "Dracula", "dark", palette{
		Background:   "#282a36",
		Foreground:   "#f8f8f2",
		Selection:    "#44475a",
		CurrentLine:  "#383a59",
		LineNumber:   "#6272a4",
		Comment:      "#6272a4",
		String:       "#f1fa8c",
		Keyword:      "#ff79c6",
		Function:     "#50fa7b",
		Type:         "#8be9fd",
		Constant:     "#bd93f9",
		Number:       "#bd93f9",
		Operator:     "#ffb86c",
		Builtin:      "#ff5555",
		Preprocessor: "#ff79c6",
	})
	register("monokai", "Monokai", "dark", palette{
		Background:   "#272822",
		Foreground:   "#f8f8f2",
		Selection:    "#49483e",
		CurrentLine:  "#3e3d32",
		LineNumber:   "#75715e",
		Comment:      "#75715e",
		String:       "#e6db74",
		Keyword:      "#f92672",
		Function:     "#a6e22e",
		Type:         "#66d9ef",
		Constant:     "#ae81ff",
		Number:       "#fd971f",
		Operator:     "#f92672",
		Builtin:      "#fd971f",
		Preprocessor: "#f92672",
	})
	register("tokyo-night", "Tokyo Night", "dark", palette{
		Background:   "#1a1b26",
		Foreground:   "#c0caf5",
		Selection:    "#283457",
		CurrentLine:  "#24283b",
		LineNumber:   "#565f89",
		Comment:      "#565f89",
		String:       "#9ece6a",
		Keyword:      "#bb9af7",
		Function:     "#7aa2f7",
		Type:         "#2ac3de",
		Constant:     "#ff9e64",
		Number:       "#ff9e64",
		Operator:     "#f7768e",
		Builtin:      "#7dcfff",
		Preprocessor: "#f7768e",
	})
	register("ayu-dark", "Ayu Dark", "dark", palette{
		Background:   "#0f1419",
		Foreground:   "#e6e1cf",
		Selection:    "#20303a",
		CurrentLine:  "#131721",
		LineNumber:   "#5c6773",
		Comment:      "#5c6773",
		String:       "#b8cc52",
		Keyword:      "#ff8f40",
		Function:     "#5ccfe6",
		Type:         "#ffd580",
		Constant:     "#d95757",
		Number:       "#ff6a00",
		Operator:     "#ffb454",
		Builtin:      "#95e6cb",
		Preprocessor: "#d95757",
	})
	register("ayu-light", "Ayu Light", "light", palette{
		Background:   "#fafafa",
		Foreground:   "#5c6773",
		Selection:    "#d1dce5",
		CurrentLine:  "#e6eef3",
		LineNumber:   "#8a9aab",
		Comment:      "#abb0b6",
		String:       "#86b300",
		Keyword:      "#f29718",
		Function:     "#41a6d9",
		Type:         "#f29718",
		Constant:     "#d96c75",
		Number:       "#d96c75",
		Operator:     "#f29718",
		Builtin:      "#4dbf99",
		Preprocessor: "#d96c75",
	})
	register("one-dark", "One Dark", "dark", palette{
		Background:   "#282c34",
		Foreground:   "#abb2bf",
		Selection:    "#3e4451",
		CurrentLine:  "#2c313c",
		LineNumber:   "#636d83",
		Comment:      "#5c6370",
		String:       "#98c379",
		Keyword:      "#c678dd",
		Function:     "#61afef",
		Type:         "#e5c07b",
		Constant:     "#d19a66",
		Number:       "#d19a66",
		Operator:     "#56b6c2",
		Builtin:      "#e06c75",
		Preprocessor: "#c678dd",
	})
	register("one-light", "One Light", "light", palette{
		Background:   "#fafafa",
		Foreground:   "#383a42",
		Selection:    "#d0d0d0",
		CurrentLine:  "#e5e5e6",
		LineNumber:   "#a0a1a7",
		Comment:      "#a0a1a7",
		String:       "#50a14f",
		Keyword:      "#a626a4",
		Function:     "#4078f2",
		Type:         "#c18401",
		Constant:     "#986801",
		Number:       "#b751b6",
		Operator:     "#0184bc",
		Builtin:      "#ca1243",
		Preprocessor: "#a626a4",
	})
	register("kanagawa", "Kanagawa", "dark", palette{
		Background:   "#1f1f28",
		Foreground:   "#dcd7ba",
		Selection:    "#223249",
		CurrentLine:  "#2a2a37",
		LineNumber:   "#54546d",
		Comment:      "#727169",
		String:       "#98bb6c",
		Keyword:      "#957fb8",
		Function:     "#7e9cd8",
		Type:         "#ffa066",
		Constant:     "#e46876",
		Number:       "#dca561",
		Operator:     "#7fb4ca",
		Builtin:      "#ffa066",
		Preprocessor: "#e46876",
	})
}

func register(id, name, variant string, p palette) {
	schemes[id] = Scheme{ID: id, DisplayName: name, Variant: variant, Data: buildXML(id, name, variant, p)}
}

func buildXML(id, name, variant string, p palette) string {
	title := variant
	if len(title) > 0 {
		title = strings.ToUpper(title[:1]) + title[1:]
	}
	return fmt.Sprintf(`<?xml version="1.0" encoding="UTF-8"?>
<style-scheme id="%s" _name="%s" version="1.0">
  <author>Colony</author>
  <_description>%s theme for the Colony editor.</_description>
  <style name="text" foreground="%s" background="%s" />
  <style name="selection" background="%s" />
  <style name="current-line" background="%s" />
  <style name="line-numbers" foreground="%s" background="%s" />
  <style name="comment" foreground="%s" italic="true" />
  <style name="string" foreground="%s" />
  <style name="keyword" foreground="%s" bold="true" />
  <style name="function" foreground="%s" />
  <style name="type" foreground="%s" />
  <style name="constant" foreground="%s" />
  <style name="number" foreground="%s" />
  <style name="operator" foreground="%s" />
  <style name="builtin" foreground="%s" />
  <style name="preprocessor" foreground="%s" />
  <style name="def:comment" foreground="%s" italic="true" />
  <style name="def:string" foreground="%s" />
  <style name="def:keyword" foreground="%s" bold="true" />
  <style name="def:type" foreground="%s" />
  <style name="def:constant" foreground="%s" />
  <style name="def:function" foreground="%s" />
  <style name="matching-bracket" foreground="%s" background="%s" weight="bold" />
  <style name="background" background="%s" />
</style-scheme>
`, id, name, title, p.Foreground, p.Background, p.Selection, p.CurrentLine, p.LineNumber, p.Background, p.Comment, p.String, p.Keyword, p.Function, p.Type, p.Constant, p.Number, p.Operator, p.Builtin, p.Preprocessor, p.Comment, p.String, p.Keyword, p.Type, p.Constant, p.Function, p.Operator, p.Selection, p.Background)
}

func Install() (string, error) {
	installOnce.Do(func() {
		dir := filepath.Join(os.TempDir(), "colony-themes")
		if err := os.MkdirAll(dir, 0o755); err != nil {
			installErr = err
			return
		}
		for id, scheme := range schemes {
			target := filepath.Join(dir, id+".xml")
			if err := os.WriteFile(target, []byte(scheme.Data), 0o644); err != nil {
				installErr = err
				return
			}
		}
		installDir = dir
	})
	return installDir, installErr
}

func AllSchemes() []Scheme {
	list := make([]Scheme, 0, len(schemes))
	for _, scheme := range schemes {
		list = append(list, scheme)
	}
	sort.Slice(list, func(i, j int) bool {
		if list[i].Variant == list[j].Variant {
			return strings.ToLower(list[i].DisplayName) < strings.ToLower(list[j].DisplayName)
		}
		return strings.ToLower(list[i].Variant) < strings.ToLower(list[j].Variant)
	})
	return list
}

func Get(id string) (Scheme, bool) {
	scheme, ok := schemes[id]
	return scheme, ok
}
