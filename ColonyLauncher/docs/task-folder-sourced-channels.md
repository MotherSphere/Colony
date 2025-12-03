# Task: Folder-sourced channels for Nexus

## Background
Currently Nexus populates its "Applications", "Programs", "Addons", and "Games" sections from the single JSON file `ColonyLauncher/assets/content/app_content.json`. Channels are static lists of identifiers that map to view definitions inside the same file; no filesystem discovery happens at runtime.

## Problem statement
Product wants each section to mirror dedicated folders on disk:
- Applications → `Applications/`
- Programs → `Programs/`
- Addons → `Addons/`
- Games → `Games/`

Dropping a folder into one of these locations should automatically create a corresponding entry in the matching section. Each entry must allow selecting a launchable file from inside that folder.

## Requirements
1. **Folder mapping**: Define canonical directories for all four sections, either relative to the launcher install root or via configurable paths.
2. **Discovery pass**: On startup (and ideally refreshable), scan each mapped directory and create in-memory channel listings based on the folders present. Avoid regressions if the folders are missing or empty.
3. **Metadata source**: Decide how titles, descriptions, colors, and button labels are populated for discovered items. Options include:
   - reading manifest files shipped inside each folder;
   - falling back to folder names with sensible defaults;
   - converting manifests into the existing `views`/`channels` shape to preserve downstream expectations.
4. **Launch configuration**: For each discovered folder, identify valid launch targets (executables, scripts, or a manifest-provided command). Ensure the UI presents a selectable action that resolves to the correct path.
5. **Validation**: Extend or replace the current JSON content validation so dynamically generated channels remain well-formed (no missing views, unique IDs, safe paths).
6. **Error handling & UX**: Provide user-facing feedback for empty folders, missing manifests, or invalid launch targets. Preserve existing content loading behavior if discovery fails.
7. **Tests**: Add automated coverage for discovery, manifest parsing (if added), validation, and UI population across all four sections.

## Deliverables
- Implementation that populates Nexus sections from the filesystem rather than hardcoded JSON.
- Updated documentation explaining directory layout, manifest format (if applicable), and refresh behavior.
- Automated tests proving discovery and launch mapping work for Applications, Programs, Addons, and Games.
