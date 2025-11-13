#pragma once

#include "ui/theme.hpp"
#include "utils/font_manager.hpp"
#include "utils/sdl_wrappers.hpp"

#include <SDL2/SDL_ttf.h>

#include <filesystem>
#include <optional>

namespace colony::frontend::fonts
{

enum class FontRole
{
    Display,
    Headline,
    Title,
    Subtitle,
    Body,
    Label,
    Caption,
};

struct FontSet
{
    sdl::FontHandle display;
    sdl::FontHandle headline;
    sdl::FontHandle title;
    sdl::FontHandle subtitle;
    sdl::FontHandle body;
    sdl::FontHandle label;
    sdl::FontHandle caption;
};

struct LoadFontSetParams
{
    colony::ui::Typography typography;
    colony::fonts::FontConfiguration configuration;
};

FontSet LoadFontSet(const LoadFontSetParams& params);

std::filesystem::path ResolveFontForRole(FontRole role, const LoadFontSetParams& params);

} // namespace colony::frontend::fonts
