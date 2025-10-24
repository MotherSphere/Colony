use crate::config::{AppConfig, LspServerConfig};
use anyhow::{Context, Result};
use async_channel::Sender;
use lsp_types::notification::{DidOpenTextDocument, DidSaveTextDocument};
use lsp_types::request::HoverRequest;
use lsp_types::{
    DidOpenTextDocumentParams, DidSaveTextDocumentParams, HoverParams, InitializeParams,
    InitializedParams, Position, TextDocumentIdentifier, TextDocumentItem, Url,
    WorkDoneProgressParams,
};
use parking_lot::{Mutex, RwLock};
use serde_json::json;
use std::collections::HashMap;
use std::path::{Path, PathBuf};
use std::sync::Arc;
use tokio::io::{AsyncBufReadExt, AsyncReadExt, AsyncWriteExt, BufReader};
use tokio::process::Command;
use tokio::runtime::Runtime;
use tokio::task::JoinHandle;

pub struct LspCoordinator {
    runtime: Arc<Runtime>,
    config: Arc<RwLock<AppConfig>>,
    sessions: Arc<Mutex<HashMap<String, Arc<LspSession>>>>,
}

impl LspCoordinator {
    pub fn new(runtime: Arc<Runtime>, config: Arc<RwLock<AppConfig>>) -> Self {
        Self {
            runtime,
            config,
            sessions: Arc::new(Mutex::new(HashMap::new())),
        }
    }

    pub fn ensure_session(&self, language: &str, root: &Path) {
        if self.sessions.lock().contains_key(language) {
            return;
        }
        if let Some(server) = self.config.read().lsp.servers.get(language).cloned() {
            let runtime = self.runtime.clone();
            let sessions = self.sessions.clone();
            let language = language.to_string();
            let root = root.to_path_buf();
            runtime.spawn(async move {
                match start_session(language.clone(), root, server).await {
                    Ok(session) => {
                        sessions.lock().insert(language, session);
                    }
                    Err(err) => {
                        log::error!("failed to start language server: {err:?}");
                    }
                }
            });
        }
    }

    pub fn document_opened(&self, language: &str, uri: Url, text: String) {
        if let Some(session) = self.sessions.lock().get(language).cloned() {
            let params = DidOpenTextDocumentParams {
                text_document: TextDocumentItem {
                    uri,
                    language_id: language.to_string(),
                    version: 1,
                    text,
                },
            };
            session.send_notification(DidOpenTextDocument::METHOD, params);
        }
    }

    pub fn document_saved(&self, language: &str, uri: Url) {
        if let Some(session) = self.sessions.lock().get(language).cloned() {
            let params = DidSaveTextDocumentParams {
                text_document: TextDocumentIdentifier { uri },
                text: None,
            };
            session.send_notification(DidSaveTextDocument::METHOD, params);
        }
    }

    pub fn request_hover(&self, language: &str, uri: Url, position: Position) {
        if let Some(session) = self.sessions.lock().get(language).cloned() {
            let params = HoverParams {
                text_document_position_params: lsp_types::TextDocumentPositionParams {
                    text_document: TextDocumentIdentifier { uri },
                    position,
                },
                work_done_progress_params: WorkDoneProgressParams::default(),
                partial_result_params: lsp_types::PartialResultParams::default(),
            };
            session.send_request(HoverRequest::METHOD, params);
        }
    }
}

struct LspSession {
    language: String,
    sender: Sender<serde_json::Value>,
    _reader: JoinHandle<()>,
    _writer: JoinHandle<()>,
    _child_watcher: JoinHandle<()>,
}

impl LspSession {
    fn send_notification<P: serde::Serialize>(&self, method: &str, params: P) {
        let message = json!({
            "jsonrpc": "2.0",
            "method": method,
            "params": params,
        });
        if let Err(err) = self.sender.try_send(message) {
            log::warn!("failed to send notification {method}: {err}");
        }
    }

    fn send_request<P: serde::Serialize>(&self, method: &str, params: P) {
        let message = json!({
            "jsonrpc": "2.0",
            "id": chrono::Utc::now().timestamp_millis(),
            "method": method,
            "params": params,
        });
        if let Err(err) = self.sender.try_send(message) {
            log::warn!("failed to send request {method}: {err}");
        }
    }
}

