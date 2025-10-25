package ui

import (
	"context"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"strings"
	"time"

	"github.com/example/colony/internal/config"
	"github.com/example/colony/internal/i18n"
	"github.com/example/colony/internal/services/fs"
	"github.com/example/colony/internal/services/lint"
	"github.com/example/colony/internal/services/lsp"
	"github.com/example/colony/internal/services/syntax"
	"github.com/example/colony/themes"
	"github.com/mattn/go-gtk/glib"
	"github.com/mattn/go-gtk/gtk"
)

type appState struct {
	cfg         *config.Config
	editor      *syntax.Editor
	explorer    *fs.Explorer
	lspManager  *lsp.Manager
	statusBar   *gtk.Statusbar
	statusCtx   uint
	lintBuffer  *gtk.TextBuffer
	mainWindow  *gtk.Window
	explorerBox *gtk.ScrolledWindow
	paned       *gtk.HPaned
}

func Run(cfg *config.Config) error {
	args := os.Args
	gtk.Init(&args)

	editor, err := syntax.NewEditor(cfg.Font)
	if err != nil {
		return err
	}

	editor.LoadDirectoryDefaults(cfg.Workspace)
	if err := editor.SetTheme(cfg.Theme); err != nil {
		log.Printf("theme error: %v", err)
	}

	explorer, err := fs.NewExplorer(cfg.Workspace)
	if err != nil {
		return err
	}

	lspManager := lsp.NewManager()

	win := gtk.NewWindow(gtk.WINDOW_TOPLEVEL)
	win.SetTitle("Colony Editor")
	win.SetDefaultSize(1280, 800)
	win.Connect("destroy", gtk.MainQuit)

	status := gtk.NewStatusbar()
	statusCtx := status.GetContextId("default")

	lintView := gtk.NewTextView()
	lintView.SetEditable(false)
	lintView.SetCursorVisible(false)
	lintBuffer := lintView.GetBuffer()
	lintScroll := gtk.NewScrolledWindow(nil, nil)
	lintScroll.SetPolicy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
	lintScroll.Add(lintView)
	lintScroll.SetSizeRequest(-1, 160)

	vPaned := gtk.NewVPaned()
	vPaned.Add1(editor.Widget())
	vPaned.Add2(lintScroll)
	vPaned.SetPosition(600)

	paned := gtk.NewHPaned()
	paned.Add1(explorer.Widget())
	paned.Add2(vPaned)
	paned.SetPosition(260)

	toolbar := buildToolbar(cfg, editor, explorer, lspManager, status, statusCtx, lintBuffer, paned)

	vbox := gtk.NewVBox(false, 0)
	vbox.PackStart(toolbar, false, false, 0)
	vbox.PackStart(paned, true, true, 0)
	vbox.PackEnd(status, false, false, 0)

	win.Add(vbox)
	status.Push(statusCtx, i18n.T(cfg.Locale, "status_ready"))

	state := &appState{
		cfg:         cfg,
		editor:      editor,
		explorer:    explorer,
		lspManager:  lspManager,
		statusBar:   status,
		statusCtx:   statusCtx,
		lintBuffer:  lintBuffer,
		mainWindow:  win,
		explorerBox: explorer.Widget(),
		paned:       paned,
	}

	explorer.OnOpen(func(path string) {
		if err := state.openFile(path); err != nil {
			state.setStatus(fmt.Sprintf(i18n.T(cfg.Locale, "status_open_error"), err))
		}
	})

	win.ShowAll()
	gtk.Main()
	lspManager.Shutdown()
	return nil
}

