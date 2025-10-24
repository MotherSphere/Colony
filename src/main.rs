mod app;
mod config;
mod fs;
mod localization;
mod lsp;
mod theme;
mod ui;

use anyhow::Result;
use app::ColonyEditor;
use tokio::runtime::Runtime;

fn main() -> Result<()> {
    env_logger::init();
    let runtime = Runtime::new()?;
    let editor = ColonyEditor::new(runtime)?;
    editor.run();
    Ok(())
}
