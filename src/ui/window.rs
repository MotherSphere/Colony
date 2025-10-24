use crate::config::AppConfig;
use crate::fs::WorkspaceManager;
use crate::localization::LocalizationProvider;
use crate::lsp::LspCoordinator;
use crate::theme::{ThemeId, ThemeRegistry, ThemeVariant};
use crate::ui::{EditorView, FileExplorer, StatusBar, TopBar};
use gtk::prelude::*;
use gtk::{Application, ApplicationWindow, gdk, gio, pango};
use parking_lot::RwLock;
use std::rc::Rc;
use std::sync::Arc;
use tokio::runtime::Runtime;
use unic_langid::LanguageIdentifier;

pub struct ColonyWindow {
    window: ApplicationWindow,
    editor: Arc<EditorView>,
    file_explorer: Rc<FileExplorer>,
    status_bar: StatusBar,
    top_bar: TopBar,
    localization: Arc<LocalizationProvider>,
    config: Arc<RwLock<AppConfig>>,
}

impl ColonyWindow {
    #[allow(clippy::too_many_arguments)]
    pub fn new(
        app: &Application,
        runtime: Arc<Runtime>,
        config: Arc<RwLock<AppConfig>>,
        workspace: Arc<WorkspaceManager>,
        lsp: Arc<LspCoordinator>,
        themes: Arc<ThemeRegistry>,
        localization: Arc<LocalizationProvider>,
    ) -> Self {
        let window = ApplicationWindow::builder()
            .application(app)
            .title("Colony Editor")
            .default_width(1280)
            .default_height(800)
            .build();

        window.add_css_class("colony-window");

        let root = gtk::Box::new(gtk::Orientation::Vertical, 0);

        let top_bar = TopBar::new(config.clone(), themes.clone(), localization.clone());
        root.append(top_bar.widget());

        let paned = gtk::Paned::builder()
            .orientation(gtk::Orientation::Horizontal)
            .shrink_start_child(false)
            .shrink_end_child(false)
            .build();
        paned.set_position(280);

        let file_explorer = FileExplorer::new(workspace.clone(), localization.clone());
        let editor = EditorView::new(
            runtime.clone(),
            config.clone(),
            themes.clone(),
            localization.clone(),
            lsp.clone(),
            workspace.clone(),
        );

        let explorer_revealer = gtk::Revealer::builder()
            .transition_type(gtk::RevealerTransitionType::SlideRight)
            .reveal_child(true)
            .build();
        explorer_revealer.set_child(Some(file_explorer.widget()));

        paned.set_start_child(Some(&explorer_revealer));
        paned.set_end_child(Some(editor.widget()));

        root.append(&paned);

        let status_bar = StatusBar::new(localization.clone());
        root.append(status_bar.widget());

        window.set_content(Some(&root));

        let window_instance = Self {
            window,
            editor: editor.clone(),
            file_explorer: file_explorer.clone(),
            status_bar,
            top_bar,
            localization: localization.clone(),
            config: config.clone(),
        };

        window_instance.setup_actions(explorer_revealer, paned, editor, file_explorer);

        {
            let font_cfg = config.read().font.clone();
            apply_ui_font(&window_instance.window, &font_cfg.ui_family, font_cfg.size);
        }

        window_instance
    }

    pub fn present(&self) {
        self.window.present();
    }

