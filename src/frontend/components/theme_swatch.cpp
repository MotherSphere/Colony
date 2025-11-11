#include "frontend/components/theme_swatch.hpp"

#include "ui/layout.hpp"
#include "utils/color.hpp"
#include "utils/drawing.hpp"
#include "utils/text.hpp"

#include <algorithm>
#include <cmath>

namespace colony::frontend::components
{
namespace
{
int ScaleValue(int value)
{
    return colony::ui::Scale(value);
}

SDL_Color Mix(const SDL_Color& a, const SDL_Color& b, float t)
{
    return colony::color::Mix(a, b, std::clamp(t, 0.0f, 1.0f));
}
}

void RenderThemeSwatch(
    SDL_Renderer* renderer,
    const colony::ui::ThemeColors& theme,
    const SDL_Rect& bounds,
    TTF_Font* headingFont,
    TTF_Font* bodyFont,
    const ThemeSwatchText& text,
    const ThemeSwatchStyle& style)
{
    if (renderer == nullptr || headingFont == nullptr || bodyFont == nullptr)
    {
        return;
    }

    if (bounds.w <= 0 || bounds.h <= 0)
    {
        return;
    }

    const int cornerRadius = ScaleValue(style.cornerRadius);
    const int padding = ScaleValue(style.padding);

    SDL_Color outerFill = Mix(theme.libraryBackground, theme.background, 0.5f);
    SDL_Color outerStroke = Mix(theme.border, theme.libraryCardActive, 0.35f);

    SDL_SetRenderDrawColor(renderer, outerFill.r, outerFill.g, outerFill.b, outerFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, bounds, cornerRadius);
    SDL_SetRenderDrawColor(renderer, outerStroke.r, outerStroke.g, outerStroke.b, outerStroke.a);
    colony::drawing::RenderRoundedRect(renderer, bounds, cornerRadius);

    const SDL_Rect previewBounds{
        bounds.x + padding,
        bounds.y + padding,
        bounds.w - padding * 2,
        bounds.h - padding * 2};

    const int previewHeaderHeight = std::max(ScaleValue(20), previewBounds.h / 5);
    SDL_Rect headerRect{previewBounds.x, previewBounds.y, previewBounds.w, previewHeaderHeight};

    SDL_Color headerFill = Mix(theme.libraryCardActive, theme.libraryCard, 0.6f);
    SDL_SetRenderDrawColor(renderer, headerFill.r, headerFill.g, headerFill.b, headerFill.a);
    SDL_RenderFillRect(renderer, &headerRect);

    const float accentPulse = std::clamp(style.accentPulse, 0.0f, 1.0f);
    const int accentHeight = std::max(ScaleValue(4), previewHeaderHeight / 6);
    SDL_Rect accentRect{headerRect.x, headerRect.y + headerRect.h - accentHeight, headerRect.w, accentHeight};
    SDL_Color accentColor = Mix(theme.channelBadge, theme.heroTitle, 0.5f + 0.5f * accentPulse);
    SDL_SetRenderDrawColor(renderer, accentColor.r, accentColor.g, accentColor.b, accentColor.a);
    SDL_RenderFillRect(renderer, &accentRect);

    const int previewBodyHeight = previewBounds.h - previewHeaderHeight - ScaleValue(12);
    SDL_Rect cardRect{
        previewBounds.x,
        headerRect.y + headerRect.h + ScaleValue(8),
        previewBounds.w,
        std::max(previewBodyHeight, ScaleValue(42))};

    SDL_Color cardFill = Mix(theme.libraryCard, theme.background, 0.55f);
    SDL_Color cardStroke = Mix(theme.border, theme.libraryCardHover, 0.5f);

    SDL_SetRenderDrawColor(renderer, cardFill.r, cardFill.g, cardFill.b, cardFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, cardRect, ScaleValue(10));
    SDL_SetRenderDrawColor(renderer, cardStroke.r, cardStroke.g, cardStroke.b, cardStroke.a);
    colony::drawing::RenderRoundedRect(renderer, cardRect, ScaleValue(10));

    const int cardPadding = ScaleValue(12);
    SDL_Rect sampleHeadingRect{
        cardRect.x + cardPadding,
        cardRect.y + cardPadding,
        cardRect.w - cardPadding * 2,
        cardRect.h / 3};

    SDL_Color headingColor = theme.heroTitle;
    SDL_Color bodyColor = theme.heroBody;

    colony::TextTexture headingTexture = colony::CreateTextTexture(renderer, headingFont, text.heading, headingColor);
    colony::TextTexture bodyTexture = colony::CreateTextTexture(renderer, bodyFont, text.body, bodyColor);

    if (headingTexture.texture)
    {
        SDL_Rect drawHeadingRect{
            sampleHeadingRect.x,
            sampleHeadingRect.y,
            std::min(headingTexture.width, sampleHeadingRect.w),
            headingTexture.height};
        colony::RenderTexture(renderer, headingTexture, drawHeadingRect);
    }

    if (bodyTexture.texture)
    {
        SDL_Rect drawBodyRect{
            sampleHeadingRect.x,
            sampleHeadingRect.y + (headingTexture.texture ? headingTexture.height + ScaleValue(6) : 0),
            std::min(bodyTexture.width, sampleHeadingRect.w),
            bodyTexture.height};
        colony::RenderTexture(renderer, bodyTexture, drawBodyRect);
    }

    const int rowHeight = ScaleValue(8);
    const int rowSpacing = ScaleValue(6);
    SDL_Rect indicatorRect{
        cardRect.x + cardPadding,
        cardRect.y + cardRect.h - rowHeight * 2 - rowSpacing,
        cardRect.w - cardPadding * 2,
        rowHeight};

    SDL_Color rowColor = Mix(theme.libraryCardActive, theme.background, 0.65f);
    SDL_SetRenderDrawColor(renderer, rowColor.r, rowColor.g, rowColor.b, rowColor.a);
    SDL_RenderFillRect(renderer, &indicatorRect);

    indicatorRect.y += rowHeight + rowSpacing;
    SDL_Color mutedRowColor = Mix(theme.muted, theme.libraryCard, 0.5f);
    SDL_SetRenderDrawColor(renderer, mutedRowColor.r, mutedRowColor.g, mutedRowColor.b, mutedRowColor.a);
    SDL_RenderFillRect(renderer, &indicatorRect);
}

} // namespace colony::frontend::components
