package syntax

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"github.com/example/colony/internal/config"
	"github.com/example/colony/themes"
	"github.com/mattn/go-gtk/gtk"
	"github.com/mattn/go-gtk/gtksourceview"
	"github.com/mattn/go-gtk/pango"
)

type Editor struct {
	Buffer      *gtksourceview.SourceBuffer
	View        *gtksourceview.SourceView
	scroller    *gtk.ScrolledWindow
	languageMgr *gtksourceview.SourceLanguageManager
	schemeMgr   *gtksourceview.SourceStyleSchemeManager
	currentFile string
	currentLang string
	font        config.FontConfig
}

func NewEditor(font config.FontConfig) (*Editor, error) {
	buffer := gtksourceview.NewSourceBuffer()
	buffer.SetHighlightSyntax(true)
	buffer.SetHighlightMatchingBrackets(true)

	view := gtksourceview.NewSourceViewWithBuffer(buffer)
	view.SetShowLineNumbers(true)
	view.SetAutoIndent(true)
	view.SetInsertSpacesInsteadOfTabs(true)
	view.SetTabWidth(4)
	view.SetIndentWidth(4)
	view.SetHighlightCurrentLine(true)

	view.SetWrapMode(gtk.WRAP_WORD_CHAR)

	scroller := gtk.NewScrolledWindow(nil, nil)
	scroller.SetPolicy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
	scroller.Add(view)

	editor := &Editor{
		Buffer:      buffer,
		View:        view,
		scroller:    scroller,
		languageMgr: gtksourceview.SourceLanguageManagerGetDefault(),
		schemeMgr:   gtksourceview.SourceStyleSchemeManagerGetDefault(),
		font:        font,
	}

	editor.applyFont(font)
	return editor, nil
}

func (e *Editor) Widget() *gtk.ScrolledWindow {
	return e.scroller
}

func (e *Editor) CurrentFile() string {
	return e.currentFile
}

func (e *Editor) LanguageID() string {
	return e.currentLang
}

func (e *Editor) CurrentText() string {
	var startIter, endIter gtk.TextIter
	e.Buffer.GetStartIter(&startIter)
	e.Buffer.GetEndIter(&endIter)
	return e.Buffer.GetText(&startIter, &endIter, true)
}

func (e *Editor) SetTheme(themeID string) error {
	scheme, err := ensureScheme(themeID, e.schemeMgr)
	if err != nil {
		return err
	}
	if scheme != nil {
		e.Buffer.SetStyleScheme(scheme)
	}
	return nil
}

func ensureScheme(themeID string, manager *gtksourceview.SourceStyleSchemeManager) (*gtksourceview.SourceStyleScheme, error) {
	installDir, err := themes.Install()
	if err != nil {
		return nil, err
	}
	manager.AppendSearchPath(installDir)
	manager.ForseRescan()
	scheme := manager.GetScheme(themeID)
	if scheme == nil {
		return nil, fmt.Errorf("theme %s not found", themeID)
	}
	return scheme, nil
}

func (e *Editor) applyFont(font config.FontConfig) {
	if font.Family == "" || font.Size == 0 {
		return
	}
	desc := pango.FontDescriptionFromString(fmt.Sprintf("%s %d", font.Family, font.Size))
	e.View.ModifyFont(desc)
}

func (e *Editor) SetFont(font config.FontConfig) {
	e.font = font
	e.applyFont(font)
}

func (e *Editor) LoadFile(path string) error {
	data, err := os.ReadFile(path)
	if err != nil {
		return err
	}
	e.Buffer.BeginNotUndoableAction()
	e.Buffer.SetText(string(data))
	e.Buffer.EndNotUndoableAction()
	e.currentFile = path
	e.detectLanguage(path)
	return nil
}

func (e *Editor) detectLanguage(path string) {
	var lang *gtksourceview.SourceLanguage
	if strings.TrimSpace(path) != "" {
		lang = e.languageMgr.GuessLanguage(path, "")
	}
	if lang != nil {
		e.Buffer.SetLanguage(lang)
		e.currentLang = lang.GetId()
	} else {
		e.currentLang = ""
	}
}

func (e *Editor) NewDocument() {
	e.Buffer.SetText("")
	e.currentFile = ""
	e.currentLang = ""
}

func (e *Editor) Save(path string) error {
	if path == "" {
		if e.currentFile == "" {
			return fmt.Errorf("no file specified")
		}
		path = e.currentFile
	}
	var startIter, endIter gtk.TextIter
	e.Buffer.GetStartIter(&startIter)
	e.Buffer.GetEndIter(&endIter)
	text := e.Buffer.GetText(&startIter, &endIter, true)
	if err := os.WriteFile(path, []byte(text), 0o644); err != nil {
		return err
	}
	e.currentFile = path
	e.detectLanguage(path)
	return nil
}

func (e *Editor) FoldSelection() {
	var startIter, endIter gtk.TextIter
	if !e.Buffer.GetSelectionBounds(&startIter, &endIter) {
		return
	}
	tag := e.Buffer.CreateTag("colony-fold", map[string]interface{}{"invisible": true})
	e.Buffer.ApplyTag(tag, &startIter, &endIter)
}

func (e *Editor) UnfoldAll() {
	var startIter, endIter gtk.TextIter
	e.Buffer.GetBounds(&startIter, &endIter)
	e.Buffer.RemoveAllTags(&startIter, &endIter)
}

func (e *Editor) LoadDirectoryDefaults(root string) {
	searchPath := e.languageMgr.GetSearchPath()
	for _, candidate := range []string{filepath.Join(root, ".colony", "languages"), filepath.Join(root, ".vscode", "syntaxes")} {
		if info, err := os.Stat(candidate); err == nil && info.IsDir() {
			searchPath = append([]string{candidate}, searchPath...)
		}
	}
	e.languageMgr.SetSearchPath(searchPath)
}

func (e *Editor) WriteBufferToTemp() (string, error) {
	file, err := os.CreateTemp("", "colony-buffer-*.tmp")
	if err != nil {
		return "", err
	}
	defer file.Close()
	var startIter, endIter gtk.TextIter
	e.Buffer.GetStartIter(&startIter)
	e.Buffer.GetEndIter(&endIter)
	text := e.Buffer.GetText(&startIter, &endIter, true)
	if _, err := file.WriteString(text); err != nil {
		return "", err
	}
	return file.Name(), nil
}