    fn setup_actions(
        &self,
        explorer_revealer: gtk::Revealer,
        paned: gtk::Paned,
        editor: Arc<EditorView>,
        explorer: Rc<FileExplorer>,
    ) {
        let window = self.window.clone();
        let window_for_font = self.window.clone();
        let status = self.status_bar.clone();
        let localization = self.localization.clone();

        explorer.connect_open({
            let editor = editor.clone();
            let status = status.clone();
            let localization = localization.clone();
            move |path| {
                editor.open_file(path.clone());
                status.set_status(&format!(
                    "{}: {}",
                    localization.text("menu_file"),
                    path.display()
                ));
            }
        });

        self.top_bar.new_button.connect_clicked({
            let editor = editor.clone();
            let status = status.clone();
            let localization = localization.clone();
            move |_| {
                editor.new_file();
                status.set_status(&localization.text("status_ready"));
            }
        });

        self.top_bar.save_button.connect_clicked({
            let editor = editor.clone();
            let status = status.clone();
            let localization = localization.clone();
            move |_| {
                if let Err(err) = editor.save() {
                    log::error!("save failed: {err:?}");
                    status.set_status(&format!("Save error: {err}"));
                } else {
                    status.set_status(&localization.text("status_ready"));
                }
            }
        });

        self.top_bar.open_button.connect_clicked({
            let window = window.clone();
            let editor = editor.clone();
            let status = status.clone();
            let localization = localization.clone();
            move |_| {
                let dialog = gtk::FileDialog::builder().title("Open File").build();
                dialog.open(Some(&window), None::<&gio::Cancellable>, move |result| {
                    if let Ok(file) = result {
                        if let Some(path) = file.path() {
                            editor.open_file(path.clone());
                            status.set_status(&format!(
                                "{}: {}",
                                localization.text("menu_file"),
                                path.display()
                            ));
                        }
                    }
                });
            }
        });

        self.top_bar.toggle_explorer.connect_toggled({
            let paned = paned.clone();
            let revealer = explorer_revealer.clone();
            move |button| {
                let visible = button.is_active();
                revealer.set_reveal_child(visible);
                if visible {
                    paned.set_position(280);
                }
            }
        });

        self.top_bar.theme_selector.connect_changed({
            let editor = editor.clone();
            let status = status.clone();
            let localization = localization.clone();
            move |combo| {
                if let Some(id) = combo.active_id() {
                    if let Some((theme_id, variant)) = parse_theme_identifier(&id) {
                        editor.update_theme(theme_id, variant);
                        status.set_status(&localization.text("status_ready"));
                    }
                }
            }
        });

        self.top_bar.locale_selector.connect_changed({
            let localization = self.localization.clone();
            let top_bar = &self.top_bar;
            let explorer = explorer.clone();
            let status = &self.status_bar;
            let config = self.config.clone();
            move |combo| {
                if let Some(id) = combo.active_id() {
                    if let Ok(lang) = id.parse::<LanguageIdentifier>() {
                        localization.set_locale(&lang);
                        top_bar.refresh_texts(&localization);
                        explorer.refresh_texts();
                        status.set_status(&localization.text("status_ready"));
                        {
                            let mut cfg = config.write();
                            cfg.locale = Some(id.to_string());
                            if let Err(err) = cfg.save() {
                                log::error!("failed to save locale: {err}");
                            }
                        }
                    }
                }
            }
        });

        self.top_bar.font_button.connect_font_activated({
            let editor = editor.clone();
            let window = window_for_font.clone();
            let status = status.clone();
            let localization = localization.clone();
            move |_btn, font| {
                let desc = pango::FontDescription::from_string(font);
                if let Some(family) = desc.family() {
                    let size = if desc.size_is_absolute() {
                        desc.size() as f32
                    } else {
                        desc.size() as f32 / pango::SCALE as f32
                    };
                    let points = if size <= 0.0 { 12.0 } else { size };
                    editor.update_font(family, points);
                    apply_ui_font(&window, family, points);
                    status.set_status(&localization.text("status_ready"));
                }
            }
        });
    }
}

fn parse_theme_identifier(value: &str) -> Option<(ThemeId, ThemeVariant)> {
    let mut parts = value.split(':');
    let theme = parts.next()?;
    let variant = parts.next()?;
    let id = match theme {
        "Catppuccin" => ThemeId::Catppuccin,
        "Everblush" => ThemeId::Everblush,
        "Gruvbox" => ThemeId::Gruvbox,
        "Solarized" => ThemeId::Solarized,
        "SolarizedLight" => ThemeId::SolarizedLight,
        "Cyberdream" => ThemeId::Cyberdream,
        "NordDark" => ThemeId::NordDark,
        "OneDark" => ThemeId::OneDark,
        "OneLight" => ThemeId::OneLight,
        "Dracula" => ThemeId::Dracula,
        other => {
            log::warn!("unknown theme id: {other}");
            return None;
        }
    };
    let variant = match variant {
        "Dark" => ThemeVariant::Dark,
        "Light" => ThemeVariant::Light,
        other => {
            log::warn!("unknown theme variant: {other}");
            return None;
        }
    };
    Some((id, variant))
}

fn apply_ui_font(window: &ApplicationWindow, family: &str, size: f32) {
    if let Some(display) = gdk::Display::default() {
        let provider = gtk::CssProvider::new();
        let css = format!("* {{ font-family: '{}'; font-size: {}pt; }}", family, size);
        if provider.load_from_data(css.as_bytes()).is_ok() {
            gtk::StyleContext::add_provider_for_display(
                &display,
                &provider,
                gtk::STYLE_PROVIDER_PRIORITY_APPLICATION,
            );
            window
                .style_context()
                .add_provider(&provider, gtk::STYLE_PROVIDER_PRIORITY_APPLICATION);
        }
    }
}
