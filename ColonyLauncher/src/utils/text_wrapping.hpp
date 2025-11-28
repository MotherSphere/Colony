#pragma once

#include <SDL2/SDL_ttf.h>

#include <string>
#include <string_view>
#include <vector>

namespace colony
{

// Wraps UTF-8 text to fit within maxWidth pixels using the provided font.
// Returns a vector of lines preserving explicit line breaks present in the
// source text.
std::vector<std::string> WrapTextToWidth(TTF_Font* font, std::string_view text, int maxWidth);

} // namespace colony

