#pragma once

#include "content_loader.hpp"
#include "ui/program_visuals.hpp"
#include "ui/theme.hpp"
#include "utils/text.hpp"

#include <SDL2/SDL.h>

#include <unordered_map>
#include <vector>

namespace colony::ui
{

struct NavigationChrome
{
    colony::TextTexture brand;
    std::vector<colony::TextTexture> channelLabels;
    colony::TextTexture userName;
    colony::TextTexture userStatus;
};

NavigationChrome BuildNavigationChrome(
    SDL_Renderer* renderer,
    TTF_Font* brandFont,
    TTF_Font* navFont,
    TTF_Font* metaFont,
    const colony::AppContent& content,
    const ThemeColors& theme);

std::vector<SDL_Rect> RenderNavigationRail(
    SDL_Renderer* renderer,
    const ThemeColors& theme,
    const SDL_Rect& navRailRect,
    int statusBarHeight,
    const NavigationChrome& chrome,
    const colony::AppContent& content,
    const std::vector<int>& channelSelections,
    int activeChannelIndex,
    const std::unordered_map<std::string, ProgramVisuals>& programVisuals);

} // namespace colony::ui
