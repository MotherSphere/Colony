#pragma once

#include "ui/theme.hpp"
#include "utils/text.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <string_view>

namespace colony::frontend::components
{

void RenderPrimaryButton(
    SDL_Renderer* renderer,
    const colony::ui::ThemeColors& theme,
    const SDL_Rect& bounds,
    std::string_view label,
    TTF_Font* font,
    bool hovered,
    bool pressed);

void RenderSecondaryButton(
    SDL_Renderer* renderer,
    const colony::ui::ThemeColors& theme,
    const SDL_Rect& bounds,
    std::string_view label,
    TTF_Font* font,
    bool hovered,
    bool pressed);

} // namespace colony::frontend::components
