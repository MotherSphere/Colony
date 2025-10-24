use crate::config::AppConfig;
use crate::localization::LocalizationProvider;
use crate::theme::{ThemeRegistry, ThemeVariant};
use gtk::prelude::*;
use gtk::{self, pango};
use parking_lot::RwLock;
use std::sync::Arc;

pub struct TopBar {
    container: gtk::Box,
    pub new_button: gtk::Button,
    pub open_button: gtk::Button,
    pub save_button: gtk::Button,
    pub toggle_explorer: gtk::ToggleButton,
    pub theme_selector: gtk::ComboBoxText,
    pub locale_selector: gtk::ComboBoxText,
    pub font_button: gtk::FontDialogButton,
    theme_label: gtk::Label,
    locale_label: gtk::Label,
}

impl TopBar {
    pub fn new(
        config: Arc<RwLock<AppConfig>>,
        themes: Arc<ThemeRegistry>,
        localization: Arc<LocalizationProvider>,
    ) -> Self {
        let container = gtk::Box::new(gtk::Orientation::Horizontal, 8);
        container.add_css_class("toolbar");
        container.set_spacing(8);

        let new_button = gtk::Button::from_icon_name("document-new");
        new_button.set_tooltip_text(Some(&localization.text("menu_file")));
        container.append(&new_button);

        let open_button = gtk::Button::from_icon_name("document-open");
        open_button.set_tooltip_text(Some(&localization.text("menu_file")));
        container.append(&open_button);

        let save_button = gtk::Button::from_icon_name("document-save");
        save_button.set_tooltip_text(Some(&localization.text("menu_file")));
        container.append(&save_button);

        let toggle_explorer =
            gtk::ToggleButton::with_label(&localization.text("menu_toggle_explorer"));
        toggle_explorer.set_active(true);
        container.append(&toggle_explorer);

        let theme_label = gtk::Label::new(Some(&localization.text("menu_theme")));
        container.append(&theme_label);
        let theme_selector = gtk::ComboBoxText::new();
        let config_values = config.read().theme.clone();
        for (id, variant, name) in themes.available() {
            theme_selector.append(Some(&format!("{:?}:{:?}", id, variant)), name);
            if id == config_values.active_theme && variant == config_values.variant {
                theme_selector.set_active_id(Some(&format!("{:?}:{:?}", id, variant)));
            }
        }
        container.append(&theme_selector);

        let locale_label = gtk::Label::new(Some(&localization.text("menu_language")));
        container.append(&locale_label);
        let locale_selector = gtk::ComboBoxText::new();
        for locale in localization.available_locales() {
            let id = locale.to_string();
            locale_selector.append(Some(&id), &id);
        }
        locale_selector.set_active_id(Some(&localization.active_locale().to_string()));
        container.append(&locale_selector);

        let font_button = gtk::FontDialogButton::new();
        font_button.set_dialog_title(Some(&localization.text("menu_settings")));
        font_button.set_modal(true);
        let font_cfg = config.read().font.clone();
        let desc =
            pango::FontDescription::from_string(&format!("{} {}", font_cfg.family, font_cfg.size));
        font_button.set_font_desc(&desc);
        container.append(&font_button);

        Self {
            container,
            new_button,
            open_button,
            save_button,
            toggle_explorer,
            theme_selector,
            locale_selector,
            font_button,
            theme_label,
            locale_label,
        }
    }

    pub fn widget(&self) -> &gtk::Box {
        &self.container
    }

    pub fn refresh_texts(&self, localization: &LocalizationProvider) {
        self.new_button
            .set_tooltip_text(Some(&localization.text("menu_file")));
        self.open_button
            .set_tooltip_text(Some(&localization.text("menu_file")));
        self.save_button
            .set_tooltip_text(Some(&localization.text("menu_file")));
        self.toggle_explorer
            .set_label(&localization.text("menu_toggle_explorer"));
        self.theme_label.set_text(&localization.text("menu_theme"));
        self.locale_label
            .set_text(&localization.text("menu_language"));
        self.font_button
            .set_dialog_title(Some(&localization.text("menu_settings")));
    }
}
