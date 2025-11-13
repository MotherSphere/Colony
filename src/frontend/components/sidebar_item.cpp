#include "frontend/components/sidebar_item.hpp"

#include "ui/layout.hpp"
#include "utils/color.hpp"
#include "utils/drawing.hpp"
#include "utils/text.hpp"

#include <algorithm>
#include <cmath>

namespace colony::frontend::components
{

void SidebarItem::Build(
    SDL_Renderer* renderer,
    TTF_Font* font,
    std::string_view id,
    std::string_view label,
    const colony::ui::ThemeColors& theme)
{
    id_ = std::string{id};
    labelTexture_ = colony::CreateTextTexture(renderer, font, std::string{label}, theme.navText);
}

SDL_Rect SidebarItem::Render(
    SDL_Renderer* renderer,
    const colony::ui::ThemeColors& theme,
    const colony::ui::Typography& typography,
    const colony::ui::InteractionColors& interactions,
    const SDL_Rect& bounds,
    SDL_Color accent,
    bool active,
    bool hovered,
    double timeSeconds) const
{
    (void)typography;
    SDL_Rect itemRect = bounds;
    const int radius = colony::ui::Scale(18);

    SDL_Color baseFill = theme.navRail;
    if (active)
    {
        baseFill = colony::color::Mix(theme.cardActive, theme.navRail, 0.55f);
    }
    else if (hovered)
    {
        baseFill = colony::color::Mix(theme.cardHover, theme.navRail, 0.4f);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, baseFill.r, baseFill.g, baseFill.b, 215);
    colony::drawing::RenderFilledRoundedRect(renderer, itemRect, radius);

    if (active)
    {
        SDL_Color outline = colony::color::Mix(accent, theme.heroTitle, 0.35f);
        SDL_SetRenderDrawColor(renderer, outline.r, outline.g, outline.b, 235);
        colony::drawing::RenderRoundedRect(renderer, itemRect, radius);
    }

    const int iconSize = colony::ui::Scale(28);
    const int padding = colony::ui::Scale(18);
    const SDL_Rect iconRect{itemRect.x + padding, itemRect.y + (itemRect.h - iconSize) / 2, iconSize, iconSize};

    const auto& icon = icons::LoadSidebarIcon(renderer, id_, accent, theme);
    if (icon.texture)
    {
        SDL_Rect renderRect = iconRect;
        renderRect.w = icon.width;
        renderRect.h = icon.height;
        renderRect.y = itemRect.y + (itemRect.h - renderRect.h) / 2;
        SDL_RenderCopy(renderer, icon.texture.get(), nullptr, &renderRect);
    }

    int textX = iconRect.x + iconRect.w + colony::ui::Scale(12);
    SDL_Color textColor = active ? theme.heroTitle : theme.navText;
    if (hovered && !active)
    {
        textColor = colony::color::Mix(textColor, theme.heroTitle, 0.35f);
    }

    const colony::TextTexture& renderLabel = labelTexture_;
    if (renderLabel.texture)
    {
        SDL_Rect labelRect{
            textX,
            itemRect.y + (itemRect.h - renderLabel.height) / 2,
            renderLabel.width,
            renderLabel.height};
        SDL_SetTextureColorMod(renderLabel.texture.get(), textColor.r, textColor.g, textColor.b);
        SDL_RenderCopy(renderer, renderLabel.texture.get(), nullptr, &labelRect);
        SDL_SetTextureColorMod(renderLabel.texture.get(), 255, 255, 255);
    }

    SDL_Color glow = interactions.subtleGlow;
    if (active)
    {
        glow = colony::color::Mix(accent, theme.heroTitle, 0.35f);
        glow.a = 90;
    }
    const float pulse = active ? static_cast<float>(std::sin(timeSeconds * 2.2) * 0.5 + 0.5) : 0.0f;
    if (pulse > 0.01f)
    {
        SDL_Color halo = colony::color::Mix(glow, theme.navRail, 0.4f);
        SDL_Rect haloRect = itemRect;
        haloRect.x -= colony::ui::Scale(6);
        haloRect.w += colony::ui::Scale(12);
        haloRect.y -= colony::ui::Scale(4);
        haloRect.h += colony::ui::Scale(8);
        SDL_SetRenderDrawColor(renderer, halo.r, halo.g, halo.b, static_cast<Uint8>(80 + pulse * 60.0f));
        colony::drawing::RenderFilledRoundedRect(renderer, haloRect, radius + colony::ui::Scale(6));
    }

    return itemRect;
}

} // namespace colony::frontend::components
