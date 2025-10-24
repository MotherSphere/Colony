use crate::config::AppConfig;
use crate::fs::WorkspaceManager;
use crate::localization::LocalizationProvider;
use crate::lsp::LspCoordinator;
use crate::theme::{ThemeId, ThemeRegistry, ThemeVariant};
use anyhow::{Context, Result};
use directories::ProjectDirs;
use gtk::gdk;
use gtk::prelude::*;
use lsp_types::Url;
use once_cell::sync::Lazy;
use parking_lot::RwLock;
use sourceview5::prelude::*;
use sourceview5::{LanguageManager, View};
use std::cell::RefCell;
use std::fs;
use std::path::{Path, PathBuf};
use std::sync::Arc;
use tokio::runtime::Runtime;

static LANGUAGE_MANAGER: Lazy<LanguageManager> = Lazy::new(LanguageManager::new);

pub struct EditorView {
    container: gtk::Box,
    scroller: gtk::ScrolledWindow,
    view: View,
    buffer: sourceview5::Buffer,
    current_path: RefCell<Option<PathBuf>>,
    current_language: RefCell<Option<String>>,
    _runtime: Arc<Runtime>,
    config: Arc<RwLock<AppConfig>>,
    themes: Arc<ThemeRegistry>,
    _localization: Arc<LocalizationProvider>,
    lsp: Arc<LspCoordinator>,
    workspace: Arc<WorkspaceManager>,
}

impl EditorView {
    pub fn new(
        runtime: Arc<Runtime>,
        config: Arc<RwLock<AppConfig>>,
        themes: Arc<ThemeRegistry>,
        localization: Arc<LocalizationProvider>,
        lsp: Arc<LspCoordinator>,
        workspace: Arc<WorkspaceManager>,
    ) -> Arc<Self> {
        let buffer = sourceview5::Buffer::new(None);
        buffer.set_highlight_syntax(true);
        let view = View::with_buffer(&buffer);
        view.set_vexpand(true);
        view.set_hexpand(true);
        let editor_config = config.read().editor.clone();
        view.set_show_line_numbers(editor_config.show_line_numbers);
        view.set_insert_spaces_instead_of_tabs(editor_config.insert_spaces);
        view.set_tab_width(editor_config.tab_width as u32);
        view.set_wrap_mode(if editor_config.wrap_lines {
            gtk::WrapMode::WordChar
        } else {
            gtk::WrapMode::None
        });
        view.set_highlight_current_line(editor_config.highlight_current_line);

        let scroller = gtk::ScrolledWindow::builder()
            .hexpand(true)
            .vexpand(true)
            .build();
        scroller.set_child(Some(&view));

        let container = gtk::Box::new(gtk::Orientation::Vertical, 0);
        container.append(&scroller);

        let editor = Arc::new(Self {
            container,
            scroller,
            view,
            buffer,
            current_path: RefCell::new(None),
            current_language: RefCell::new(None),
            _runtime: runtime,
            config,
            themes,
            _localization: localization,
            lsp,
            workspace,
        });

        editor.apply_theme();
        editor.apply_font();
        editor
    }

    pub fn widget(&self) -> &gtk::Box {
        &self.container
    }

    pub fn open_file(&self, path: PathBuf) {
        match fs::read_to_string(&path) {
            Ok(content) => {
                self.buffer.set_text(&content);
                self.buffer.set_modified(false);
                *self.current_path.borrow_mut() = Some(path.clone());

                let auto_detect = self.config.read().workspace.auto_detect_language;
                if auto_detect {
                    if let Some(language) = Self::guess_language(&path) {
                        self.buffer.set_language(Some(&language));
                        *self.current_language.borrow_mut() =
                            language.id().map(|id| id.to_string());
                    } else {
                        self.buffer.set_language(None);
                        *self.current_language.borrow_mut() = None;
                    }
                } else {
                    self.buffer.set_language(None);
                    *self.current_language.borrow_mut() = None;
                }

                self.connect_language_services();
            }
            Err(err) => {
                log::error!("failed to open file {}: {err}", path.display());
            }
        }
    }

