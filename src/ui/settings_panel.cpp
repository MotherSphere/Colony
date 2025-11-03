#include "ui/settings_panel.hpp"

#include "utils/color.hpp"

#include <algorithm>

namespace colony::ui
{

void SettingsPanel::Build(
    SDL_Renderer* renderer,
    TTF_Font* titleFont,
    TTF_Font* bodyFont,
    SDL_Color titleColor,
    SDL_Color bodyColor,
    const ThemeManager& themeManager)
{
    options_.clear();

    title_ = colony::CreateTextTexture(renderer, titleFont, "Interface color scheme", titleColor);
    subtitle_ = colony::CreateTextTexture(renderer, bodyFont, "Choose a preset palette for Colony", bodyColor);

    for (const auto& scheme : themeManager.Schemes())
    {
        Option option;
        option.id = scheme.id;
        option.label = colony::CreateTextTexture(renderer, bodyFont, scheme.name, bodyColor);
        option.swatch = {scheme.colors.background, scheme.colors.libraryCard, scheme.colors.heroTitle};
        options_.emplace_back(std::move(option));
    }
}

SettingsPanel::RenderResult SettingsPanel::Render(
    SDL_Renderer* renderer,
    const SDL_Rect& bounds,
    const ThemeColors& theme,
    std::string_view activeSchemeId) const
{
    SettingsPanel::RenderResult result;

    int cursorY = bounds.y;
    const int horizontalPadding = 32;

    if (title_.texture)
    {
        SDL_Rect titleRect{bounds.x + horizontalPadding, cursorY, title_.width, title_.height};
        colony::RenderTexture(renderer, title_, titleRect);
        cursorY += titleRect.h + 12;
    }

    if (subtitle_.texture)
    {
        SDL_Rect subtitleRect{bounds.x + horizontalPadding, cursorY, subtitle_.width, subtitle_.height};
        colony::RenderTexture(renderer, subtitle_, subtitleRect);
        cursorY += subtitleRect.h + 24;
    }

    const int availableWidth = bounds.w - horizontalPadding * 2;
    const int cardSpacing = 20;
    const int columns = availableWidth >= 640 ? 2 : 1;
    const int cardWidth = columns == 1 ? availableWidth : (availableWidth - cardSpacing) / 2;
    const int cardHeight = 108;

    for (std::size_t index = 0; index < options_.size(); ++index)
    {
        const int column = columns == 1 ? 0 : static_cast<int>(index % columns);
        const int row = columns == 1 ? static_cast<int>(index) : static_cast<int>(index / columns);
        const int cardX = bounds.x + horizontalPadding + column * (cardWidth + cardSpacing);
        const int cardY = cursorY + row * (cardHeight + cardSpacing);
        SDL_Rect cardRect{cardX, cardY, cardWidth, cardHeight};

        bool isActive = options_[index].id == activeSchemeId;
        SDL_Color baseColor = isActive ? colony::color::Mix(theme.libraryCardActive, theme.heroTitle, 0.1f) : theme.libraryCard;
        SDL_Color borderColor = isActive ? theme.heroTitle : theme.border;

        SDL_SetRenderDrawColor(renderer, baseColor.r, baseColor.g, baseColor.b, baseColor.a);
        SDL_RenderFillRect(renderer, &cardRect);
        SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
        SDL_RenderDrawRect(renderer, &cardRect);

        const auto& option = options_[index];
        SDL_Rect labelRect{
            cardRect.x + 20,
            cardRect.y + 18,
            option.label.width,
            option.label.height};
        colony::RenderTexture(renderer, option.label, labelRect);

        const int swatchWidth = (cardRect.w - 40 - 12 * 2) / 3;
        const int swatchHeight = 28;
        int swatchX = cardRect.x + 20;
        const int swatchY = cardRect.y + cardRect.h - swatchHeight - 20;
        for (const SDL_Color& swatchColor : option.swatch)
        {
            SDL_Rect swatchRect{swatchX, swatchY, swatchWidth, swatchHeight};
            SDL_SetRenderDrawColor(renderer, swatchColor.r, swatchColor.g, swatchColor.b, swatchColor.a);
            SDL_RenderFillRect(renderer, &swatchRect);
            SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
            SDL_RenderDrawRect(renderer, &swatchRect);
            swatchX += swatchWidth + 12;
        }

        option.rect = cardRect;
        result.optionRects.emplace_back(option.id, cardRect);

        if (columns == 1)
        {
            cursorY += cardHeight + cardSpacing;
        }
        else if (column + 1 == columns)
        {
            cursorY = cardY + cardHeight + cardSpacing;
        }
    }

    result.contentHeight = cursorY - bounds.y;
    return result;
}

} // namespace colony::ui
