# Ecosystem Application Skeleton

This repository contains a minimal C++/CMake project that opens a blank, resizable window using [SDL2](https://www.libsdl.org/). It gives you a clean canvas where you can begin integrating the different tools that will make up your "ecosystem application" (media player, text editor, etc.).

## Prerequisites

You need a working C++20 compiler, CMake (3.16 or newer), and the development headers for SDL2. SDL2 can be installed through the package manager on most platforms:

- **Ubuntu/Debian**
  ```bash
  sudo apt-get update
  sudo apt-get install build-essential cmake libsdl2-dev
  ```
- **Fedora**
  ```bash
  sudo dnf install cmake gcc-c++ SDL2-devel
  ```
- **Arch Linux**
  ```bash
  sudo pacman -S cmake sdl2
  ```
- **macOS (Homebrew)**
  ```bash
  brew install cmake sdl2
  ```
- **Windows (vcpkg)**
  ```powershell
  vcpkg install sdl2
  ```
  Then configure CMake with `-DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake`.

## Building

```bash
cmake -S . -B build
cmake --build build
```

The resulting executable (`ecosystem_app`) will be located in the `build` directory. Run it to display the blank application window:

```bash
./build/ecosystem_app
```

Close the window to exit the application.

### Fonts

The UI uses the **DejaVu Sans** typeface. At launch the application verifies whether the font is available on your system. If it
can be found in a common location, it is copied into `assets/fonts/DejaVuSans.ttf`; otherwise the font is downloaded on demand
from the [official DejaVu repository](https://github.com/dejavu-fonts/dejavu-fonts). If you prefer to use a different font, set
the `COLONY_FONT_PATH` environment variable to a TTF file before launching the executable:

```bash
COLONY_FONT_PATH=/path/to/your/font.ttf ./build/ecosystem_app
```

If the environment variable is unset and the automatic lookup or download fails (for example, because `curl` is unavailable),
place `DejaVuSans.ttf` under `assets/fonts/` manually.

## Next steps

- Add new source files under `src/` and register them in `CMakeLists.txt`.
- Use SDL2 (or integrate another GUI framework alongside SDL) to build your multimedia player, text editor, and other tools inside this shell window.
- Replace the render loop with your own scene graph, widget system, or layout engine as your project grows.
