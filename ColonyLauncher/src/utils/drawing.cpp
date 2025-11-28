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

void DrawCornerPoints(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, bool filled, int cornerMask)
{
    if (radius <= 0 || cornerMask == CornerNone)
    {
        return;
    }

    const float centerOffset = static_cast<float>(radius) - 0.5f;
    const int xMax = rect.x + rect.w - 1;
    const int yMax = rect.y + rect.h - 1;

    const bool roundTopLeft = (cornerMask & CornerTopLeft) != 0;
    const bool roundTopRight = (cornerMask & CornerTopRight) != 0;
    const bool roundBottomLeft = (cornerMask & CornerBottomLeft) != 0;
    const bool roundBottomRight = (cornerMask & CornerBottomRight) != 0;

    for (int dy = 0; dy < radius; ++dy)
    {
        for (int dx = 0; dx < radius; ++dx)
        {
            const float distance = std::hypot(dx - centerOffset, dy - centerOffset);
            if (filled)
            {
                if (distance <= static_cast<float>(radius))
                {
                    if (roundTopLeft)
                    {
                        SDL_RenderDrawPoint(renderer, rect.x + dx, rect.y + dy);
                    }
                    if (roundTopRight)
                    {
                        SDL_RenderDrawPoint(renderer, xMax - dx, rect.y + dy);
                    }
                    if (roundBottomLeft)
                    {
                        SDL_RenderDrawPoint(renderer, rect.x + dx, yMax - dy);
                    }
                    if (roundBottomRight)
                    {
                        SDL_RenderDrawPoint(renderer, xMax - dx, yMax - dy);
                    }
                }
            }
            else
            {
                if (distance >= static_cast<float>(radius) - 1.0f && distance <= static_cast<float>(radius))
                {
                    if (roundTopLeft)
                    {
                        SDL_RenderDrawPoint(renderer, rect.x + dx, rect.y + dy);
                    }
                    if (roundTopRight)
                    {
                        SDL_RenderDrawPoint(renderer, xMax - dx, rect.y + dy);
                    }
                    if (roundBottomLeft)
                    {
                        SDL_RenderDrawPoint(renderer, rect.x + dx, yMax - dy);
                    }
                    if (roundBottomRight)
                    {
                        SDL_RenderDrawPoint(renderer, xMax - dx, yMax - dy);
                    }
                }
            }
        }
    }
}

} // namespace

void RenderFilledRoundedRect(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, int cornerMask)
{
    if (renderer == nullptr || rect.w <= 0 || rect.h <= 0)
    {
        return;
    }

    radius = ClampRadius(rect, radius);
    if (radius == 0 || cornerMask == CornerNone)
    {
        SDL_RenderFillRect(renderer, &rect);
        return;
    }

    const int radiusTopLeft = (cornerMask & CornerTopLeft) != 0 ? radius : 0;
    const int radiusTopRight = (cornerMask & CornerTopRight) != 0 ? radius : 0;
    const int radiusBottomLeft = (cornerMask & CornerBottomLeft) != 0 ? radius : 0;
    const int radiusBottomRight = (cornerMask & CornerBottomRight) != 0 ? radius : 0;

    const int leftRadius = std::max(radiusTopLeft, radiusBottomLeft);
    const int rightRadius = std::max(radiusTopRight, radiusBottomRight);
    const int topRadius = std::max(radiusTopLeft, radiusTopRight);
    const int bottomRadius = std::max(radiusBottomLeft, radiusBottomRight);

    SDL_Rect centerRect{
        rect.x + leftRadius,
        rect.y + topRadius,
        rect.w - leftRadius - rightRadius,
        rect.h - topRadius - bottomRadius};
    if (centerRect.w > 0 && centerRect.h > 0)
    {
        SDL_RenderFillRect(renderer, &centerRect);
    }

    if (topRadius > 0)
    {
        SDL_Rect topRect{rect.x + radiusTopLeft, rect.y, rect.w - radiusTopLeft - radiusTopRight, topRadius};
        if (topRect.w > 0 && topRect.h > 0)
        {
            SDL_RenderFillRect(renderer, &topRect);
        }
    }

    if (bottomRadius > 0)
    {
        SDL_Rect bottomRect{
            rect.x + radiusBottomLeft,
            rect.y + rect.h - bottomRadius,
            rect.w - radiusBottomLeft - radiusBottomRight,
            bottomRadius};
        if (bottomRect.w > 0 && bottomRect.h > 0)
        {
            SDL_RenderFillRect(renderer, &bottomRect);
        }
    }

    if (leftRadius > 0)
    {
        SDL_Rect leftRect{rect.x, rect.y + radiusTopLeft, leftRadius, rect.h - radiusTopLeft - radiusBottomLeft};
        if (leftRect.w > 0 && leftRect.h > 0)
        {
            SDL_RenderFillRect(renderer, &leftRect);
        }
    }

    if (rightRadius > 0)
    {
        SDL_Rect rightRect{
            rect.x + rect.w - rightRadius,
            rect.y + radiusTopRight,
            rightRadius,
            rect.h - radiusTopRight - radiusBottomRight};
        if (rightRect.w > 0 && rightRect.h > 0)
        {
            SDL_RenderFillRect(renderer, &rightRect);
        }
    }

    DrawCornerPoints(renderer, rect, radius, true, cornerMask);
}

void RenderRoundedRect(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, int cornerMask)
{
    if (renderer == nullptr || rect.w <= 0 || rect.h <= 0)
    {
        return;
    }

    radius = ClampRadius(rect, radius);
    if (radius == 0 || cornerMask == CornerNone)
    {
        SDL_RenderDrawRect(renderer, &rect);
        return;
    }

    const int radiusTopLeft = (cornerMask & CornerTopLeft) != 0 ? radius : 0;
    const int radiusTopRight = (cornerMask & CornerTopRight) != 0 ? radius : 0;
    const int radiusBottomLeft = (cornerMask & CornerBottomLeft) != 0 ? radius : 0;
    const int radiusBottomRight = (cornerMask & CornerBottomRight) != 0 ? radius : 0;

    const int x1 = rect.x;
    const int y1 = rect.y;
    const int x2 = rect.x + rect.w - 1;
    const int y2 = rect.y + rect.h - 1;

    const int topStart = x1 + radiusTopLeft;
    const int topEnd = x2 - radiusTopRight;
    if (topEnd >= topStart)
    {
        SDL_RenderDrawLine(renderer, topStart, y1, topEnd, y1);
    }

    const int bottomStart = x1 + radiusBottomLeft;
    const int bottomEnd = x2 - radiusBottomRight;
    if (bottomEnd >= bottomStart)
    {
        SDL_RenderDrawLine(renderer, bottomStart, y2, bottomEnd, y2);
    }

    const int leftStart = y1 + radiusTopLeft;
    const int leftEnd = y2 - radiusBottomLeft;
    if (leftEnd >= leftStart)
    {
        SDL_RenderDrawLine(renderer, x1, leftStart, x1, leftEnd);
    }

    const int rightStart = y1 + radiusTopRight;
    const int rightEnd = y2 - radiusBottomRight;
    if (rightEnd >= rightStart)
    {
        SDL_RenderDrawLine(renderer, x2, rightStart, x2, rightEnd);
    }

    DrawCornerPoints(renderer, rect, radius, false, cornerMask);
}

} // namespace colony::drawing
