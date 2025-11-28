#pragma once

#include <functional>
#include <string>
#include <vector>

namespace colony
{

class NavigationController
{
  public:
    using Callback = std::function<void(int)>;

    void SetEntries(std::vector<std::string> entries);
    void OnSelectionChanged(Callback callback);

    [[nodiscard]] const std::vector<std::string>& Entries() const noexcept { return entries_; }
    [[nodiscard]] int ActiveIndex() const noexcept { return activeIndex_; }

    bool Activate(int index);

  private:
    std::vector<std::string> entries_;
    Callback callback_;
    int activeIndex_ = 0;
};

} // namespace colony
