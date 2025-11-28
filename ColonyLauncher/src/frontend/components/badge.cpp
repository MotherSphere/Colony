#include "frontend/components/badge.hpp"

#include "ui/layout.hpp"
#include "utils/drawing.hpp"

namespace colony::frontend::components
{

void RenderBadge(
    SDL_Renderer* renderer,
    const colony::ui::ThemeColors& theme,
    const SDL_Rect& bounds,
    std::string_view label,
    TTF_Font* font,
    SDL_Color fillColor,
    SDL_Color textColor)
{
    const int radius = colony::ui::Scale(12);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
    colony::drawing::RenderFilledRoundedRect(renderer, bounds, radius);

    if (!font || label.empty())
    {
        return;
    }

    colony::TextTexture textTexture = colony::CreateTextTexture(renderer, font, label, textColor);
    if (textTexture.texture)
    {
        SDL_Rect textRect{
            bounds.x + (bounds.w - textTexture.width) / 2,
            bounds.y + (bounds.h - textTexture.height) / 2,
            textTexture.width,
            textTexture.height};
        colony::RenderTexture(renderer, textTexture, textRect);
    }
}

} // namespace colony::frontend::components
