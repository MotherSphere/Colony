package main

import (
	"log"

	"github.com/example/colony/internal/config"
	"github.com/example/colony/internal/ui"
)

func main() {
	cfg, err := config.LoadDefault()
	if err != nil {
		log.Fatalf("failed to load configuration: %v", err)
	}

	if err := ui.Run(cfg); err != nil {
		log.Fatalf("application exited with error: %v", err)
	}
}
