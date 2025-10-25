package fs

import (
	"os"
	"path/filepath"
	"sort"
	"strings"

	"github.com/mattn/go-gtk/gdkpixbuf"
	"github.com/mattn/go-gtk/glib"
	"github.com/mattn/go-gtk/gtk"
)

type Explorer struct {
	rootDir string
	store   *gtk.TreeStore
	tree    *gtk.TreeView
	scroll  *gtk.ScrolledWindow
	onOpen  func(string)
}

func NewExplorer(root string) (*Explorer, error) {
	if root == "" {
		root = "."
	}
	store := gtk.NewTreeStore(gdkpixbuf.GetType(), glib.G_TYPE_STRING, glib.G_TYPE_STRING)
	tree := gtk.NewTreeView()
	tree.SetHeadersVisible(false)
	tree.SetModel(store.ToTreeModel())

	tree.AppendColumn(gtk.NewTreeViewColumnWithAttributes("", gtk.NewCellRendererPixbuf(), "pixbuf", 0))
	tree.AppendColumn(gtk.NewTreeViewColumnWithAttributes("", gtk.NewCellRendererText(), "text", 1))

	scroll := gtk.NewScrolledWindow(nil, nil)
	scroll.SetPolicy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
	scroll.Add(tree)

	explorer := &Explorer{
		rootDir: root,
		store:   store,
		tree:    tree,
		scroll:  scroll,
	}

	if err := explorer.Refresh(); err != nil {
		return nil, err
	}

	tree.Connect("row_activated", func() {
		explorer.handleActivation()
	})

	return explorer, nil
}

func (e *Explorer) Widget() *gtk.ScrolledWindow {
	return e.scroll
}

func (e *Explorer) OnOpen(handler func(string)) {
	e.onOpen = handler
}

func (e *Explorer) Refresh() error {
	e.store.Clear()
	return e.addDirectory(nil, e.rootDir)
}

func (e *Explorer) addDirectory(parent *gtk.TreeIter, path string) error {
	entries, err := os.ReadDir(path)
	if err != nil {
		return err
	}

	sort.SliceStable(entries, func(i, j int) bool {
		if entries[i].IsDir() && !entries[j].IsDir() {
			return true
		}
		if !entries[i].IsDir() && entries[j].IsDir() {
			return false
		}
		return strings.ToLower(entries[i].Name()) < strings.ToLower(entries[j].Name())
	})

	for _, entry := range entries {
		name := entry.Name()
		fullPath := filepath.Join(path, name)
		var iter gtk.TreeIter
		e.store.Append(&iter, parent)
		if entry.IsDir() {
			e.store.Set(&iter, folderIcon(), name, fullPath)
			if err := e.addDirectory(&iter, fullPath); err != nil {
				continue
			}
		} else {
			e.store.Set(&iter, fileIcon(), name, fullPath)
		}
	}
	return nil
}

func (e *Explorer) handleActivation() {
	if e.onOpen == nil {
		return
	}
	var path *gtk.TreePath
	var column *gtk.TreeViewColumn
	e.tree.GetCursor(&path, &column)
	if path == nil {
		return
	}
	iter := new(gtk.TreeIter)
	if !e.store.GetIter(iter, path) {
		return
	}
	var value glib.GValue
	value.Init(glib.G_TYPE_STRING)
	e.store.GetValue(iter, 2, &value)
	filePath := value.GetString()
	info, err := os.Stat(filePath)
	if err != nil || info.IsDir() {
		return
	}
	e.onOpen(filePath)
}

var (
	folderPixbuf *gdkpixbuf.Pixbuf
	filePixbuf   *gdkpixbuf.Pixbuf
)

func folderIcon() *gdkpixbuf.Pixbuf {
	if folderPixbuf == nil {
		folderPixbuf = gtk.NewImage().RenderIcon(gtk.STOCK_DIRECTORY, gtk.ICON_SIZE_SMALL_TOOLBAR, "")
	}
	return folderPixbuf
}

func fileIcon() *gdkpixbuf.Pixbuf {
	if filePixbuf == nil {
		filePixbuf = gtk.NewImage().RenderIcon(gtk.STOCK_FILE, gtk.ICON_SIZE_SMALL_TOOLBAR, "")
	}
	return filePixbuf
}