    pub fn new_file(&self) {
        self.buffer.set_text("");
        self.buffer.set_modified(false);
        *self.current_path.borrow_mut() = None;
        *self.current_language.borrow_mut() = None;
    }

    pub fn update_theme(&self, id: ThemeId, variant: ThemeVariant) {
        {
            let mut cfg = self.config.write();
            cfg.theme.active_theme = id;
            cfg.theme.variant = variant;
            if let Err(err) = cfg.save() {
                log::error!("failed to save theme preference: {err}");
            }
        }
        self.apply_theme();
    }

    pub fn update_font(&self, family: &str, size: f32) {
        {
            let mut cfg = self.config.write();
            cfg.font.family = family.to_string();
            cfg.font.ui_family = family.to_string();
            cfg.font.size = size;
            if let Err(err) = cfg.save() {
                log::error!("failed to save font preference: {err}");
            }
        }
        self.apply_font();
    }

    pub fn save(&self) -> Result<()> {
        let Some(path) = self.current_path.borrow().clone() else {
            return Ok(());
        };
        let text = self
            .buffer
            .text(&self.buffer.start_iter(), &self.buffer.end_iter(), true);
        fs::write(&path, text.as_str())
            .with_context(|| format!("failed to write {}", path.display()))?;

        if let Some(language) = self.current_language.borrow().clone() {
            if let Ok(uri) = Url::from_file_path(&path) {
                self.lsp.document_saved(&language, uri);
            }
        }
        Ok(())
    }

    fn connect_language_services(&self) {
        let Some(language_id) = self.current_language.borrow().clone() else {
            return;
        };
        let root = self
            .workspace
            .current_root()
            .or_else(|| {
                self.current_path
                    .borrow()
                    .as_ref()
                    .and_then(|p| p.parent().map(Path::to_path_buf))
            })
            .unwrap_or_else(|| PathBuf::from("."));

        self.lsp.ensure_session(&language_id, &root);
        if let Some(path) = self.current_path.borrow().clone() {
            if let Ok(uri) = Url::from_file_path(&path) {
                let text = self
                    .buffer
                    .text(&self.buffer.start_iter(), &self.buffer.end_iter(), true)
                    .to_string();
                self.lsp.document_opened(&language_id, uri, text);
            }
        }
    }

    fn apply_theme(&self) {
        let theme_cfg = self.config.read().theme.clone();
        let theme_id = theme_cfg.active_theme;
        let variant = theme_cfg.variant;
        if let Some(palette) = self.themes.palette(theme_id, variant) {
            if let Some(scheme) = ensure_style_scheme(palette) {
                self.buffer.set_style_scheme(Some(&scheme));
            }
            let css = format!(
                "textview {{
                    background-color: {bg};
                    color: {fg};
                    selection-background-color: {selection};
                    caret-color: {cursor};
                }}
                textview:selected {{ color: {fg}; }}",
                bg = to_css_color(palette.background),
                fg = to_css_color(palette.foreground),
                selection = to_css_color(palette.selection),
                cursor = to_css_color(palette.cursor),
            );
            apply_css(&self.view, &css);
        }
    }

    fn apply_font(&self) {
        let font_cfg = self.config.read().font.clone();
        let css = format!(
            "textview {{ font-family: '{}'; font-size: {}pt; }}",
            font_cfg.family, font_cfg.size
        );
        apply_css(&self.view, &css);
    }

    fn guess_language(path: &Path) -> Option<sourceview5::Language> {
        let filename = path.file_name().and_then(|name| name.to_str());
        LANGUAGE_MANAGER.guess_language(filename, None)
    }
}

