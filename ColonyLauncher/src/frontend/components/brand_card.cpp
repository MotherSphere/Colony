#include "frontend/components/brand_card.hpp"

#include "frontend/components/badge.hpp"
#include "frontend/components/buttons.hpp"
#include "ui/layout.hpp"
#include "utils/color.hpp"
#include "utils/drawing.hpp"

#include <algorithm>
#include <cmath>

namespace colony::frontend::components
{

void BrandCard::Build(
    SDL_Renderer* renderer,
    const Content& content,
    TTF_Font* titleFont,
    TTF_Font* subtitleFont,
    TTF_Font* labelFont,
    const colony::ui::ThemeColors& theme)
{
    content_ = content;
    titleTexture_ = colony::CreateTextTexture(renderer, titleFont, content.title, theme.heroTitle);
    subtitleTexture_ = colony::CreateTextTexture(renderer, subtitleFont, content.subtitle, theme.muted);
    categoryTexture_ = colony::CreateTextTexture(renderer, labelFont, content.category, theme.navTextMuted);
    metricTexture_ = colony::CreateTextTexture(renderer, labelFont, content.metric, theme.statusBarText);
    highlightChips_.clear();
    highlightChips_.reserve(content.highlights.size());
    if (labelFont != nullptr)
    {
        for (const auto& highlight : content.highlights)
        {
            if (highlight.empty())
            {
                continue;
            }

            HighlightChip chip;
            chip.label = highlight;
            chip.texture = colony::CreateTextTexture(renderer, labelFont, highlight, theme.navText);
            if (chip.texture.texture)
            {
                highlightChips_.push_back(std::move(chip));
            }
        }
    }
}

SDL_Rect BrandCard::Render(
    SDL_Renderer* renderer,
    const colony::ui::ThemeColors& theme,
    const colony::ui::InteractionColors& interactions,
    const SDL_Rect& bounds,
    TTF_Font* buttonFont,
    TTF_Font* labelFont,
    bool hovered,
    bool active,
    double timeSeconds) const
{
    SDL_Rect cardRect = bounds;
    const int radius = colony::ui::Scale(20);

    SDL_Color baseFill = colony::color::Mix(theme.card, theme.elevatedSurface, 0.45f);
    if (hovered)
    {
        baseFill = colony::color::Mix(theme.cardHover, baseFill, 0.6f);
    }
    if (active)
    {
        baseFill = colony::color::Mix(theme.cardActive, baseFill, 0.7f);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, baseFill.r, baseFill.g, baseFill.b, 240);
    colony::drawing::RenderFilledRoundedRect(renderer, cardRect, radius);
    SDL_SetRenderDrawColor(renderer, theme.divider.r, theme.divider.g, theme.divider.b, 180);
    colony::drawing::RenderRoundedRect(renderer, cardRect, radius);

    const int padding = colony::ui::Scale(22);
    int cursorX = cardRect.x + padding;
    int cursorY = cardRect.y + padding;

    const int avatarSize = colony::ui::Scale(48);
    SDL_Rect avatarRect{cursorX, cursorY, avatarSize, avatarSize};
    SDL_SetRenderDrawColor(renderer, content_.accent.r, content_.accent.g, content_.accent.b, SDL_ALPHA_OPAQUE);
    colony::drawing::RenderFilledRoundedRect(renderer, avatarRect, avatarSize / 2);
    SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, 180);
    colony::drawing::RenderRoundedRect(renderer, avatarRect, avatarSize / 2);

    cursorX += avatarSize + colony::ui::Scale(18);

    if (titleTexture_.texture)
    {
        SDL_Rect titleRect{cursorX, cursorY, titleTexture_.width, titleTexture_.height};
        colony::RenderTexture(renderer, titleTexture_, titleRect);
        cursorY += titleRect.h + colony::ui::Scale(4);
    }

    if (subtitleTexture_.texture)
    {
        SDL_Rect subtitleRect{cursorX, cursorY, subtitleTexture_.width, subtitleTexture_.height};
        colony::RenderTexture(renderer, subtitleTexture_, subtitleRect);
        cursorY += subtitleRect.h + colony::ui::Scale(10);
    }

    if (!content_.statusLabel.empty())
    {
        const int badgeHeight = colony::ui::Scale(24);
        const int badgeWidth = std::max(colony::ui::Scale(72), colony::ui::Scale(16) + static_cast<int>(content_.statusLabel.size()));
        SDL_Rect badgeRect{cursorX, cursorY, badgeWidth, badgeHeight};
        SDL_Color badgeFill = content_.ready ? colony::color::Mix(theme.success, content_.accent, 0.35f)
                                             : colony::color::Mix(theme.warning, content_.accent, 0.25f);
        SDL_Color badgeText = theme.heroTitle;
        RenderBadge(renderer, theme, badgeRect, content_.statusLabel, labelFont, badgeFill, badgeText);

        cursorY += badgeRect.h + colony::ui::Scale(12);
    }

    if (categoryTexture_.texture)
    {
        SDL_Rect categoryRect{cursorX, cursorY, categoryTexture_.width, categoryTexture_.height};
        colony::RenderTexture(renderer, categoryTexture_, categoryRect);
        cursorY += categoryRect.h + colony::ui::Scale(6);
    }

