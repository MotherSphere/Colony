use crate::fs::{FileNode, WorkspaceManager};
use crate::localization::LocalizationProvider;
use gtk::prelude::*;
use std::cell::RefCell;
use std::path::PathBuf;
use std::rc::Rc;
use std::sync::Arc;

const COLUMN_NAME: i32 = 0;
const COLUMN_IS_DIR: i32 = 1;
const COLUMN_PATH: i32 = 2;

pub struct FileExplorer {
    container: gtk::Box,
    tree: gtk::TreeView,
    store: gtk::TreeStore,
    on_open: Rc<RefCell<Option<Box<dyn Fn(PathBuf) + 'static>>>>,
    workspace: Arc<WorkspaceManager>,
    localization: Arc<LocalizationProvider>,
    title_label: gtk::Label,
}

impl FileExplorer {
    pub fn new(
        workspace: Arc<WorkspaceManager>,
        localization: Arc<LocalizationProvider>,
    ) -> Rc<Self> {
        let store = gtk::TreeStore::new(&[
            String::static_type(),
            bool::static_type(),
            String::static_type(),
        ]);
        let tree = gtk::TreeView::with_model(&store);
        tree.set_headers_visible(false);
        tree.set_hexpand(true);
        tree.set_vexpand(true);

        let column = gtk::TreeViewColumn::new();
        let renderer = gtk::CellRendererText::new();
        column.pack_start(&renderer, true);
        column.add_attribute(&renderer, "text", COLUMN_NAME);
        tree.append_column(&column);

        let scroller = gtk::ScrolledWindow::builder()
            .hexpand(true)
            .vexpand(true)
            .build();
        scroller.set_child(Some(&tree));

        let header = gtk::Box::new(gtk::Orientation::Horizontal, 6);
        header.add_css_class("toolbar");
        let title = gtk::Label::new(Some(&localization.text("menu_workspace")));
        title.add_css_class("title-3");
        header.append(&title);

        let refresh_button = gtk::Button::from_icon_name("view-refresh");
        refresh_button.set_tooltip_text(Some(&localization.text("menu_view")));
        header.append(&refresh_button);

        let container = gtk::Box::new(gtk::Orientation::Vertical, 0);
        container.append(&header);
        container.append(&scroller);

        let explorer = Rc::new(Self {
            container,
            tree,
            store,
            on_open: Rc::new(RefCell::new(None)),
            workspace,
            localization,
            title_label: title,
        });

        explorer.refresh();

        let on_open = explorer.on_open.clone();
        explorer
            .tree
            .connect_row_activated(move |tree, path, _col| {
                if let Some(model) = tree.model() {
                    if let Some(iter) = model.iter(path) {
                        let is_dir = model
                            .value(&iter, COLUMN_IS_DIR)
                            .get::<bool>()
                            .unwrap_or(false);
                        if is_dir {
                            if tree.row_expanded(path) {
                                tree.collapse_row(path);
                            } else {
                                tree.expand_row(path, false);
                            }
                        } else {
                            let value = model.value(&iter, COLUMN_PATH);
                            if let Ok(Some(path)) = value.get::<Option<String>>() {
                                if let Some(callback) = on_open.borrow().as_ref() {
                                    callback(PathBuf::from(path));
                                }
                            }
                        }
                    }
                }
            });

        let explorer_clone = explorer.clone();
        refresh_button.connect_clicked(move |_| {
            explorer_clone.refresh();
        });

        explorer
    }

    pub fn widget(&self) -> &gtk::Box {
        &self.container
    }

    pub fn connect_open<F>(&self, callback: F)
    where
        F: Fn(PathBuf) + 'static,
    {
        *self.on_open.borrow_mut() = Some(Box::new(callback));
    }

    pub fn refresh(&self) {
        self.store.clear();
        for node in self.workspace.tree() {
            self.append_node(None, &node);
        }
    }

    pub fn refresh_texts(&self) {
        self.title_label
            .set_text(&self.localization.text("menu_workspace"));
    }

    fn append_node(&self, parent: Option<&gtk::TreeIter>, node: &FileNode) {
        let iter = self.store.append(parent);
        self.store.set(
            &iter,
            &[
                (COLUMN_NAME, &node.name),
                (COLUMN_IS_DIR, &node.is_dir),
                (COLUMN_PATH, &node.path.display().to_string()),
            ],
        );

        if node.is_dir {
            for child in &node.children {
                self.append_node(Some(&iter), child);
            }
        }
    }
}
