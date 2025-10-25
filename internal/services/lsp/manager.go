package lsp

import (
	"bufio"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/url"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"sync"
	"sync/atomic"

	"github.com/example/colony/internal/config"
)

type Manager struct {
	mu      sync.Mutex
	clients map[string]*Client
}

type Client struct {
	language string
	root     string
	cfg      config.LSPServerConfig
	cmd      *exec.Cmd
	stdin    io.WriteCloser
	stdout   io.ReadCloser
	cancel   context.CancelFunc
	seq      atomic.Int64
	mu       sync.Mutex
}

type requestMessage struct {
	JSONRPC string      `json:"jsonrpc"`
	ID      int64       `json:"id,omitempty"`
	Method  string      `json:"method"`
	Params  interface{} `json:"params,omitempty"`
}

type responseMessage struct {
	JSONRPC string          `json:"jsonrpc"`
	ID      int64           `json:"id,omitempty"`
	Result  json.RawMessage `json:"result,omitempty"`
	Error   *struct {
		Code    int    `json:"code"`
		Message string `json:"message"`
	} `json:"error,omitempty"`
}

type notificationMessage struct {
	JSONRPC string      `json:"jsonrpc"`
	Method  string      `json:"method"`
	Params  interface{} `json:"params,omitempty"`
}

func NewManager() *Manager {
	return &Manager{clients: make(map[string]*Client)}
}

func (m *Manager) Ensure(language string, cfg config.LSPServerConfig, root string) (*Client, error) {
	m.mu.Lock()
	if client, ok := m.clients[language]; ok {
		m.mu.Unlock()
		return client, nil
	}
	m.mu.Unlock()

	client, err := startClient(language, cfg, root)
	if err != nil {
		return nil, err
	}

	m.mu.Lock()
	m.clients[language] = client
	m.mu.Unlock()
	return client, nil
}

func (m *Manager) Shutdown() {
	m.mu.Lock()
	defer m.mu.Unlock()
	for key, client := range m.clients {
		client.Shutdown()
		delete(m.clients, key)
	}
}

func startClient(language string, cfg config.LSPServerConfig, root string) (*Client, error) {
	ctx, cancel := context.WithCancel(context.Background())
	cmd := exec.CommandContext(ctx, cfg.Command, cfg.Args...)
	stdout, err := cmd.StdoutPipe()
	if err != nil {
		cancel()
		return nil, err
	}
	stdin, err := cmd.StdinPipe()
	if err != nil {
		cancel()
		return nil, err
	}
	cmd.Stderr = os.Stderr
	if err := cmd.Start(); err != nil {
		cancel()
		return nil, err
	}

	client := &Client{
		language: language,
		root:     root,
		cfg:      cfg,
		cmd:      cmd,
		stdin:    stdin,
		stdout:   stdout,
		cancel:   cancel,
	}

	go client.readLoop()
	if err := client.initialize(); err != nil {
		client.Shutdown()
		return nil, err
	}
	return client, nil
}

func (c *Client) initialize() error {
	rootURI := ""
	if c.root != "" {
		abs, _ := filepath.Abs(c.root)
		rootURI = pathToURI(abs)
	}
	params := map[string]interface{}{
		"processId": os.Getpid(),
		"rootUri":   rootURI,
		"capabilities": map[string]interface{}{
			"textDocument": map[string]interface{}{
				"synchronization": map[string]bool{
					"didSave": true,
				},
				"completion": map[string]interface{}{
					"completionItem": map[string]bool{"snippetSupport": true},
				},
				"hover": map[string]bool{"dynamicRegistration": false},
			},
		},
		"workspaceFolders": []map[string]string{{"uri": rootURI, "name": filepath.Base(c.root)}},
	}

	if err := c.sendRequest("initialize", params); err != nil {
		return err
	}
	return c.sendNotification("initialized", map[string]interface{}{})
}

func (c *Client) sendRequest(method string, params interface{}) error {
	id := c.seq.Add(1)
	msg := requestMessage{JSONRPC: "2.0", ID: id, Method: method, Params: params}
	return c.send(msg)
}

func (c *Client) sendNotification(method string, params interface{}) error {
	msg := notificationMessage{JSONRPC: "2.0", Method: method, Params: params}
	return c.send(msg)
}

func (c *Client) send(payload interface{}) error {
	body, err := json.Marshal(payload)
	if err != nil {
		return err
	}
	header := fmt.Sprintf("Content-Length: %d\r\n\r\n", len(body))
	c.mu.Lock()
	defer c.mu.Unlock()
	if _, err := io.WriteString(c.stdin, header); err != nil {
		return err
	}
	if _, err := c.stdin.Write(body); err != nil {
		return err
	}
	return nil
}

func (c *Client) readLoop() {
	reader := bufio.NewReader(c.stdout)
	for {
		header, err := reader.ReadString('\n')
		if err != nil {
			return
		}
		header = strings.TrimSpace(header)
		if header == "" {
			continue
		}
		headers := []string{header}
		for {
			line, err := reader.ReadString('\n')
			if err != nil {
				return
			}
			trimmed := strings.TrimSpace(line)
			if trimmed == "" {
				break
			}
			headers = append(headers, trimmed)
		}
		length := 0
		for _, h := range headers {
			lower := strings.ToLower(h)
			if strings.HasPrefix(lower, "content-length:") {
				fmt.Sscanf(lower, "content-length: %d", &length)
				break
			}
		}
		if length == 0 {
			continue
		}
		buf := make([]byte, length)
		if _, err := io.ReadFull(reader, buf); err != nil {
			return
		}
		// Future: dispatch to diagnostics view.
	}
}

func (c *Client) DidOpen(path string, language string, text string) error {
	params := map[string]interface{}{
		"textDocument": map[string]interface{}{
			"uri":        pathToURI(path),
			"languageId": language,
			"version":    1,
			"text":       text,
		},
	}
	return c.sendNotification("textDocument/didOpen", params)
}

func (c *Client) DidChange(path string, version int, text string) error {
	params := map[string]interface{}{
		"textDocument": map[string]interface{}{
			"uri":     pathToURI(path),
			"version": version,
		},
		"contentChanges": []map[string]string{{"text": text}},
	}
	return c.sendNotification("textDocument/didChange", params)
}

func (c *Client) DidSave(path string) error {
	params := map[string]interface{}{
		"textDocument": map[string]interface{}{
			"uri": pathToURI(path),
		},
	}
	return c.sendNotification("textDocument/didSave", params)
}

func (c *Client) Shutdown() {
	c.sendRequest("shutdown", nil) // ignore error
	c.sendNotification("exit", nil)
	if c.cancel != nil {
		c.cancel()
	}
	if c.cmd != nil {
		_ = c.cmd.Wait()
	}
}

func pathToURI(path string) string {
	if path == "" {
		return ""
	}
	abs := filepath.ToSlash(path)
	if !strings.HasPrefix(abs, "/") {
		abs = "/" + abs
	}
	u := url.URL{Scheme: "file", Path: abs}
	return u.String()
}
