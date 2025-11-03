#pragma once

#include <SDL2/SDL.h>

namespace colony::drawing
{

void RenderFilledRoundedRect(SDL_Renderer* renderer, const SDL_Rect& rect, int radius);

void RenderRoundedRect(SDL_Renderer* renderer, const SDL_Rect& rect, int radius);

} // namespace colony::drawing