fn ensure_style_scheme(palette: &crate::theme::ThemePalette) -> Option<sourceview5::StyleScheme> {
    let dirs = ProjectDirs::from("com", "Colony", "Editor")?;
    let theme_dir = dirs.data_dir().join("themes");
    if let Err(err) = fs::create_dir_all(&theme_dir) {
        log::error!(
            "failed to create theme directory {}: {err}",
            theme_dir.display()
        );
        return None;
    }
    let slug = format!(
        "colony-{}-{}",
        palette.name.to_lowercase().replace(' ', "-"),
        match palette.variant {
            ThemeVariant::Dark => "dark",
            ThemeVariant::Light => "light",
        }
    );
    let scheme_path = theme_dir.join(format!("{slug}.xml"));
    let scheme_data = build_scheme_xml(&slug, palette);
    if let Err(err) = fs::write(&scheme_path, scheme_data) {
        log::error!(
            "failed to write theme file {}: {err}",
            scheme_path.display()
        );
        return None;
    }

    let manager = sourceview5::StyleSchemeManager::new();
    let mut paths = manager.search_path();
    paths.push(theme_dir.to_string_lossy().to_string());
    manager.set_search_path(&paths);
    manager.force_rescan();
    manager.scheme(&slug)
}

fn build_scheme_xml(slug: &str, palette: &crate::theme::ThemePalette) -> String {
    let line = blend(palette.background, palette.accent, 0.15);
    format!(
        r#"<?xml version="1.0" encoding="UTF-8"?>
<style-scheme id="{slug}" name="{name}" version="1.0">
  <author>Colony</author>
  <description>{name} color scheme for Colony Editor</description>
  <style name="text" foreground="{fg}" background="{bg}"/>
  <style name="selection" background="{selection}"/>
  <style name="cursor" foreground="{cursor}"/>
  <style name="line-numbers" foreground="{fg}" background="{bg}"/>
  <style name="current-line" background="{line}"/>
  <style name="comment" foreground="{comment}"/>
  <style name="string" foreground="{string}"/>
  <style name="keyword" foreground="{keyword}" bold="true"/>
  <style name="function" foreground="{function}" bold="true"/>
  <style name="variable" foreground="{variable}"/>
  <style name="number" foreground="{accent}"/>
</style-scheme>
"#,
        slug = slug,
        name = palette.name,
        fg = to_css_color(palette.foreground),
        bg = to_css_color(palette.background),
        selection = to_css_color(palette.selection),
        cursor = to_css_color(palette.cursor),
        line = to_css_color(line),
        comment = to_css_color(palette.comment),
        string = to_css_color(palette.string),
        keyword = to_css_color(palette.keyword),
        function = to_css_color(palette.function),
        variable = to_css_color(palette.variable),
        accent = to_css_color(palette.accent)
    )
}

fn to_css_color(color: syntect::highlighting::Color) -> String {
    format!("#{:02x}{:02x}{:02x}", color.r, color.g, color.b)
}

fn blend(
    a: syntect::highlighting::Color,
    b: syntect::highlighting::Color,
    ratio: f32,
) -> syntect::highlighting::Color {
    let inv = 1.0 - ratio;
    let mix = |x: u8, y: u8| ((x as f32 * inv) + (y as f32 * ratio)).round() as u8;
    syntect::highlighting::Color {
        r: mix(a.r, b.r),
        g: mix(a.g, b.g),
        b: mix(a.b, b.b),
        a: 0xff,
    }
}

fn apply_css(widget: &impl IsA<gtk::Widget>, css: &str) {
    if let Some(display) = gdk::Display::default() {
        let provider = gtk::CssProvider::new();
        if let Err(err) = provider.load_from_data(css.as_bytes()) {
            log::error!("failed to load CSS: {err}");
            return;
        }
        gtk::StyleContext::add_provider_for_display(
            &display,
            &provider,
            gtk::STYLE_PROVIDER_PRIORITY_APPLICATION,
        );
        let context = widget.style_context();
        context.add_provider(&provider, gtk::STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
}
