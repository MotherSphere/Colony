#include "programs/archive/ArchiveStateMachine.hpp"

#include "core/localization_manager.hpp"
#include "utils/preferences.hpp"

#include <algorithm>
#include <iostream>
#include <utility>

namespace colony::programs::archive
{

namespace
{
ArchiveStateMachine::Screen DetermineInitialScreen(const ArchiveStateMachine::Dependencies& dependencies)
{
    if (dependencies.preferences == nullptr)
    {
        return ArchiveStateMachine::Screen::Onboarding;
    }

    const auto& prefs = *dependencies.preferences;
    const auto it = prefs.toggleStates.find("archive.onboarding_complete");
    if (it == prefs.toggleStates.end() || !it->second)
    {
        return ArchiveStateMachine::Screen::Onboarding;
    }

    return ArchiveStateMachine::Screen::Unlock;
}
} // namespace

ArchiveStateMachine::ArchiveStateMachine(Dependencies dependencies)
    : dependencies_{dependencies}
    , active_screen_{DetermineInitialScreen(dependencies)}
{
}

void ArchiveStateMachine::Update()
{
    DrainExternalEvents();
    ApplyPendingTransition();
}

void ArchiveStateMachine::Render()
{
    // Rendering will be implemented once the dedicated ImGui layer ships.
    // For now we expose state changes via the console for developer insight.
}

void ArchiveStateMachine::RequestTransition(Screen target)
{
    pending_transition_ = target;
}

void ArchiveStateMachine::Enqueue(const ArchiveEvent& event)
{
    internal_events_.push_back(event);
}

void ArchiveStateMachine::SetStateChangedCallback(std::function<void(Screen)> callback)
{
    on_state_changed_ = std::move(callback);
}

void ArchiveStateMachine::ApplyPendingTransition()
{
    if (!pending_transition_.has_value())
    {
        return;
    }

    if (active_screen_ == *pending_transition_)
    {
        pending_transition_.reset();
        return;
    }

    active_screen_ = *pending_transition_;
    pending_transition_.reset();

    if (on_state_changed_)
    {
        on_state_changed_(active_screen_);
    }
}

void ArchiveStateMachine::HandleEvent(const ArchiveEvent& event)
{
    switch (event.type)
    {
    case ArchiveEventType::BeginOnboarding:
        RequestTransition(Screen::Onboarding);
        break;
    case ArchiveEventType::PromptUnlock:
        RequestTransition(Screen::Unlock);
        break;
    case ArchiveEventType::UnlockSucceeded:
        RequestTransition(Screen::Dashboard);
        break;
    case ArchiveEventType::UnlockFailed:
        toast_queue_.push_back(event.message.empty() ? "Unable to unlock vault." : event.message);
        RequestTransition(Screen::Unlock);
        break;
    case ArchiveEventType::LogoutRequested:
        RequestTransition(Screen::Unlock);
        break;
    case ArchiveEventType::OpenEntryDetail:
        RequestTransition(Screen::EntryDetail);
        break;
    case ArchiveEventType::CloseEntryDetail:
        RequestTransition(Screen::Dashboard);
        break;
    case ArchiveEventType::OpenSettings:
        RequestTransition(Screen::Settings);
        break;
    case ArchiveEventType::CloseSettings:
        RequestTransition(Screen::Dashboard);
        break;
    case ArchiveEventType::ShowToast:
        if (!event.message.empty())
        {
            toast_queue_.push_back(event.message);
        }
        break;
    case ArchiveEventType::PushNotification:
        if (!event.message.empty())
        {
            notifications_.push_back(event.message);
        }
        break;
    case ArchiveEventType::None:
    default:
        break;
    }
}

void ArchiveStateMachine::DrainExternalEvents()
{
    if (dependencies_.eventBus != nullptr)
    {
        auto external = dependencies_.eventBus->Drain();
        internal_events_.insert(internal_events_.end(), external.begin(), external.end());
    }

    for (const auto& event : internal_events_)
    {
        HandleEvent(event);
    }
    internal_events_.clear();
}

std::vector<std::string> ArchiveStateMachine::ConsumeToasts()
{
    std::vector<std::string> out = std::move(toast_queue_);
    toast_queue_.clear();
    return out;
}

std::vector<std::string> ArchiveStateMachine::ConsumeNotifications()
{
    std::vector<std::string> out = std::move(notifications_);
    notifications_.clear();
    return out;
}

std::string_view ToString(ArchiveStateMachine::Screen screen) noexcept
{
    switch (screen)
    {
    case ArchiveStateMachine::Screen::Onboarding:
        return "Onboarding";
    case ArchiveStateMachine::Screen::Unlock:
        return "Unlock";
    case ArchiveStateMachine::Screen::Dashboard:
        return "Dashboard";
    case ArchiveStateMachine::Screen::EntryDetail:
        return "EntryDetail";
    case ArchiveStateMachine::Screen::Settings:
        return "Settings";
    }

    return "Unknown";
}

} // namespace colony::programs::archive

