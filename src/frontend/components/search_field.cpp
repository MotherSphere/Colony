#include "frontend/components/search_field.hpp"

#include "ui/layout.hpp"
#include "utils/color.hpp"
#include "utils/drawing.hpp"

#include <algorithm>
#include <cmath>

namespace colony::frontend::components
{

void SearchField::Build(SDL_Renderer* renderer, TTF_Font* font, std::string_view placeholder, const colony::ui::ThemeColors& theme)
{
    font_ = font;
    placeholder_ = colony::CreateTextTexture(renderer, font_, std::string{placeholder}, theme.inputPlaceholder);
    cachedValueTexture_ = {};
    cachedValue_.clear();
    cachedTextColor_ = SDL_Color{0, 0, 0, 0};
}

SearchField::RenderResult SearchField::Render(
    SDL_Renderer* renderer,
    const colony::ui::ThemeColors& theme,
    const colony::ui::InteractionColors& interactions,
    const SDL_Rect& bounds,
    std::string_view value,
    bool focused,
    double timeSeconds) const
{
    RenderResult result;
    result.inputRect = bounds;

    const int radius = colony::ui::Scale(16);
    SDL_Color baseFill = focused ? colony::color::Mix(theme.cardActive, theme.inputBackground, 0.45f)
                                 : colony::color::Mix(theme.inputBackground, theme.navRail, 0.35f);
    SDL_Color border = focused ? theme.focusRing : colony::color::Mix(theme.border, theme.inputBorder, 0.55f);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, baseFill.r, baseFill.g, baseFill.b, 230);
    colony::drawing::RenderFilledRoundedRect(renderer, bounds, radius);
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, focused ? 240 : 180);
    colony::drawing::RenderRoundedRect(renderer, bounds, radius);

    const int iconSize = std::max(colony::ui::Scale(18), bounds.h - colony::ui::Scale(16));
    SDL_Rect iconRect{
        bounds.x + colony::ui::Scale(16),
        bounds.y + (bounds.h - iconSize) / 2,
        iconSize,
        iconSize};

    SDL_Color iconColor = focused ? colony::color::Mix(theme.heroTitle, theme.channelBadge, 0.35f)
                                  : colony::color::Mix(theme.navText, theme.inputPlaceholder, 0.5f);
    SDL_SetRenderDrawColor(renderer, iconColor.r, iconColor.g, iconColor.b, iconColor.a);
    colony::drawing::RenderRoundedRect(renderer, iconRect, iconSize / 2);
    SDL_RenderDrawLine(
        renderer,
        iconRect.x + iconRect.w - colony::ui::Scale(4),
        iconRect.y + iconRect.h - colony::ui::Scale(4),
        iconRect.x + iconRect.w + colony::ui::Scale(6),
        iconRect.y + iconRect.h + colony::ui::Scale(6));

    const int textStartX = iconRect.x + iconRect.w + colony::ui::Scale(12);
    const int textMaxWidth = bounds.x + bounds.w - colony::ui::Scale(18) - textStartX;
    if (textMaxWidth <= 0)
    {
        return result;
    }

    SDL_Rect textClip{textStartX, bounds.y + colony::ui::Scale(6), textMaxWidth, bounds.h - colony::ui::Scale(12)};
    SDL_RenderSetClipRect(renderer, &textClip);

    if (!value.empty() && font_)
    {
        SDL_Color desiredColor = theme.heroTitle;
        if (cachedValue_ != value
            || cachedTextColor_.r != desiredColor.r || cachedTextColor_.g != desiredColor.g
            || cachedTextColor_.b != desiredColor.b || cachedTextColor_.a != desiredColor.a)
        {
            cachedValue_ = std::string{value};
            cachedTextColor_ = desiredColor;
            cachedValueTexture_ = colony::CreateTextTexture(renderer, font_, cachedValue_, desiredColor);
        }

        if (cachedValueTexture_.texture)
        {
            SDL_Rect textRect{
                textStartX,
                bounds.y + (bounds.h - cachedValueTexture_.height) / 2,
                cachedValueTexture_.width,
                cachedValueTexture_.height};
            colony::RenderTexture(renderer, cachedValueTexture_, textRect);
        }
    }
    else if (placeholder_.texture)
    {
        cachedValue_.clear();
        cachedValueTexture_ = {};
        SDL_Rect placeholderRect{
            textStartX,
            bounds.y + (bounds.h - placeholder_.height) / 2,
            placeholder_.width,
            placeholder_.height};
        SDL_SetTextureAlphaMod(placeholder_.texture.get(), focused ? 180 : 220);
        colony::RenderTexture(renderer, placeholder_, placeholderRect);
        SDL_SetTextureAlphaMod(placeholder_.texture.get(), 255);
    }

    SDL_RenderSetClipRect(renderer, nullptr);

    if (focused)
    {
        const float pulse = static_cast<float>(std::sin(timeSeconds * 3.0) * 0.5 + 0.5);
        SDL_Color glow = colony::color::Mix(interactions.focus, theme.heroTitle, 0.25f);
        SDL_Rect glowRect = bounds;
        glowRect.x -= colony::ui::Scale(4);
        glowRect.y -= colony::ui::Scale(4);
        glowRect.w += colony::ui::Scale(8);
        glowRect.h += colony::ui::Scale(8);
        SDL_SetRenderDrawColor(renderer, glow.r, glow.g, glow.b, static_cast<Uint8>(80 + pulse * 70.0f));
        colony::drawing::RenderRoundedRect(renderer, glowRect, radius + colony::ui::Scale(4));
    }

    return result;
}

} // namespace colony::frontend::components
