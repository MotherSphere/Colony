#pragma once

#include "core/content.hpp"

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace colony::frontend::models
{

enum class LibrarySortOption
{
    RecentlyPlayed,
    Alphabetical,
};

struct LibrarySortChip
{
    LibrarySortOption option;
    std::string label;
    bool active = false;
};

struct LibraryProgramEntry
{
    std::string programId;
    bool selected = false;
};

class LibraryViewModel
{
  public:
    void SetFilter(std::string value);
    [[nodiscard]] const std::string& Filter() const noexcept;

    void SetSortOption(LibrarySortOption option) noexcept;
    [[nodiscard]] LibrarySortOption SortOption() const noexcept;

    [[nodiscard]] std::vector<LibrarySortChip> BuildSortChips(
        const std::function<std::string(std::string_view)>& localize) const;

    [[nodiscard]] std::vector<LibraryProgramEntry> BuildProgramList(
        const colony::AppContent& content,
        int activeChannelIndex,
        const std::vector<int>& channelSelections) const;

    [[nodiscard]] bool HasActiveFilter() const noexcept;

  private:
    [[nodiscard]] bool MatchesFilter(std::string_view value) const;

    std::string filter_;
    std::string normalizedFilter_;
    LibrarySortOption sortOption_ = LibrarySortOption::RecentlyPlayed;
};

} // namespace colony::frontend::models
