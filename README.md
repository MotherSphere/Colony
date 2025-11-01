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

## Next steps

- Add new source files under `src/` and register them in `CMakeLists.txt`.
- Use SDL2 (or integrate another GUI framework alongside SDL) to build your multimedia player, text editor, and other tools inside this shell window.
- Replace the render loop with your own scene graph, widget system, or layout engine as your project grows.
