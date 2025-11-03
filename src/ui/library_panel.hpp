#pragma once

#include "core/content.hpp"
#include "ui/program_visuals.hpp"
#include "ui/theme.hpp"
#include "utils/text.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <unordered_map>
#include <vector>

namespace colony::ui
{

struct LibraryRenderResult
{
    std::vector<SDL_Rect> tileRects;
};

class LibraryPanelRenderer
{
  public:
    void Build(SDL_Renderer* renderer, TTF_Font* bodyFont, const ThemeColors& theme);

    LibraryRenderResult Render(
        SDL_Renderer* renderer,
        const ThemeColors& theme,
        const SDL_Rect& libraryRect,
        const colony::AppContent& content,
        int activeChannelIndex,
        const std::vector<int>& channelSelections,
        const std::unordered_map<std::string, ProgramVisuals>& programVisuals,
        TTF_Font* channelFont) const;

  private:
    struct LibraryChrome
    {
        colony::TextTexture filterLabel;
    };

    LibraryChrome chrome_;
};

} // namespace colony::ui
