#pragma once

#include <SDL2/SDL.h>

namespace nexus
{
struct ArcadeResult
{
    bool propagateQuit = false;
};

/// Launches the Nexus window as a secondary module.
[[nodiscard]] ArcadeResult LaunchStandalone();
} // namespace nexus

