#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace colony::programs::archive
{

enum class ArchiveEventType
{
    None,
    BeginOnboarding,
    PromptUnlock,
    UnlockSucceeded,
    UnlockFailed,
    LogoutRequested,
    OpenEntryDetail,
    CloseEntryDetail,
    OpenSettings,
    CloseSettings,
    ShowToast,
    PushNotification,
};

struct ArchiveEvent
{
    ArchiveEventType type = ArchiveEventType::None;
    std::string payload{};
    std::string message{};
};

class ArchiveEventBus
{
  public:
    void Publish(ArchiveEvent event)
    {
        queue_.emplace_back(std::move(event));
    }

    [[nodiscard]] bool Empty() const noexcept { return queue_.empty(); }

    [[nodiscard]] std::vector<ArchiveEvent> Drain()
    {
        std::vector<ArchiveEvent> events = std::move(queue_);
        queue_.clear();
        return events;
    }

  private:
    std::vector<ArchiveEvent> queue_{};
};

inline ArchiveEvent MakeToastEvent(std::string_view message)
{
    ArchiveEvent event;
    event.type = ArchiveEventType::ShowToast;
    event.message = message;
    return event;
}

inline ArchiveEvent MakeNotificationEvent(std::string_view message)
{
    ArchiveEvent event;
    event.type = ArchiveEventType::PushNotification;
    event.message = message;
    return event;
}

} // namespace colony::programs::archive