func buildToolbar(cfg *config.Config, editor *syntax.Editor, explorer *fs.Explorer, manager *lsp.Manager, status *gtk.Statusbar, statusCtx uint, lintBuffer *gtk.TextBuffer, paned *gtk.HPaned) *gtk.Toolbar {
	toolbar := gtk.NewToolbar()
	toolbar.SetStyle(gtk.TOOLBAR_BOTH)
	toolbar.SetIconSize(gtk.ICON_SIZE_SMALL_TOOLBAR)

	newBtn := gtk.NewToolButton(nil, i18n.T(cfg.Locale, "toolbar_new"))
	newBtn.OnClicked(func() {
		editor.NewDocument()
		status.Push(statusCtx, i18n.T(cfg.Locale, "status_ready"))
	})

	openBtn := gtk.NewToolButton(nil, i18n.T(cfg.Locale, "toolbar_open"))
	openBtn.OnClicked(func() {
		showOpenDialog(editor, cfg, status, statusCtx)
	})

	saveBtn := gtk.NewToolButton(nil, i18n.T(cfg.Locale, "toolbar_save"))
	saveBtn.OnClicked(func() {
		saveCurrentFile(editor, cfg, status, statusCtx)
	})

	lintBtn := gtk.NewToolButton(nil, i18n.T(cfg.Locale, "toolbar_lint"))
	lintBtn.OnClicked(func() {
		runLint(cfg, editor, lintBuffer, status, statusCtx)
	})

	formatBtn := gtk.NewToolButton(nil, i18n.T(cfg.Locale, "toolbar_format"))
	formatBtn.OnClicked(func() {
		formatBuffer(cfg, editor, status, statusCtx)
	})

	toggleExplorer := gtk.NewToggleToolButton()
	toggleExplorer.SetLabel(i18n.T(cfg.Locale, "toolbar_toggle_explorer"))
	toggleExplorer.SetActive(true)
	toggleExplorer.OnToggled(func() {
		if toggleExplorer.GetActive() {
			paned.SetPosition(260)
		} else {
			paned.SetPosition(0)
		}
	})

	toolbar.Insert(newBtn, -1)
	toolbar.Insert(openBtn, -1)
	toolbar.Insert(saveBtn, -1)
	toolbar.Insert(gtk.NewSeparatorToolItem(), -1)
	toolbar.Insert(lintBtn, -1)
	toolbar.Insert(formatBtn, -1)
	toolbar.Insert(gtk.NewSeparatorToolItem(), -1)
	toolbar.Insert(toggleExplorer, -1)

	menus := buildMenus(cfg, editor, explorer, manager, status, statusCtx, lintBuffer)
	for _, button := range menus {
		toolbar.Insert(button, -1)
	}

	return toolbar
}

