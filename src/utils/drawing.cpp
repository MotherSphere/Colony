#include "utils/drawing.hpp"

#include <algorithm>
#include <cmath>

namespace colony::drawing
{
namespace
{
int ClampRadius(const SDL_Rect& rect, int radius)
{
    const int maxRadius = std::min(rect.w, rect.h) / 2;
    if (maxRadius <= 0)
    {
        return 0;
    }
    return std::clamp(radius, 0, maxRadius);
}

void DrawCornerPoints(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, bool filled)
{
    const float centerOffset = static_cast<float>(radius) - 0.5f;
    const int xMax = rect.x + rect.w - 1;
    const int yMax = rect.y + rect.h - 1;

    for (int dy = 0; dy < radius; ++dy)
    {
        for (int dx = 0; dx < radius; ++dx)
        {
            const float distance = std::hypot(dx - centerOffset, dy - centerOffset);
            if (filled)
            {
                if (distance <= static_cast<float>(radius))
                {
                    SDL_RenderDrawPoint(renderer, rect.x + dx, rect.y + dy);
                    SDL_RenderDrawPoint(renderer, xMax - dx, rect.y + dy);
                    SDL_RenderDrawPoint(renderer, rect.x + dx, yMax - dy);
                    SDL_RenderDrawPoint(renderer, xMax - dx, yMax - dy);
                }
            }
            else
            {
                if (distance >= static_cast<float>(radius) - 1.0f && distance <= static_cast<float>(radius))
                {
                    SDL_RenderDrawPoint(renderer, rect.x + dx, rect.y + dy);
                    SDL_RenderDrawPoint(renderer, xMax - dx, rect.y + dy);
                    SDL_RenderDrawPoint(renderer, rect.x + dx, yMax - dy);
                    SDL_RenderDrawPoint(renderer, xMax - dx, yMax - dy);
                }
            }
        }
    }
}

} // namespace

void RenderFilledRoundedRect(SDL_Renderer* renderer, const SDL_Rect& rect, int radius)
{
    if (renderer == nullptr || rect.w <= 0 || rect.h <= 0)
    {
        return;
    }

    radius = ClampRadius(rect, radius);
    if (radius == 0)
    {
        SDL_RenderFillRect(renderer, &rect);
        return;
    }

    const int diameter = radius * 2;

    SDL_Rect centerRect{rect.x + radius, rect.y + radius, rect.w - diameter, rect.h - diameter};
    if (centerRect.w > 0 && centerRect.h > 0)
    {
        SDL_RenderFillRect(renderer, &centerRect);
    }

    SDL_Rect topRect{rect.x + radius, rect.y, rect.w - diameter, radius};
    if (topRect.w > 0)
    {
        SDL_RenderFillRect(renderer, &topRect);
    }

    SDL_Rect bottomRect{rect.x + radius, rect.y + rect.h - radius, rect.w - diameter, radius};
    if (bottomRect.w > 0)
    {
        SDL_RenderFillRect(renderer, &bottomRect);
    }

    SDL_Rect leftRect{rect.x, rect.y + radius, radius, rect.h - diameter};
    if (leftRect.h > 0)
    {
        SDL_RenderFillRect(renderer, &leftRect);
    }

    SDL_Rect rightRect{rect.x + rect.w - radius, rect.y + radius, radius, rect.h - diameter};
    if (rightRect.h > 0)
    {
        SDL_RenderFillRect(renderer, &rightRect);
    }

    DrawCornerPoints(renderer, rect, radius, true);
}

void RenderRoundedRect(SDL_Renderer* renderer, const SDL_Rect& rect, int radius)
{
    if (renderer == nullptr || rect.w <= 0 || rect.h <= 0)
    {
        return;
    }

    radius = ClampRadius(rect, radius);
    if (radius == 0)
    {
        SDL_RenderDrawRect(renderer, &rect);
        return;
    }

    const int x1 = rect.x;
    const int y1 = rect.y;
    const int x2 = rect.x + rect.w - 1;
    const int y2 = rect.y + rect.h - 1;

    SDL_RenderDrawLine(renderer, x1 + radius, y1, x2 - radius, y1);
    SDL_RenderDrawLine(renderer, x1 + radius, y2, x2 - radius, y2);
    SDL_RenderDrawLine(renderer, x1, y1 + radius, x1, y2 - radius);
    SDL_RenderDrawLine(renderer, x2, y1 + radius, x2, y2 - radius);

    DrawCornerPoints(renderer, rect, radius, false);
}

} // namespace colony::drawing