async fn start_session(
    language: String,
    root: PathBuf,
    server: LspServerConfig,
) -> Result<Arc<LspSession>> {
    let mut command = Command::new(&server.command);
    command.args(&server.args);
    command.stdin(std::process::Stdio::piped());
    command.stdout(std::process::Stdio::piped());
    command.stderr(std::process::Stdio::piped());
    command.current_dir(&root);

    let mut child = command
        .spawn()
        .with_context(|| format!("failed to spawn LSP server {}", server.command))?;

    let stdin = child.stdin.take().context("missing stdin for LSP server")?;
    let stdout = child
        .stdout
        .take()
        .context("missing stdout for LSP server")?;
    let stderr = child.stderr.take();

    let (tx, rx) = async_channel::unbounded::<serde_json::Value>();

    let writer_handle = tokio::spawn(async move {
        let mut stdin = stdin;
        while let Ok(msg) = rx.recv().await {
            if let Err(err) = write_lsp_message(&mut stdin, &msg).await {
                log::error!("failed to write LSP message: {err}");
                break;
            }
        }
    });

    let reader_handle = tokio::spawn(async move {
        let mut reader = BufReader::new(stdout);
        loop {
            match read_lsp_message(&mut reader).await {
                Ok(Some(message)) => {
                    log::debug!("[{language}] <- {message}");
                }
                Ok(None) => break,
                Err(err) => {
                    log::error!("failed to read LSP message: {err}");
                    break;
                }
            }
        }
    });

    if let Some(mut stderr) = stderr {
        tokio::spawn(async move {
            let mut reader = BufReader::new(stderr);
            let mut line = String::new();
            loop {
                line.clear();
                match reader.read_line(&mut line).await {
                    Ok(0) => break,
                    Ok(_) => {
                        log::warn!("LSP stderr: {line}");
                    }
                    Err(err) => {
                        log::error!("stderr read error: {err}");
                        break;
                    }
                }
            }
        });
    }

    let child_handle = tokio::spawn(async move {
        if let Err(err) = child.wait().await {
            log::error!("LSP server exited with error: {err}");
        }
    });

    let session = Arc::new(LspSession {
        language: language.clone(),
        sender: tx.clone(),
        _reader: reader_handle,
        _writer: writer_handle,
        _child_watcher: child_handle,
    });

    initialize_session(session.clone(), &language, &root, server).await?;

    Ok(session)
}

async fn initialize_session(
    session: Arc<LspSession>,
    language: &str,
    root: &Path,
    server: LspServerConfig,
) -> Result<()> {
    let root_uri = Url::from_directory_path(root).unwrap_or_else(|_| Url::parse("file:/").unwrap());
    let initialize = InitializeParams {
        process_id: Some(std::process::id()),
        root_path: None,
        root_uri: Some(root_uri.clone()),
        initialization_options: Some(server.initialization_options.clone()),
        capabilities: lsp_types::ClientCapabilities::default(),
        trace: None,
        workspace_folders: None,
        client_info: Some(lsp_types::ClientInfo {
            name: "Colony Editor".into(),
            version: Some(env!("CARGO_PKG_VERSION").into()),
        }),
        locale: Some(language.into()),
        ..Default::default()
    };

    session.send_request("initialize", initialize);
    session.send_notification("initialized", InitializedParams {});
    Ok(())
}

async fn write_lsp_message(
    stdin: &mut tokio::process::ChildStdin,
    value: &serde_json::Value,
) -> Result<()> {
    let payload = serde_json::to_string(value)?;
    let header = format!("Content-Length: {}\r\n\r\n", payload.len());
    stdin.write_all(header.as_bytes()).await?;
    stdin.write_all(payload.as_bytes()).await?;
    stdin.flush().await?;
    Ok(())
}

async fn read_lsp_message(
    reader: &mut BufReader<tokio::process::ChildStdout>,
) -> Result<Option<String>> {
    let mut header = String::new();
    let mut content_length = None;

    loop {
        header.clear();
        let bytes = reader.read_line(&mut header).await?;
        if bytes == 0 {
            return Ok(None);
        }
        let line = header.trim();
        if line.is_empty() {
            break;
        }
        if let Some((name, value)) = line.split_once(':') {
            if name.eq_ignore_ascii_case("Content-Length") {
                content_length = value.trim().parse::<usize>().ok();
            }
        }
    }

    let length = match content_length {
        Some(length) => length,
        None => return Ok(None),
    };

    let mut buffer = vec![0u8; length];
    reader.read_exact(&mut buffer).await?;
    let message = String::from_utf8(buffer)?;
    Ok(Some(message))
}
