package config

import (
	"errors"
	"os"
	"path/filepath"

	"gopkg.in/yaml.v3"
)

type FontConfig struct {
	Family string `yaml:"family"`
	Size   int    `yaml:"size"`
}

type Config struct {
	Workspace    string                     `yaml:"workspace"`
	Locale       string                     `yaml:"locale"`
	Theme        string                     `yaml:"theme"`
	Font         FontConfig                 `yaml:"font"`
	LintCommands map[string][]string        `yaml:"lintCommands"`
	LSPServers   map[string]LSPServerConfig `yaml:"lspServers"`
}

type LSPServerConfig struct {
	Command string   `yaml:"command"`
	Args    []string `yaml:"args"`
}

func LoadDefault() (*Config, error) {
	cfg := defaultConfig()
	configPath := filepath.Join("config", "colony.yaml")
	if _, err := os.Stat(configPath); errors.Is(err, os.ErrNotExist) {
		return cfg, nil
	}

	data, err := os.ReadFile(configPath)
	if err != nil {
		return nil, err
	}

	if err := yaml.Unmarshal(data, cfg); err != nil {
		return nil, err
	}

	if cfg.Locale == "" {
		cfg.Locale = "en"
	}

	return cfg, nil
}

func defaultConfig() *Config {
	return &Config{
		Workspace: ".",
		Locale:    "en",
		Theme:     "catppuccin-mocha",
		Font: FontConfig{
			Family: "Fira Code",
			Size:   12,
		},
		LintCommands: map[string][]string{
			"go":         []string{"golangci-lint", "run"},
			"python":     []string{"ruff"},
			"javascript": []string{"eslint", "."},
			"typescript": []string{"eslint", "."},
		},
		LSPServers: map[string]LSPServerConfig{
			"go": {
				Command: "gopls",
			},
			"python": {
				Command: "pyright-langserver",
				Args:    []string{"--stdio"},
			},
			"javascript": {
				Command: "typescript-language-server",
				Args:    []string{"--stdio"},
			},
			"typescript": {
				Command: "typescript-language-server",
				Args:    []string{"--stdio"},
			},
		},
	}
}
