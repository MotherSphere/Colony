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

    const int cornerRadius = colony::ui::Scale(16);
    SDL_Color cardFill = colony::color::Mix(theme.libraryCard, theme.libraryBackground, 0.5f);
    SDL_Color borderColor = colony::color::Mix(theme.border, theme.libraryCardActive, 0.3f);

    SDL_SetRenderDrawColor(renderer, cardFill.r, cardFill.g, cardFill.b, cardFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, bounds, cornerRadius);
    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    colony::drawing::RenderRoundedRect(renderer, bounds, cornerRadius);

    const int iconSize = std::min(bounds.w, bounds.h) / 6;
    const int iconRadius = std::max(colony::ui::Scale(12), iconSize / 2);
    SDL_Rect iconRect{
        bounds.x + colony::ui::Scale(28),
        bounds.y + colony::ui::Scale(28),
        std::max(iconSize, colony::ui::Scale(48)),
        std::max(iconSize, colony::ui::Scale(48))};

    const float pulse = static_cast<float>(0.5 + 0.5 * std::sin(timeSeconds * 1.5));
    SDL_Color iconFill = colony::color::Mix(theme.libraryCardActive, theme.libraryBackground, 0.35f + 0.25f * pulse);
    SDL_Color iconStroke = colony::color::Mix(theme.channelBadge, theme.libraryCardActive, 0.4f);

    SDL_SetRenderDrawColor(renderer, iconFill.r, iconFill.g, iconFill.b, iconFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, iconRect, iconRadius);
    SDL_SetRenderDrawColor(renderer, iconStroke.r, iconStroke.g, iconStroke.b, iconStroke.a);
    colony::drawing::RenderRoundedRect(renderer, iconRect, iconRadius);

    const int glyphPadding = colony::ui::Scale(10);
    SDL_Rect glyphRect{
        iconRect.x + glyphPadding,
        iconRect.y + glyphPadding,
        iconRect.w - glyphPadding * 2,
        iconRect.h - glyphPadding * 2};
    SDL_SetRenderDrawColor(renderer, iconStroke.r, iconStroke.g, iconStroke.b, iconStroke.a);
    SDL_RenderDrawLine(renderer, glyphRect.x, glyphRect.y + glyphRect.h, glyphRect.x + glyphRect.w, glyphRect.y);
    SDL_RenderDrawLine(
        renderer,
        glyphRect.x + glyphRect.w / 2,
        glyphRect.y,
        glyphRect.x + glyphRect.w / 2,
        glyphRect.y + glyphRect.h);

    const int textStartX = iconRect.x + iconRect.w + colony::ui::Scale(20);
    const int textWidth = bounds.x + bounds.w - textStartX - colony::ui::Scale(24);
    const int textTop = iconRect.y;

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
            textTop + titleTexture.height + colony::ui::Scale(8),
            std::min(messageTexture.width, textWidth),
            messageTexture.height};
        colony::RenderTexture(renderer, messageTexture, messageRect);
    }
}

} // namespace colony::frontend::components
