#pragma once

#include <SDL2/SDL.h>

#include <string>
#include <string_view>

namespace colony::color
{

SDL_Color ParseHexColor(std::string_view hex, SDL_Color fallback = {255, 255, 255, SDL_ALPHA_OPAQUE});

SDL_Color Mix(const SDL_Color& a, const SDL_Color& b, float t);

void RenderVerticalGradient(SDL_Renderer* renderer, const SDL_Rect& area, SDL_Color top, SDL_Color bottom);

std::string ToHexString(const SDL_Color& color);

} // namespace colony::color
