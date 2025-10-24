use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::str::FromStr;
use syntect::highlighting::{Color, FontStyle, ScopeSelectors, ScopeStyle, Style, Theme};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum ThemeId {
    Catppuccin,
    Everblush,
    Gruvbox,
    Solarized,
    SolarizedLight,
    Cyberdream,
    NordDark,
    OneDark,
    OneLight,
    Dracula,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum ThemeVariant {
    Dark,
    Light,
}

#[derive(Debug, Clone)]
pub struct ThemePalette {
    pub name: &'static str,
    pub background: Color,
    pub foreground: Color,
    pub cursor: Color,
    pub selection: Color,
    pub comment: Color,
    pub keyword: Color,
    pub string: Color,
    pub function: Color,
    pub variable: Color,
    pub accent: Color,
    pub variant: ThemeVariant,
}

impl ThemePalette {
    fn to_theme(&self) -> Theme {
        let mut theme = Theme::default();
        theme.name = Some(self.name.to_string());
        theme.settings.background = Some(self.background);
        theme.settings.foreground = Some(self.foreground);
        theme.settings.caret = Some(self.cursor);
        theme.settings.selection = Some(self.selection);
        theme.settings.line_highlight = Some(Color {
            a: 0x33,
            ..self.accent
        });
        theme.settings.gutter = Some(self.background);
        theme.settings.gutter_foreground = Some(Color {
            a: 0x99,
            ..self.foreground
        });

        let mut scopes = Vec::new();
        scopes.push(scope(
            "comment",
            Style {
                foreground: self.comment,
                ..Style::default()
            },
        ));
        scopes.push(scope(
            "keyword",
            Style {
                foreground: self.keyword,
                font_style: FontStyle::BOLD,
            },
        ));
        scopes.push(scope(
            "string",
            Style {
                foreground: self.string,
                ..Style::default()
            },
        ));
        scopes.push(scope(
            "entity.name.function",
            Style {
                foreground: self.function,
                font_style: FontStyle::BOLD,
            },
        ));
        scopes.push(scope(
            "variable",
            Style {
                foreground: self.variable,
                ..Style::default()
            },
        ));
        scopes.push(scope(
            "constant.numeric",
            Style {
                foreground: self.accent,
                ..Style::default()
            },
        ));

        theme.scopes = scopes;
        theme
    }
}

fn scope(selector: &str, style: Style) -> ScopeStyle {
    ScopeStyle {
        scope: ScopeSelectors::from_str(selector).expect("valid scope selector"),
        style,
    }
}

use std::str::FromStr;

pub struct ThemeRegistry {
    themes: HashMap<(ThemeId, ThemeVariant), ThemePalette>,
}

impl ThemeRegistry {
    pub fn new() -> Self {
        let mut themes = HashMap::new();
        for record in predefined_palettes() {
            themes.insert((record.id, record.palette.variant), record.palette);
        }
        Self { themes }
    }

    pub fn palette(&self, id: ThemeId, variant: ThemeVariant) -> Option<&ThemePalette> {
        self.themes.get(&(id, variant))
    }

    pub fn theme(&self, id: ThemeId, variant: ThemeVariant) -> Option<Theme> {
        self.palette(id, variant).map(|palette| palette.to_theme())
    }

    pub fn available(&self) -> Vec<(ThemeId, ThemeVariant, &'static str)> {
        self.themes
            .iter()
            .map(|((id, variant), palette)| (*id, *variant, palette.name))
            .collect()
    }
}

struct PaletteRecord {
    id: ThemeId,
    palette: ThemePalette,
}

fn predefined_palettes() -> Vec<PaletteRecord> {
    vec![
        PaletteRecord {
            id: ThemeId::Catppuccin,
            palette: ThemePalette {
                name: "Catppuccin Mocha",
                background: hex("1e1e2e"),
                foreground: hex("cdd6f4"),
                cursor: hex("f5e0dc"),
                selection: hex("585b70"),
                comment: hex("6c7086"),
                keyword: hex("cba6f7"),
                string: hex("a6e3a1"),
                function: hex("89b4fa"),
                variable: hex("f2cdcd"),
                accent: hex("fab387"),
                variant: ThemeVariant::Dark,
            },
        },
        PaletteRecord {
            id: ThemeId::Catppuccin,
            palette: ThemePalette {
                name: "Catppuccin Latte",
                background: hex("eff1f5"),
                foreground: hex("4c4f69"),
                cursor: hex("dc8a78"),
                selection: hex("ccd0da"),
                comment: hex("9ca0b0"),
                keyword: hex("d20f39"),
                string: hex("40a02b"),
                function: hex("1e66f5"),
                variable: hex("df8e1d"),
                accent: hex("8839ef"),
                variant: ThemeVariant::Light,
            },
        },
        PaletteRecord {
            id: ThemeId::Everblush,
            palette: ThemePalette {
                name: "Everblush",
                background: hex("141b1e"),
                foreground: hex("dadada"),
                cursor: hex("f5f5f5"),
                selection: hex("232a2d"),
                comment: hex("3e4b51"),
                keyword: hex("e57474"),
                string: hex("8ccf7e"),
                function: hex("6cbfbf"),
                variable: hex("f0a8cd"),
                accent: hex("67b0e8"),
                variant: ThemeVariant::Dark,
            },
        },
        PaletteRecord {
            id: ThemeId::Gruvbox,
            palette: ThemePalette {
                name: "Gruvbox Dark",
                background: hex("282828"),
                foreground: hex("ebdbb2"),
                cursor: hex("fbf1c7"),
                selection: hex("3c3836"),
                comment: hex("928374"),
                keyword: hex("fb4934"),
                string: hex("b8bb26"),
                function: hex("83a598"),
                variable: hex("fabd2f"),
                accent: hex("d3869b"),
                variant: ThemeVariant::Dark,
            },
        },
        PaletteRecord {
            id: ThemeId::Solarized,
            palette: ThemePalette {
                name: "Solarized Dark",
                background: hex("002b36"),
                foreground: hex("93a1a1"),
                cursor: hex("839496"),
                selection: hex("073642"),
                comment: hex("586e75"),
                keyword: hex("cb4b16"),
                string: hex("859900"),
                function: hex("268bd2"),
                variable: hex("b58900"),
                accent: hex("d33682"),
                variant: ThemeVariant::Dark,
            },
        },
        PaletteRecord {
            id: ThemeId::SolarizedLight,
            palette: ThemePalette {
                name: "Solarized Light",
                background: hex("fdf6e3"),
                foreground: hex("657b83"),
                cursor: hex("586e75"),
                selection: hex("eee8d5"),
                comment: hex("93a1a1"),
                keyword: hex("b58900"),
                string: hex("2aa198"),
                function: hex("268bd2"),
                variable: hex("cb4b16"),
                accent: hex("d33682"),
                variant: ThemeVariant::Light,
            },
        },
        PaletteRecord {
            id: ThemeId::Cyberdream,
            palette: ThemePalette {
                name: "Cyberdream",
                background: hex("0f172a"),
                foreground: hex("cbd5f5"),
                cursor: hex("f472b6"),
                selection: hex("1e293b"),
                comment: hex("475569"),
                keyword: hex("f472b6"),
                string: hex("38bdf8"),
                function: hex("a855f7"),
                variable: hex("22d3ee"),
                accent: hex("facc15"),
                variant: ThemeVariant::Dark,
            },
        },
        PaletteRecord {
            id: ThemeId::NordDark,
            palette: ThemePalette {
                name: "Nord Dark",
                background: hex("2e3440"),
                foreground: hex("d8dee9"),
                cursor: hex("eceff4"),
                selection: hex("3b4252"),
                comment: hex("4c566a"),
                keyword: hex("bf616a"),
                string: hex("a3be8c"),
                function: hex("88c0d0"),
                variable: hex("ebcb8b"),
                accent: hex("b48ead"),
                variant: ThemeVariant::Dark,
            },
        },
        PaletteRecord {
            id: ThemeId::OneDark,
            palette: ThemePalette {
                name: "One Dark",
                background: hex("282c34"),
                foreground: hex("abb2bf"),
                cursor: hex("528bff"),
                selection: hex("3e4451"),
                comment: hex("5c6370"),
                keyword: hex("c678dd"),
                string: hex("98c379"),
                function: hex("61afef"),
                variable: hex("e5c07b"),
                accent: hex("56b6c2"),
                variant: ThemeVariant::Dark,
            },
        },
        PaletteRecord {
            id: ThemeId::OneLight,
            palette: ThemePalette {
                name: "One Light",
                background: hex("fafafa"),
                foreground: hex("383a42"),
                cursor: hex("526fff"),
                selection: hex("e5e5e6"),
                comment: hex("a0a1a7"),
                keyword: hex("a626a4"),
                string: hex("50a14f"),
                function: hex("4078f2"),
                variable: hex("c18401"),
                accent: hex("0184bc"),
                variant: ThemeVariant::Light,
            },
        },
        PaletteRecord {
            id: ThemeId::Dracula,
            palette: ThemePalette {
                name: "Dracula",
                background: hex("282a36"),
                foreground: hex("f8f8f2"),
                cursor: hex("50fa7b"),
                selection: hex("44475a"),
                comment: hex("6272a4"),
                keyword: hex("ff79c6"),
                string: hex("f1fa8c"),
                function: hex("8be9fd"),
                variable: hex("bd93f9"),
                accent: hex("ffb86c"),
                variant: ThemeVariant::Dark,
            },
        },
    ]
}

fn hex(code: &str) -> Color {
    let r = u8::from_str_radix(&code[0..2], 16).unwrap();
    let g = u8::from_str_radix(&code[2..4], 16).unwrap();
    let b = u8::from_str_radix(&code[4..6], 16).unwrap();
    Color { r, g, b, a: 0xff }
}
