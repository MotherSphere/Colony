#pragma once

#include <filesystem>

struct SDL_Renderer;
struct SDL_Window;

namespace colony
{
class LocalizationManager;
}

namespace colony::preferences
{
struct Preferences;
}

namespace colony::programs
{

//! LaunchContext provides shared application state that program modules can
//! use when being instantiated from the Colony shell.
struct LaunchContext
{
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    LocalizationManager* localization = nullptr;
    preferences::Preferences* preferences = nullptr;
    std::filesystem::path preferencesPath{};
    std::filesystem::path contentRoot{};
};

//! Common interface implemented by every launchable program module.
class LaunchContract
{
  public:
    virtual ~LaunchContract() = default;

    //! Bootstraps the program module using the provided launch context. The
    //! implementation may cache the context to continue interacting with
    //! shared services (localization, preferences, etc.).
    virtual void Launch(const LaunchContext& context) = 0;
};

} // namespace colony::programs

