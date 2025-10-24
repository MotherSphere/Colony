use crate::config::AppConfig;
use notify::{RecommendedWatcher, RecursiveMode, Watcher};
use parking_lot::RwLock;
use std::fs;
use std::path::{Path, PathBuf};
use std::sync::Arc;
use std::time::Duration;

#[derive(Debug, Clone)]
pub struct FileNode {
    pub name: String,
    pub path: PathBuf,
    pub is_dir: bool,
    pub children: Vec<FileNode>,
}

impl FileNode {
    pub fn new(path: PathBuf) -> Self {
        let name = path
            .file_name()
            .map(|name| name.to_string_lossy().to_string())
            .unwrap_or_else(|| path.display().to_string());
        let is_dir = path.is_dir();
        Self {
            name,
            path,
            is_dir,
            children: Vec::new(),
        }
    }
}

pub struct WorkspaceManager {
    config: Arc<RwLock<AppConfig>>,
    root: RwLock<Option<PathBuf>>,
    tree: RwLock<Vec<FileNode>>,
    #[allow(dead_code)]
    watcher: RwLock<Option<RecommendedWatcher>>,
}

impl WorkspaceManager {
    pub fn new(config: Arc<RwLock<AppConfig>>) -> Self {
        let root = config.read().workspace.roots.first().cloned();
        let manager = Self {
            config,
            root: RwLock::new(root),
            tree: RwLock::new(Vec::new()),
            watcher: RwLock::new(None),
        };
        manager.refresh_tree();
        manager
    }

    pub fn set_workspace_root(&self, path: PathBuf) {
        *self.root.write() = Some(path);
        self.refresh_tree();
    }

    pub fn current_root(&self) -> Option<PathBuf> {
        self.root.read().clone()
    }

    pub fn tree(&self) -> Vec<FileNode> {
        self.tree.read().clone()
    }

    fn refresh_tree(&self) {
        let root = match self.current_root() {
            Some(root) => root,
            None => return,
        };
        let mut nodes = Vec::new();
        for entry in fs::read_dir(&root).into_iter().flatten() {
            if let Ok(entry) = entry {
                let path = entry.path();
                nodes.push(Self::build_node(&path));
            }
        }
        nodes.sort_by(|a, b| a.name.to_lowercase().cmp(&b.name.to_lowercase()));
        *self.tree.write() = nodes;
        self.setup_watcher();
    }

    fn build_node(path: &Path) -> FileNode {
        let mut node = FileNode::new(path.to_path_buf());
        if path.is_dir() {
            let mut children = Vec::new();
            for entry in fs::read_dir(path).into_iter().flatten() {
                if let Ok(entry) = entry {
                    let child_path = entry.path();
                    children.push(Self::build_node(&child_path));
                }
            }
            children.sort_by(|a, b| a.name.to_lowercase().cmp(&b.name.to_lowercase()));
            node.children = children;
        }
        node
    }

    fn setup_watcher(&self) {
        let root = match self.current_root() {
            Some(root) => root,
            None => return,
        };
        let mut watcher = match RecommendedWatcher::new(
            move |res| {
                if let Ok(event) = res {
                    log::debug!("fs event: {:?}", event);
                }
            },
            notify::Config::default().with_poll_interval(Duration::from_secs(2)),
        ) {
            Ok(watcher) => watcher,
            Err(err) => {
                log::warn!("failed to create filesystem watcher: {err}");
                return;
            }
        };

        if let Err(err) = watcher.watch(&root, RecursiveMode::Recursive) {
            log::warn!("failed to watch workspace: {err}");
        }

        *self.watcher.write() = Some(watcher);
    }
}