    if (!highlightChips_.empty())
    {
        int chipX = cursorX;
        int chipY = cursorY;
        const int chipPadX = colony::ui::Scale(10);
        const int chipPadY = colony::ui::Scale(6);
        const int chipSpacing = colony::ui::Scale(8);
        const int lineSpacing = colony::ui::Scale(8);
        const int maxContentX = cardRect.x + cardRect.w - padding;

        for (const auto& chip : highlightChips_)
        {
            if (!chip.texture.texture)
            {
                continue;
            }

            const int chipWidth = chip.texture.width + chipPadX * 2;
            const int chipHeight = chip.texture.height + chipPadY * 2;
            if (chipX + chipWidth > maxContentX)
            {
                chipX = cursorX;
                chipY += chipHeight + lineSpacing;
            }

            SDL_Rect chipRect{chipX, chipY, chipWidth, chipHeight};
            SDL_Color chipFill = colony::color::Mix(content_.accent, theme.buttonGhost, 0.55f);
            chipFill.a = 220;
            SDL_Color chipOutline = colony::color::Mix(theme.border, chipFill, 0.35f);
            SDL_SetRenderDrawColor(renderer, chipFill.r, chipFill.g, chipFill.b, chipFill.a);
            colony::drawing::RenderFilledRoundedRect(renderer, chipRect, chipHeight / 2);
            SDL_SetRenderDrawColor(renderer, chipOutline.r, chipOutline.g, chipOutline.b, 200);
            colony::drawing::RenderRoundedRect(renderer, chipRect, chipHeight / 2);

            SDL_Rect chipTextRect{
                chipRect.x + chipPadX,
                chipRect.y + chipPadY,
                chip.texture.width,
                chip.texture.height};
            colony::RenderTexture(renderer, chip.texture, chipTextRect);

            chipX += chipWidth + chipSpacing;
            cursorY = std::max(cursorY, chipRect.y + chipRect.h);
        }

        cursorY += lineSpacing;
    }

    cursorY += colony::ui::Scale(4);

    if (metricTexture_.texture)
    {
        SDL_Rect metricRect{cursorX, cursorY, metricTexture_.width, metricTexture_.height};
        colony::RenderTexture(renderer, metricTexture_, metricRect);
    }

    const int buttonHeight = colony::ui::Scale(36);
    const int buttonSpacing = colony::ui::Scale(12);
    const int buttonWidth = colony::ui::Scale(128);
    const int buttonY = cardRect.y + cardRect.h - padding - buttonHeight;
    int buttonX = cardRect.x + padding;

    if (!content_.primaryActionLabel.empty())
    {
        SDL_Rect primaryButtonRect{buttonX, buttonY, buttonWidth, buttonHeight};
        RenderPrimaryButton(renderer, theme, primaryButtonRect, content_.primaryActionLabel, buttonFont, hovered, active);
        buttonX += buttonWidth + buttonSpacing;
    }

    if (!content_.secondaryActionLabel.empty())
    {
        SDL_Rect secondaryButtonRect{buttonX, buttonY, buttonWidth, buttonHeight};
        RenderSecondaryButton(renderer, theme, secondaryButtonRect, content_.secondaryActionLabel, buttonFont, hovered, false);
    }

    if (!content_.metricBadgeLabel.empty())
    {
        SDL_Rect metricBadgeRect{
            cardRect.x + cardRect.w - padding - colony::ui::Scale(88),
            buttonY,
            colony::ui::Scale(88),
            buttonHeight};
        SDL_Color metricFill = colony::color::Mix(theme.info, content_.accent, 0.25f);
        RenderBadge(renderer, theme, metricBadgeRect, content_.metricBadgeLabel, labelFont, metricFill, theme.heroTitle);
    }

    if (hovered)
    {
        SDL_Color glow = interactions.hover;
        glow.a = 60;
        SDL_Rect halo = cardRect;
        halo.x -= colony::ui::Scale(6);
        halo.y -= colony::ui::Scale(6);
        halo.w += colony::ui::Scale(12);
        halo.h += colony::ui::Scale(12);
        colony::drawing::RenderRoundedRect(renderer, halo, radius + colony::ui::Scale(4));
    }

    if (active)
    {
        const float pulse = static_cast<float>(std::sin(timeSeconds * 2.8) * 0.5 + 0.5);
        SDL_Color activeGlow = colony::color::Mix(content_.accent, theme.heroTitle, 0.35f);
        SDL_Rect halo = cardRect;
        halo.x -= colony::ui::Scale(8);
        halo.y -= colony::ui::Scale(8);
        halo.w += colony::ui::Scale(16);
        halo.h += colony::ui::Scale(16);
        SDL_SetRenderDrawColor(renderer, activeGlow.r, activeGlow.g, activeGlow.b, static_cast<Uint8>(90 + 60 * pulse));
        colony::drawing::RenderRoundedRect(renderer, halo, radius + colony::ui::Scale(6));
    }

    return cardRect;
}

} // namespace colony::frontend::components
