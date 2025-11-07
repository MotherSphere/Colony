#include "ArchiveApp.hpp"

#include <SDL.h>
#include <SDL_ttf.h>

#include <stdexcept>

#include "core/localization_manager.hpp"
#include "utils/preferences.hpp"

namespace colony::programs::archive {

namespace {
void ensure_sdl_is_ready()
{
    if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
        throw std::runtime_error{"ArchiveApp requires the SDL video subsystem to be initialized"};
    }
    if (TTF_WasInit() == 0) {
        throw std::runtime_error{"ArchiveApp requires the SDL_ttf subsystem to be initialized"};
    }
}
} // namespace

ArchiveApp::ArchiveApp() = default;
ArchiveApp::~ArchiveApp() = default;

void ArchiveApp::launch()
{
    if (initialized_) {
        return;
    }

    ensure_sdl_is_ready();
    initialize_services();
    configure_state_machine();

    initialized_ = true;

    // TODO: Wire the ArchiveStateMachine into a dedicated event/render loop.
}

void ArchiveApp::initialize_services()
{
    // TODO: Attach preferences, localization, and other shared services.
    core::LocalizationManager::Get();
    utils::Preferences::Get();
}

void ArchiveApp::configure_state_machine()
{
    // TODO: Instantiate ArchiveStateMachine and configure default routes.
}

} // namespace colony::programs::archive
