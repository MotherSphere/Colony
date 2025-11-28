#pragma once

#include <SDL2/SDL.h>

namespace orbital_arcade
{
struct ArcadeResult
{
    bool propagateQuit = false;
};

/// Launches the Orbital Arcade window as a secondary module.
[[nodiscard]] ArcadeResult LaunchStandalone();
} // namespace orbital_arcade

