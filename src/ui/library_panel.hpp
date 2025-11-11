#pragma once

#include "core/content.hpp"
#include "frontend/models/library_view_model.hpp"
#include "ui/program_visuals.hpp"
#include "ui/theme.hpp"
#include "utils/text.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace colony::ui
{

struct LibraryRenderResult
{
    std::vector<SDL_Rect> tileRects;
    std::optional<SDL_Rect> addButtonRect;
    std::vector<std::string> programIds;
    std::optional<SDL_Rect> filterInputRect;

    struct SortChipHitbox
    {
        SDL_Rect rect{0, 0, 0, 0};
        colony::frontend::models::LibrarySortOption option = colony::frontend::models::LibrarySortOption::RecentlyPlayed;
    };
    std::vector<SortChipHitbox> sortChipHitboxes;
};

class LibraryPanelRenderer
{
  public:
    void Build(
        SDL_Renderer* renderer,
        TTF_Font* bodyFont,
        const ThemeColors& theme,
        const std::function<std::string(std::string_view)>& localize);

    LibraryRenderResult Render(
        SDL_Renderer* renderer,
        const ThemeColors& theme,
        const SDL_Rect& libraryRect,
        const colony::AppContent& content,
        int activeChannelIndex,
        const std::unordered_map<std::string, ProgramVisuals>& programVisuals,
        TTF_Font* channelFont,
        TTF_Font* bodyFont,
        bool showAddButton,
        double timeSeconds,
        double deltaSeconds,
        std::string_view filterText,
        bool filterFocused,
        const std::vector<colony::frontend::models::LibraryProgramEntry>& programs,
        const std::vector<colony::frontend::models::LibrarySortChip>& sortChips) const;

  private:
    struct LibraryChrome
    {
        colony::TextTexture filterPlaceholder;
    };

    LibraryChrome chrome_;
};

} // namespace colony::ui
