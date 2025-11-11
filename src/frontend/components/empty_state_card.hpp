#pragma once

#include "ui/theme.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <string_view>

namespace colony::frontend::components
{

void RenderEmptyStateCard(
    SDL_Renderer* renderer,
    const colony::ui::ThemeColors& theme,
    const SDL_Rect& bounds,
    TTF_Font* titleFont,
    TTF_Font* bodyFont,
    std::string_view title,
    std::string_view message,
    double timeSeconds);

} // namespace colony::frontend::components
