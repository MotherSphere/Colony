#include "frontend/models/library_view_model.hpp"

#include <algorithm>
#include <cctype>

namespace colony::frontend::models
{
namespace
{
std::string Trim(std::string value)
{
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    value.erase(value.begin(), first);
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    value.erase(last, value.end());
    return value;
}

std::string ToLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

} // namespace

void LibraryViewModel::SetFilter(std::string value)
{
    filter_ = Trim(std::move(value));
    normalizedFilter_ = ToLower(filter_);
}

const std::string& LibraryViewModel::Filter() const noexcept
{
    return filter_;
}

void LibraryViewModel::SetSortOption(LibrarySortOption option) noexcept
{
    sortOption_ = option;
}

LibrarySortOption LibraryViewModel::SortOption() const noexcept
{
    return sortOption_;
}

std::vector<LibrarySortChip> LibraryViewModel::BuildSortChips(
    const std::function<std::string(std::string_view)>& localize) const
{
    auto resolveLabel = [&](std::string_view key, std::string_view fallback) {
        if (localize)
        {
            std::string localized = localize(key);
            if (!localized.empty())
            {
                return localized;
            }
        }
        return std::string{fallback};
    };

    std::vector<LibrarySortChip> chips;
    chips.reserve(2);
    chips.push_back(LibrarySortChip{
        LibrarySortOption::RecentlyPlayed,
        resolveLabel("library.sort_recent", "Recently Played"),
        sortOption_ == LibrarySortOption::RecentlyPlayed});
    chips.push_back(LibrarySortChip{
        LibrarySortOption::Alphabetical,
        resolveLabel("library.sort_alphabetical", "Alphabetical"),
        sortOption_ == LibrarySortOption::Alphabetical});
    return chips;
}

std::vector<LibraryProgramEntry> LibraryViewModel::BuildProgramList(
    const colony::AppContent& content,
    int activeChannelIndex,
    const std::vector<int>& channelSelections) const
{
    std::vector<LibraryProgramEntry> entries;

    if (activeChannelIndex < 0 || activeChannelIndex >= static_cast<int>(content.channels.size()))
    {
        return entries;
    }

    const auto& channel = content.channels[activeChannelIndex];
    if (channel.programs.empty())
    {
        return entries;
    }

    std::vector<std::string> workingPrograms = channel.programs;
    if (sortOption_ == LibrarySortOption::Alphabetical)
    {
        auto makeSortKey = [&](const std::string& programId) {
            auto viewIt = content.views.find(programId);
            if (viewIt != content.views.end())
            {
                return ToLower(viewIt->second.heading);
            }
            return ToLower(programId);
        };

        std::sort(workingPrograms.begin(), workingPrograms.end(), [&](const std::string& lhs, const std::string& rhs) {
            return makeSortKey(lhs) < makeSortKey(rhs);
        });
    }

    std::string selectedProgramId;
    if (activeChannelIndex >= 0 && activeChannelIndex < static_cast<int>(channelSelections.size()))
    {
        const int selectionIndex = std::clamp(
            channelSelections[activeChannelIndex],
            0,
            static_cast<int>(channel.programs.size()) - 1);
        if (selectionIndex >= 0 && selectionIndex < static_cast<int>(channel.programs.size()))
        {
            selectedProgramId = channel.programs[selectionIndex];
        }
    }

    entries.reserve(workingPrograms.size());
    for (const auto& programId : workingPrograms)
    {
        if (!normalizedFilter_.empty())
        {
            auto viewIt = content.views.find(programId);
            bool matches = MatchesFilter(programId);
            if (viewIt != content.views.end())
            {
                matches = matches || MatchesFilter(viewIt->second.heading) || MatchesFilter(viewIt->second.tagline);
            }
            if (!matches)
            {
                continue;
            }
        }

        entries.push_back(LibraryProgramEntry{programId, programId == selectedProgramId});
    }

    return entries;
}

bool LibraryViewModel::HasActiveFilter() const noexcept
{
    return !normalizedFilter_.empty();
}

bool LibraryViewModel::MatchesFilter(std::string_view value) const
{
    if (normalizedFilter_.empty())
    {
        return true;
    }

    std::string candidate{value};
    candidate = ToLower(candidate);
    return candidate.find(normalizedFilter_) != std::string::npos;
}

} // namespace colony::frontend::models
