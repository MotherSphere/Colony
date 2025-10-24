use crate::config::AppConfig;
use crate::fs::WorkspaceManager;
use crate::localization::LocalizationProvider;
use crate::lsp::LspCoordinator;
use crate::theme::ThemeRegistry;
use crate::ui::ColonyWindow;
use anyhow::Result;
use gtk::Application;
use gtk::gio;
use gtk::prelude::*;
use parking_lot::RwLock;
use std::sync::Arc;
use tokio::runtime::Runtime;
use unic_langid::LanguageIdentifier;

pub struct ColonyEditor {
    application: Application,
    runtime: Arc<Runtime>,
    config: Arc<RwLock<AppConfig>>,
    workspace: Arc<WorkspaceManager>,
    lsp: Arc<LspCoordinator>,
    themes: Arc<ThemeRegistry>,
    localization: Arc<LocalizationProvider>,
}

impl ColonyEditor {
    pub fn new(runtime: Runtime) -> Result<Self> {
        let application = Application::builder()
            .application_id("com.colony.editor")
            .flags(gio::ApplicationFlags::HANDLES_OPEN)
            .build();

        let runtime = Arc::new(runtime);
        let config = Arc::new(RwLock::new(AppConfig::load()?));
        let themes = Arc::new(ThemeRegistry::new());
        let localization = Arc::new(LocalizationProvider::new()?);
        if let Some(locale) = config.read().locale.clone() {
            if let Ok(lang) = locale.parse::<LanguageIdentifier>() {
                localization.set_locale(&lang);
            }
        }
        let workspace = Arc::new(WorkspaceManager::new(config.clone()));
        let lsp = Arc::new(LspCoordinator::new(runtime.clone(), config.clone()));

        Ok(Self {
            application,
            runtime,
            config,
            workspace,
            lsp,
            themes,
            localization,
        })
    }

    pub fn run(self) {
        let config = self.config.clone();
        let workspace = self.workspace.clone();
        let lsp = self.lsp.clone();
        let themes = self.themes.clone();
        let localization = self.localization.clone();
        let runtime = self.runtime.clone();

        self.application.connect_open(move |app, files, _| {
            if let Some(file) = files.first() {
                if let Some(path) = file.path() {
                    workspace.set_workspace_root(path.clone());
                    if let Some(parent) = path.parent() {
                        workspace.set_workspace_root(parent.to_path_buf());
                    }
                }
            }
            Self::activate_application(
                app,
                runtime.clone(),
                config.clone(),
                workspace.clone(),
                lsp.clone(),
                themes.clone(),
                localization.clone(),
            );
        });

        let runtime = self.runtime.clone();
        let config = self.config.clone();
        let workspace = self.workspace.clone();
        let lsp = self.lsp.clone();
        let themes = self.themes.clone();
        let localization = self.localization.clone();

        self.application.connect_activate(move |app| {
            Self::activate_application(
                app,
                runtime.clone(),
                config.clone(),
                workspace.clone(),
                lsp.clone(),
                themes.clone(),
                localization.clone(),
            );
        });

        self.application.run();
    }

    fn activate_application(
        app: &Application,
        runtime: Arc<Runtime>,
        config: Arc<AppConfig>,
        workspace: Arc<WorkspaceManager>,
        lsp: Arc<LspCoordinator>,
        themes: Arc<ThemeRegistry>,
        localization: Arc<LocalizationProvider>,
    ) {
        let window = ColonyWindow::new(app, runtime, config, workspace, lsp, themes, localization);
        window.present();
    }
}
