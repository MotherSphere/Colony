#include "frontend/components/buttons.hpp"

#include "ui/layout.hpp"
#include "utils/color.hpp"
#include "utils/drawing.hpp"

namespace colony::frontend::components
{
namespace
{
void RenderButtonInternal(
    SDL_Renderer* renderer,
    const colony::ui::ThemeColors& theme,
    const SDL_Rect& bounds,
    std::string_view label,
    TTF_Font* font,
    SDL_Color fill,
    SDL_Color outline,
    SDL_Color textColor)
{
    const int radius = colony::ui::Scale(14);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, bounds, radius);
    SDL_SetRenderDrawColor(renderer, outline.r, outline.g, outline.b, outline.a);
    colony::drawing::RenderRoundedRect(renderer, bounds, radius);

    if (!font || label.empty())
    {
        return;
    }

    colony::TextTexture labelTexture = colony::CreateTextTexture(renderer, font, label, textColor);
    if (labelTexture.texture)
    {
        SDL_Rect labelRect{
            bounds.x + (bounds.w - labelTexture.width) / 2,
            bounds.y + (bounds.h - labelTexture.height) / 2,
            labelTexture.width,
            labelTexture.height};
        colony::RenderTexture(renderer, labelTexture, labelRect);
    }
}
} // namespace

void RenderPrimaryButton(
    SDL_Renderer* renderer,
    const colony::ui::ThemeColors& theme,
    const SDL_Rect& bounds,
    std::string_view label,
    TTF_Font* font,
    bool hovered,
    bool pressed)
{
    SDL_Color fill = pressed ? theme.buttonPrimaryActive : hovered ? theme.buttonPrimaryHover : theme.buttonPrimary;
    SDL_Color outline = colony::color::Mix(theme.buttonPrimary, theme.heroTitle, hovered ? 0.35f : 0.2f);
    SDL_Color textColor = theme.heroTitle;
    RenderButtonInternal(renderer, theme, bounds, label, font, fill, outline, textColor);
}

void RenderSecondaryButton(
    SDL_Renderer* renderer,
    const colony::ui::ThemeColors& theme,
    const SDL_Rect& bounds,
    std::string_view label,
    TTF_Font* font,
    bool hovered,
    bool pressed)
{
    SDL_Color fill = pressed ? colony::color::Mix(theme.buttonGhost, theme.cardActive, 0.4f)
                             : hovered ? theme.buttonGhostHover
                                       : theme.buttonGhost;
    SDL_Color outline = colony::color::Mix(theme.border, theme.buttonGhost, hovered ? 0.4f : 0.2f);
    SDL_Color textColor = colony::color::Mix(theme.heroTitle, theme.navText, 0.35f);
    RenderButtonInternal(renderer, theme, bounds, label, font, fill, outline, textColor);
}

} // namespace colony::frontend::components