func buildMenus(cfg *config.Config, editor *syntax.Editor, explorer *fs.Explorer, manager *lsp.Manager, status *gtk.Statusbar, statusCtx uint, lintBuffer *gtk.TextBuffer) []*gtk.MenuToolButton {
	locale := cfg.Locale

	fileMenu := gtk.NewMenu()
	newItem := gtk.NewMenuItemWithLabel(i18n.T(locale, "menu_new_file"))
	newItem.Connect("activate", func() {
		editor.NewDocument()
		status.Push(statusCtx, i18n.T(locale, "status_ready"))
	})
	openItem := gtk.NewMenuItemWithLabel(i18n.T(locale, "menu_open_file"))
	openItem.Connect("activate", func() {
		showOpenDialog(editor, cfg, status, statusCtx)
	})
	saveItem := gtk.NewMenuItemWithLabel(i18n.T(locale, "menu_save"))
	saveItem.Connect("activate", func() {
		saveCurrentFile(editor, cfg, status, statusCtx)
	})
	saveAsItem := gtk.NewMenuItemWithLabel(i18n.T(locale, "menu_save_as"))
	saveAsItem.Connect("activate", func() {
		showSaveAsDialog(editor, cfg, status, statusCtx)
	})
	quitItem := gtk.NewMenuItemWithLabel(i18n.T(locale, "menu_quit"))
	quitItem.Connect("activate", gtk.MainQuit)
	fileMenu.Append(newItem)
	fileMenu.Append(openItem)
	fileMenu.Append(saveItem)
	fileMenu.Append(saveAsItem)
	fileMenu.Append(gtk.NewSeparatorMenuItem())
	fileMenu.Append(quitItem)

	fileButton := gtk.NewMenuToolButton(nil, i18n.T(locale, "menu_file"))
	fileButton.SetMenu(fileMenu)

	editMenu := gtk.NewMenu()
	foldItem := gtk.NewMenuItemWithLabel(i18n.T(locale, "menu_fold"))
	foldItem.Connect("activate", func() {
		editor.FoldSelection()
	})
	unfoldItem := gtk.NewMenuItemWithLabel(i18n.T(locale, "menu_unfold"))
	unfoldItem.Connect("activate", func() {
		editor.UnfoldAll()
	})
	editMenu.Append(foldItem)
	editMenu.Append(unfoldItem)

	editButton := gtk.NewMenuToolButton(nil, i18n.T(locale, "menu_edit"))
	editButton.SetMenu(editMenu)

	viewMenu := gtk.NewMenu()
	reloadItem := gtk.NewMenuItemWithLabel(i18n.T(locale, "menu_reload_workspace"))
	reloadItem.Connect("activate", func() {
		if err := explorer.Refresh(); err != nil {
			status.Push(statusCtx, err.Error())
		}
	})

	themeMenu := gtk.NewMenu()
	for _, scheme := range themes.AllSchemes() {
		scheme := scheme
		item := gtk.NewMenuItemWithLabel(scheme.DisplayName)
		item.Connect("activate", func() {
			if err := editor.SetTheme(scheme.ID); err != nil {
				status.Push(statusCtx, err.Error())
			}
		})
		themeMenu.Append(item)
	}
	themeItem := gtk.NewMenuItemWithLabel(i18n.T(locale, "menu_theme"))
	themeItem.SetSubmenu(themeMenu)

	viewMenu.Append(reloadItem)
	viewMenu.Append(themeItem)

	viewButton := gtk.NewMenuToolButton(nil, i18n.T(locale, "menu_view"))
	viewButton.SetMenu(viewMenu)

	toolsMenu := gtk.NewMenu()
	lintItem := gtk.NewMenuItemWithLabel(i18n.T(locale, "menu_run_lint"))
	lintItem.Connect("activate", func() {
		runLint(cfg, editor, lintBuffer, status, statusCtx)
	})
	formatItem := gtk.NewMenuItemWithLabel(i18n.T(locale, "menu_format"))
	formatItem.Connect("activate", func() {
		formatBuffer(cfg, editor, status, statusCtx)
	})
	toolsMenu.Append(lintItem)
	toolsMenu.Append(formatItem)

	toolsButton := gtk.NewMenuToolButton(nil, i18n.T(locale, "menu_tools"))
	toolsButton.SetMenu(toolsMenu)

	lspMenu := gtk.NewMenu()
	startItem := gtk.NewMenuItemWithLabel(i18n.T(locale, "menu_start_lsp"))
	startItem.Connect("activate", func() {
		startLSP(cfg, editor, manager, status, statusCtx)
	})
	stopItem := gtk.NewMenuItemWithLabel(i18n.T(locale, "menu_stop_lsp"))
	stopItem.Connect("activate", func() {
		manager.Shutdown()
		status.Push(statusCtx, i18n.T(locale, "status_lsp_stopped"))
	})
	lspMenu.Append(startItem)
	lspMenu.Append(stopItem)

	lspButton := gtk.NewMenuToolButton(nil, "LSP")
	lspButton.SetMenu(lspMenu)

	helpMenu := gtk.NewMenu()
	aboutItem := gtk.NewMenuItemWithLabel(i18n.T(locale, "menu_about"))
	aboutItem.Connect("activate", func() {
		dialog := gtk.NewMessageDialog(nil, gtk.DIALOG_MODAL, gtk.MESSAGE_INFO, gtk.BUTTONS_OK, "Colony Editor\nA modern Go-based code editor prototype.")
		dialog.Run()
		dialog.Destroy()
	})
	helpMenu.Append(aboutItem)

	helpButton := gtk.NewMenuToolButton(nil, i18n.T(locale, "menu_help"))
	helpButton.SetMenu(helpMenu)

	return []*gtk.MenuToolButton{fileButton, editButton, viewButton, toolsButton, lspButton, helpButton}
}

