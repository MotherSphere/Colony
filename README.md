# Colony

Colony is a GTK-based desktop code editor written in Go. It provides a file explorer, syntax-highlighted editor, lint feedback, and integrations with Language Server Protocol (LSP) servers.

## Prerequisites

Before running the application make sure the following tools are installed:

- [Go](https://go.dev/dl/) 1.24 or newer
- GTK 2.24+ development libraries (for Debian/Ubuntu: `sudo apt install libgtk2.0-dev`)
- Linting tools or LSP servers you plan to use (optional). The defaults expect tools such as `golangci-lint`, `ruff`, and `typescript-language-server` to be available in your `PATH` if you enable those languages.

## Getting started

Clone the repository and download Go module dependencies:

```bash
git clone https://github.com/example/colony.git
cd colony
go mod download
```

## Running the application

You can run the editor directly with `go run`:

```bash
go run ./cmd/colony
```

Alternatively, build a binary and run it separately:

```bash
go build -o colony ./cmd/colony
./colony
```

The application loads its configuration from `config/colony.yaml` if the file exists. You can create this file to override defaults such as the workspace directory, theme, fonts, or external tooling commands. Without the file, Colony uses built-in defaults.

## Troubleshooting

If GTK cannot open a display, ensure you are running in a desktop session with access to an X11 or Wayland server. When LSP or lint commands fail, check that the corresponding executables are installed and reachable from your `PATH`.
