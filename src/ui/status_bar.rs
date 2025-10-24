use crate::localization::LocalizationProvider;
use gtk::prelude::*;
use std::sync::Arc;

#[derive(Clone)]
pub struct StatusBar {
    container: gtk::Box,
    status_label: gtk::Label,
}

impl StatusBar {
    pub fn new(localization: Arc<LocalizationProvider>) -> Self {
        let container = gtk::Box::new(gtk::Orientation::Horizontal, 12);
        container.add_css_class("statusbar");
        container.set_spacing(12);

        let status_label = gtk::Label::new(Some(&localization.text("status_ready")));
        status_label.set_xalign(0.0);
        container.append(&status_label);

        Self {
            container,
            status_label,
        }
    }

    pub fn widget(&self) -> &gtk::Box {
        &self.container
    }

    pub fn set_status(&self, text: &str) {
        self.status_label.set_text(text);
    }
}