func showOpenDialog(editor *syntax.Editor, cfg *config.Config, status *gtk.Statusbar, statusCtx uint) {
	dialog := gtk.NewFileChooserDialog(i18n.T(cfg.Locale, "menu_open_file"), nil, gtk.FILE_CHOOSER_ACTION_OPEN, i18n.T(cfg.Locale, "menu_open_file"), gtk.RESPONSE_ACCEPT)
	dialog.AddButton(i18n.T(cfg.Locale, "menu_cancel"), gtk.RESPONSE_CANCEL)
	if dialog.Run() == int(gtk.RESPONSE_ACCEPT) {
		filename := dialog.GetFilename()
		if err := editor.LoadFile(filename); err != nil {
			status.Push(statusCtx, err.Error())
		} else {
			status.Push(statusCtx, fmt.Sprintf(i18n.T(cfg.Locale, "status_opened"), filepath.Base(filename)))
		}
	}
	dialog.Destroy()
}

func showSaveAsDialog(editor *syntax.Editor, cfg *config.Config, status *gtk.Statusbar, statusCtx uint) {
	dialog := gtk.NewFileChooserDialog(i18n.T(cfg.Locale, "menu_save_as"), nil, gtk.FILE_CHOOSER_ACTION_SAVE, i18n.T(cfg.Locale, "menu_save"), gtk.RESPONSE_ACCEPT)
	dialog.AddButton(i18n.T(cfg.Locale, "menu_cancel"), gtk.RESPONSE_CANCEL)
	dialog.SetDoOverwriteConfirmation(true)
	if dialog.Run() == int(gtk.RESPONSE_ACCEPT) {
		filename := dialog.GetFilename()
		if err := editor.Save(filename); err != nil {
			status.Push(statusCtx, err.Error())
		} else {
			status.Push(statusCtx, fmt.Sprintf(i18n.T(cfg.Locale, "status_saved"), filepath.Base(filename)))
		}
	}
	dialog.Destroy()
}

func saveCurrentFile(editor *syntax.Editor, cfg *config.Config, status *gtk.Statusbar, statusCtx uint) {
	if editor.CurrentFile() == "" {
		showSaveAsDialog(editor, cfg, status, statusCtx)
		return
	}
	if err := editor.Save(""); err != nil {
		status.Push(statusCtx, err.Error())
	} else {
		status.Push(statusCtx, fmt.Sprintf(i18n.T(cfg.Locale, "status_saved"), filepath.Base(editor.CurrentFile())))
	}
}

func runLint(cfg *config.Config, editor *syntax.Editor, buffer *gtk.TextBuffer, status *gtk.Statusbar, statusCtx uint) {
	cmd := lintCommandFor(cfg, editor)
	if len(cmd) == 0 {
		status.Push(statusCtx, i18n.T(cfg.Locale, "status_no_lint"))
		return
	}

	status.Push(statusCtx, i18n.T(cfg.Locale, "status_running_lint"))
	go func() {
		ctx, cancel := context.WithTimeout(context.Background(), 45*time.Second)
		defer cancel()
		output, err := lint.Run(ctx, cfg.Workspace, cmd)
		glib.IdleAdd(func() bool {
			buffer.SetText(output)
			if err != nil {
				status.Push(statusCtx, fmt.Sprintf(i18n.T(cfg.Locale, "status_lint_failed"), err))
			} else {
				status.Push(statusCtx, i18n.T(cfg.Locale, "status_lint_complete"))
			}
			return false
		})
	}()
}

