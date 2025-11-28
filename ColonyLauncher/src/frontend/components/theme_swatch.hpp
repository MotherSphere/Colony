#pragma once

#include "ui/theme.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <string_view>

namespace colony
{
struct TextTexture;
}

namespace colony::frontend::components
{

struct ThemeSwatchText
{
    std::string_view heading;
    std::string_view body;
};

struct ThemeSwatchStyle
{
    int cornerRadius = 12;
    int padding = 12;
    float accentPulse = 0.0f;
};

void RenderThemeSwatch(
    SDL_Renderer* renderer,
    const colony::ui::ThemeColors& theme,
    const SDL_Rect& bounds,
    TTF_Font* headingFont,
    TTF_Font* bodyFont,
    const ThemeSwatchText& text,
    const ThemeSwatchStyle& style = {});

} // namespace colony::frontend::components
