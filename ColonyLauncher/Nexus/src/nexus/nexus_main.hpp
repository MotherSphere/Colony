#pragma once

#include <SDL2/SDL.h>

namespace nexus
{
struct NexusResult
{
    bool propagateQuit = false;
};

/// Launches the Nexus window as a secondary module.
[[nodiscard]] NexusResult LaunchStandalone();
} // namespace nexus

