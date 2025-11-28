#pragma once

#include "ui/theme.hpp"
#include "utils/sdl_wrappers.hpp"

#include <SDL2/SDL.h>

#include <string>
#include <string_view>

namespace colony::frontend::icons
{

struct IconTexture
{
    sdl::TextureHandle texture;
    int width = 0;
    int height = 0;
};

const IconTexture& LoadSidebarIcon(
    SDL_Renderer* renderer,
    std::string_view id,
    SDL_Color accent,
    const colony::ui::ThemeColors& theme);

} // namespace colony::frontend::icons
