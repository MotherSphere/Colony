#include "controllers/navigation_controller.hpp"

#include <algorithm>

namespace colony
{

void NavigationController::SetEntries(std::vector<std::string> entries)
{
    entries_ = std::move(entries);
    if (activeIndex_ >= static_cast<int>(entries_.size()))
    {
        activeIndex_ = 0;
    }
}

void NavigationController::OnSelectionChanged(Callback callback)
{
    callback_ = std::move(callback);
}

bool NavigationController::Activate(int index)
{
    if (index < 0 || index >= static_cast<int>(entries_.size()) || index == activeIndex_)
    {
        return false;
    }

    activeIndex_ = index;
    if (callback_)
    {
        callback_(activeIndex_);
    }
    return true;
}

} // namespace colony
