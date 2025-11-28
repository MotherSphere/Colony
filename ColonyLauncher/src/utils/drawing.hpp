#pragma once

#include <SDL2/SDL.h>

namespace colony::drawing
{

enum RoundedCorner
{
    CornerNone = 0,
    CornerTopLeft = 1 << 0,
    CornerTopRight = 1 << 1,
    CornerBottomLeft = 1 << 2,
    CornerBottomRight = 1 << 3,
    CornerAll = CornerTopLeft | CornerTopRight | CornerBottomLeft | CornerBottomRight
};

void RenderFilledRoundedRect(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, int cornerMask = CornerAll);

void RenderRoundedRect(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, int cornerMask = CornerAll);

} // namespace colony::drawing
