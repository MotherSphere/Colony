#pragma once

#include "core/content.hpp"
#include "ui/program_visuals.hpp"
#include "ui/theme.hpp"
#include "utils/text.hpp"

#include <SDL2/SDL.h>

#include <unordered_map>
#include <vector>

namespace colony::ui
{

class NavigationRail
{
  public:
    void Build(
        SDL_Renderer* renderer,
        TTF_Font* brandFont,
        TTF_Font* navFont,
        TTF_Font* metaFont,
        const colony::AppContent& content,
        const ThemeColors& theme);

    std::vector<SDL_Rect> Render(
        SDL_Renderer* renderer,
        const ThemeColors& theme,
        const SDL_Rect& navRailRect,
        int statusBarHeight,
        const colony::AppContent& content,
        const std::vector<int>& channelSelections,
        int activeChannelIndex,
        const std::unordered_map<std::string, ProgramVisuals>& programVisuals,
        double timeSeconds) const;

  private:
    struct NavigationChrome
    {
        colony::TextTexture brand;
    };

    NavigationChrome chrome_;
};

} // namespace colony::ui