func formatBuffer(cfg *config.Config, editor *syntax.Editor, status *gtk.Statusbar, statusCtx uint) {
	formatter := formatterFor(editor)
	if len(formatter) == 0 {
		status.Push(statusCtx, i18n.T(cfg.Locale, "status_no_formatter"))
		return
	}

	tempFile, err := editor.WriteBufferToTemp()
	if err != nil {
		status.Push(statusCtx, err.Error())
		return
	}
	defer os.Remove(tempFile)

	status.Push(statusCtx, i18n.T(cfg.Locale, "status_formatting"))

	go func() {
		ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
		defer cancel()
		cmd := formatter
		cmd = append(cmd, tempFile)
		output, err := lint.Run(ctx, filepath.Dir(tempFile), cmd)
		glib.IdleAdd(func() bool {
			if err != nil {
				status.Push(statusCtx, fmt.Sprintf(i18n.T(cfg.Locale, "status_format_failed"), err, output))
			} else {
				data, readErr := os.ReadFile(tempFile)
				if readErr == nil {
					editor.Buffer.SetText(string(data))
					status.Push(statusCtx, i18n.T(cfg.Locale, "status_format_complete"))
				} else {
					status.Push(statusCtx, readErr.Error())
				}
			}
			return false
		})
	}()
}

func lintCommandFor(cfg *config.Config, editor *syntax.Editor) []string {
	lang := editor.LanguageID()
	if lang != "" {
		if cmd, ok := cfg.LintCommands[lang]; ok {
			return cmd
		}
	}
	if current := editor.CurrentFile(); current != "" {
		ext := strings.TrimPrefix(filepath.Ext(current), ".")
		if cmd, ok := cfg.LintCommands[ext]; ok {
			return cmd
		}
	}
	return nil
}

func formatterFor(editor *syntax.Editor) []string {
	lang := editor.LanguageID()
	switch lang {
	case "go":
		return []string{"gofmt", "-w"}
	case "python":
		return []string{"black"}
	case "javascript", "typescript":
		return []string{"prettier", "--write"}
	default:
		return nil
	}
}

func startLSP(cfg *config.Config, editor *syntax.Editor, manager *lsp.Manager, status *gtk.Statusbar, statusCtx uint) {
	lang := editor.LanguageID()
	if lang == "" {
		status.Push(statusCtx, i18n.T(cfg.Locale, "status_no_language"))
		return
	}
	cfgServer, ok := cfg.LSPServers[lang]
	if !ok {
		ext := strings.TrimPrefix(filepath.Ext(editor.CurrentFile()), ".")
		cfgServer, ok = cfg.LSPServers[ext]
	}
	if !ok {
		status.Push(statusCtx, i18n.T(cfg.Locale, "status_no_lsp"))
		return
	}

	status.Push(statusCtx, i18n.T(cfg.Locale, "status_starting_lsp"))

	go func() {
		client, err := manager.Ensure(lang, cfgServer, cfg.Workspace)
		if err != nil {
			glib.IdleAdd(func() bool {
				status.Push(statusCtx, fmt.Sprintf(i18n.T(cfg.Locale, "status_lsp_error"), err))
				return false
			})
			return
		}
		text := editor.CurrentText()
		file := editor.CurrentFile()
		glib.IdleAdd(func() bool {
			if file != "" {
				if err := client.DidOpen(file, lang, text); err != nil {
					status.Push(statusCtx, fmt.Sprintf(i18n.T(cfg.Locale, "status_lsp_error"), err))
				} else {
					status.Push(statusCtx, i18n.T(cfg.Locale, "status_lsp_ready"))
				}
			}
			return false
		})
	}()
}

func (a *appState) openFile(path string) error {
	if err := a.editor.LoadFile(path); err != nil {
		return err
	}
	a.setStatus(fmt.Sprintf(i18n.T(a.cfg.Locale, "status_opened"), filepath.Base(path)))
	return nil
}

func (a *appState) setStatus(message string) {
	a.statusBar.Push(a.statusCtx, message)
}

func (a *appState) startWatcher() {
	// Placeholder for future filesystem watcher integration.
}
