#include "frontend/components/empty_state_card.hpp"

#include "ui/layout.hpp"
#include "utils/color.hpp"
#include "utils/drawing.hpp"
#include "utils/text.hpp"

#include <algorithm>
#include <cmath>

namespace colony::frontend::components
{

void RenderEmptyStateCard(
    SDL_Renderer* renderer,
    const colony::ui::ThemeColors& theme,
    const SDL_Rect& bounds,
    TTF_Font* titleFont,
    TTF_Font* bodyFont,
    std::string_view title,
    std::string_view message,
    double timeSeconds)
{
    if (renderer == nullptr || titleFont == nullptr || bodyFont == nullptr)
    {
        return;
    }

    if (bounds.w <= 0 || bounds.h <= 0)
    {
        return;
    }

    const int cornerRadius = colony::ui::Scale(18);
    SDL_Color cardFill = colony::color::Mix(theme.libraryCardActive, theme.libraryBackground, 0.4f);
    SDL_Color borderColor = colony::color::Mix(theme.border, theme.heroTitle, 0.25f);

    SDL_Rect shadowRect = bounds;
    shadowRect.y += colony::ui::Scale(4);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, 45);
    colony::drawing::RenderFilledRoundedRect(renderer, shadowRect, cornerRadius + colony::ui::Scale(4));
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    SDL_SetRenderDrawColor(renderer, cardFill.r, cardFill.g, cardFill.b, cardFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, bounds, cornerRadius);
    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    colony::drawing::RenderRoundedRect(renderer, bounds, cornerRadius);

    const int iconSize = std::max(colony::ui::Scale(56), std::min(bounds.w, bounds.h) / 5);
    const int iconRadius = std::max(colony::ui::Scale(18), iconSize / 2);
    SDL_Rect iconRect{
        bounds.x + colony::ui::Scale(32),
        bounds.y + colony::ui::Scale(32),
        iconSize,
        iconSize};

    const float pulse = static_cast<float>(0.55f + 0.45f * std::sin(timeSeconds * 1.5f));
    SDL_Color iconFill = colony::color::Mix(theme.channelBadge, theme.libraryCardActive, 0.4f + 0.2f * pulse);
    SDL_Color iconStroke = colony::color::Mix(theme.channelBadge, theme.heroTitle, 0.25f);

    SDL_SetRenderDrawColor(renderer, iconFill.r, iconFill.g, iconFill.b, iconFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, iconRect, iconRadius);
    SDL_SetRenderDrawColor(renderer, iconStroke.r, iconStroke.g, iconStroke.b, iconStroke.a);
    colony::drawing::RenderRoundedRect(renderer, iconRect, iconRadius);

    const int glyphPadding = colony::ui::Scale(12);
    SDL_Rect glyphRect{
        iconRect.x + glyphPadding,
        iconRect.y + glyphPadding,
        iconRect.w - glyphPadding * 2,
        iconRect.h - glyphPadding * 2};
    SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, theme.heroTitle.a);
    colony::drawing::RenderRoundedRect(renderer, glyphRect, glyphPadding / 2);
    SDL_RenderDrawLine(renderer, glyphRect.x, glyphRect.y + glyphRect.h, glyphRect.x + glyphRect.w, glyphRect.y);
    SDL_RenderDrawLine(
        renderer,
        glyphRect.x + glyphRect.w / 2,
        glyphRect.y + colony::ui::Scale(4),
        glyphRect.x + glyphRect.w / 2,
        glyphRect.y + glyphRect.h - colony::ui::Scale(4));

    const int textStartX = iconRect.x + iconRect.w + colony::ui::Scale(28);
    const int textWidth = bounds.x + bounds.w - textStartX - colony::ui::Scale(32);
    const int textTop = iconRect.y + colony::ui::Scale(4);

    colony::TextTexture titleTexture = colony::CreateTextTexture(renderer, titleFont, title, theme.heroTitle);
    colony::TextTexture messageTexture = colony::CreateTextTexture(renderer, bodyFont, message, theme.muted);

    if (titleTexture.texture)
    {
        SDL_Rect titleRect{
            textStartX,
            textTop,
            std::min(titleTexture.width, textWidth),
            titleTexture.height};
        colony::RenderTexture(renderer, titleTexture, titleRect);
    }

    if (messageTexture.texture)
    {
        SDL_Rect messageRect{
            textStartX,
            textTop + titleTexture.height + colony::ui::Scale(12),
            std::min(messageTexture.width, textWidth),
            messageTexture.height};
        colony::RenderTexture(renderer, messageTexture, messageRect);
    }
}

} // namespace colony::frontend::components
