#pragma once

#include <functional>
#include <utility>

namespace colony::frontend::utils
{

class Debouncer
{
  public:
    explicit Debouncer(double delaySeconds = 0.25) noexcept
        : delaySeconds_(delaySeconds)
    {
    }

    void SetDelay(double delaySeconds) noexcept
    {
        delaySeconds_ = delaySeconds;
    }

    template <typename Callback>
    void Schedule(double nowSeconds, Callback&& callback)
    {
        scheduledAtSeconds_ = nowSeconds;
        pending_ = true;
        callback_ = std::forward<Callback>(callback);
    }

    void Cancel() noexcept
    {
        pending_ = false;
        callback_ = nullptr;
    }

    void Flush(double nowSeconds)
    {
        if (!pending_)
        {
            return;
        }

        if ((nowSeconds - scheduledAtSeconds_) < delaySeconds_)
        {
            return;
        }

        pending_ = false;
        auto callback = std::move(callback_);
        callback_ = nullptr;
        if (callback)
        {
            callback();
        }
    }

    [[nodiscard]] bool HasPending() const noexcept
    {
        return pending_;
    }

  private:
    double delaySeconds_ = 0.0;
    double scheduledAtSeconds_ = 0.0;
    bool pending_ = false;
    std::function<void()> callback_{};
};

} // namespace colony::frontend::utils
