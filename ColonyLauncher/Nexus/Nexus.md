# Nexus

This directory contains the standalone Nexus window. All Nexus-specific source code and assets live here so the main launcher remains focused on navigation and system-wide controls.

- `src/` – source files for the Nexus module.
- `assets/` – audio, images, and fonts scoped to Nexus.
- `nexus_main.cpp` entry point lives under `src/nexus/` and is built as the `nexus` library.
- Environment variable `COLONY_MODULE_NAME` – overrides the window title so the launcher can brand whatever module is being started without relying on a hard-coded name.
