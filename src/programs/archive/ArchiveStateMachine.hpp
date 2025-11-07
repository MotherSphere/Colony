#pragma once

#include "programs/archive/ArchiveEvent.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace colony
{
class LocalizationManager;
}

namespace colony::preferences
{
struct Preferences;
}

namespace colony::programs::archive
{

//! Central coordinator for Archive Vault screens and transitions.
class ArchiveStateMachine
{
  public:
    enum class Screen
    {
        Onboarding,
        Unlock,
        Dashboard,
        EntryDetail,
        Settings,
    };

    struct Dependencies
    {
        LocalizationManager* localization = nullptr;
        preferences::Preferences* preferences = nullptr;
        ArchiveEventBus* eventBus = nullptr;
    };

    explicit ArchiveStateMachine(Dependencies dependencies);

    void Update();
    void Render();

    void RequestTransition(Screen target);
    void Enqueue(const ArchiveEvent& event);

    [[nodiscard]] Screen ActiveScreen() const noexcept { return active_screen_; }
    [[nodiscard]] std::vector<std::string> ConsumeToasts();
    [[nodiscard]] std::vector<std::string> ConsumeNotifications();

    void SetStateChangedCallback(std::function<void(Screen)> callback);

  private:
    void ApplyPendingTransition();
    void HandleEvent(const ArchiveEvent& event);
    void DrainExternalEvents();

    Dependencies dependencies_{};
    Screen active_screen_ = Screen::Onboarding;
    std::optional<Screen> pending_transition_{};
    std::vector<ArchiveEvent> internal_events_{};
    std::vector<std::string> toast_queue_{};
    std::vector<std::string> notifications_{};
    std::function<void(Screen)> on_state_changed_{};
};

std::string_view ToString(ArchiveStateMachine::Screen screen) noexcept;

} // namespace colony::programs::archive

