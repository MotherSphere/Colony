# Colony Workspace

This repository now organizes the Colony Launcher application as the core project with the Orbital Arcade module contained inside the same workspace. The main build files live under `ColonyLauncher/`, and the root CMake file simply delegates to that directory.

## Layout
- `ColonyLauncher/` – main launcher sources, assets, tests, and third-party dependencies.
- `ColonyLauncher/OrbitalArcade/` – isolated module for the Orbital Arcade window, with its own sources and assets.
- `CMakeLists.txt` – root wrapper that adds the `ColonyLauncher` project directory.

## README naming convention
For every directory created inside `ColonyLauncher/`, add an overview file named after the directory itself (for example, `Colon
yLauncher/ColonyLauncher.md`, `ColonyLauncher/OrbitalArcade/OrbitalArcade.md`, or `ColonyLauncher/docs/docs.md`). Apply this ru
le to any future folders so each module’s entry point is easy to discover.

Run CMake from the repository root as usual:

```bash
cmake -S . -B build
cmake --build build
```
