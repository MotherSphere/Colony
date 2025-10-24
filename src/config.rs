use crate::theme::{ThemeId, ThemeVariant};
use anyhow::{Context, Result};
use directories::ProjectDirs;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::fs;
use std::path::PathBuf;
use std::time::Duration;

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(default)]
pub struct AppConfig {
    pub workspace: WorkspaceConfig,
    pub theme: ThemeConfig,
    pub font: FontConfig,
    pub lsp: LspConfig,
    pub editor: EditorConfig,
    pub locale: Option<String>,
}

impl Default for AppConfig {
    fn default() -> Self {
        Self {
            workspace: WorkspaceConfig::default(),
            theme: ThemeConfig::default(),
            font: FontConfig::default(),
            lsp: LspConfig::default(),
            editor: EditorConfig::default(),
            locale: None,
        }
    }
}

impl AppConfig {
    pub fn load() -> Result<Self> {
        if let Some(path) = Self::config_path() {
            if path.exists() {
                let contents = fs::read_to_string(&path)
                    .with_context(|| format!("failed to read config at {}", path.display()))?;
                let config: AppConfig = toml::from_str(&contents)
                    .with_context(|| format!("failed to parse config at {}", path.display()))?;
                return Ok(config);
            }
        }

        Ok(Self::default())
    }

    pub fn save(&self) -> Result<()> {
        if let Some(path) = Self::config_path() {
            if let Some(parent) = path.parent() {
                fs::create_dir_all(parent)?;
            }
            let serialized = toml::to_string_pretty(self)?;
            fs::write(&path, serialized)?;
        }
        Ok(())
    }

    fn config_path() -> Option<PathBuf> {
        ProjectDirs::from("com", "Colony", "Editor")
            .map(|dirs| dirs.config_dir().join("config.toml"))
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(default)]
pub struct WorkspaceConfig {
    pub roots: Vec<PathBuf>,
    pub auto_detect_language: bool,
}

impl Default for WorkspaceConfig {
    fn default() -> Self {
        Self {
            roots: vec![std::env::current_dir().unwrap_or_else(|_| PathBuf::from("."))],
            auto_detect_language: true,
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(default)]
pub struct ThemeConfig {
    pub active_theme: ThemeId,
    pub variant: ThemeVariant,
}

impl Default for ThemeConfig {
    fn default() -> Self {
        Self {
            active_theme: ThemeId::Catppuccin,
            variant: ThemeVariant::Dark,
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(default)]
pub struct FontConfig {
    pub family: String,
    pub size: f32,
    pub ui_family: String,
}

impl Default for FontConfig {
    fn default() -> Self {
        Self {
            family: "Fira Code".to_string(),
            size: 13.0,
            ui_family: "Inter".to_string(),
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(default)]
pub struct LspConfig {
    pub servers: HashMap<String, LspServerConfig>,
}

impl Default for LspConfig {
    fn default() -> Self {
        let mut servers = HashMap::new();
        servers.insert(
            "rust".into(),
            LspServerConfig {
                command: "rust-analyzer".into(),
                args: vec![],
                initialization_options: serde_json::Value::Null,
            },
        );
        servers.insert(
            "python".into(),
            LspServerConfig {
                command: "pyright-langserver".into(),
                args: vec!["--stdio".into()],
                initialization_options: serde_json::Value::Null,
            },
        );
        Self { servers }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(default)]
pub struct EditorConfig {
    pub tab_width: u8,
    pub insert_spaces: bool,
    pub wrap_lines: bool,
    pub show_line_numbers: bool,
    pub highlight_current_line: bool,
    pub auto_save: Option<DurationConfig>,
}

impl Default for EditorConfig {
    fn default() -> Self {
        Self {
            tab_width: 4,
            insert_spaces: true,
            wrap_lines: false,
            show_line_numbers: true,
            highlight_current_line: true,
            auto_save: Some(DurationConfig { millis: 5_000 }),
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DurationConfig {
    pub millis: u64,
}

impl DurationConfig {
    pub fn as_duration(&self) -> Duration {
        Duration::from_millis(self.millis)
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LspServerConfig {
    pub command: String,
    pub args: Vec<String>,
    #[serde(default)]
    pub initialization_options: serde_json::Value,
}

impl LspServerConfig {
    pub fn command_line(&self) -> (String, Vec<String>) {
        (self.command.clone(), self.args.clone())
    }
}
