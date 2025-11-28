#pragma once

#include "ui/theme.hpp"
#include "utils/text.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <string_view>

namespace colony::frontend::components
{

void RenderBadge(
    SDL_Renderer* renderer,
    const colony::ui::ThemeColors& theme,
    const SDL_Rect& bounds,
    std::string_view label,
    TTF_Font* font,
    SDL_Color fillColor,
    SDL_Color textColor);

}
