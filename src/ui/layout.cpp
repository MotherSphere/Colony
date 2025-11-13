#include "ui/layout.hpp"

#include "utils/color.hpp"
#include "utils/drawing.hpp"

#include <algorithm>

namespace colony::ui
{

void TopBar::Build(
    SDL_Renderer* renderer,
    TTF_Font* titleFont,
    TTF_Font* bodyFont,
    const ThemeColors& theme,
    const Typography& typography,
    std::string_view searchPlaceholder,
    std::string_view titleText)
{
    titleFont_ = titleFont;
    titleColor_ = theme.heroTitle;
    UpdateTitle(renderer, titleText, theme.heroTitle);
    searchField_.Build(renderer, bodyFont, searchPlaceholder, theme);
}

void TopBar::UpdateTitle(SDL_Renderer* renderer, std::string_view titleText, SDL_Color titleColor)
{
    if (renderer == nullptr || titleFont_ == nullptr)
    {
        return;
    }

    if (currentTitle_ == titleText && titleTexture_.texture && titleColor_.r == titleColor.r
        && titleColor_.g == titleColor.g && titleColor_.b == titleColor.b && titleColor_.a == titleColor.a)
    {
        return;
    }

    currentTitle_ = std::string{titleText};
    titleColor_ = titleColor;
    titleTexture_ = colony::CreateTextTexture(renderer, titleFont_, currentTitle_, titleColor_);
}

TopBar::RenderResult TopBar::Render(
    SDL_Renderer* renderer,
    const ThemeColors& theme,
    const Typography& typography,
    const InteractionColors& interactions,
    const SDL_Rect& bounds,
    std::string_view searchValue,
    bool searchFocused,
    double timeSeconds) const
{
    TopBar::RenderResult result;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, theme.surface.r, theme.surface.g, theme.surface.b, 230);
    SDL_RenderFillRect(renderer, &bounds);
    SDL_SetRenderDrawColor(renderer, theme.divider.r, theme.divider.g, theme.divider.b, 160);
    SDL_RenderDrawLine(renderer, bounds.x, bounds.y + bounds.h - 1, bounds.x + bounds.w, bounds.y + bounds.h - 1);

    const int padding = Scale(20);
    int cursorX = bounds.x + padding;
    int cursorY = bounds.y + padding / 2;

    if (titleTexture_.texture)
    {
        SDL_Rect titleRect{cursorX, cursorY, titleTexture_.width, titleTexture_.height};
        colony::RenderTexture(renderer, titleTexture_, titleRect);
        cursorY += titleRect.h + Scale(8);
    }

    const int searchWidth = std::max(bounds.w / 2, Scale(320));
    SDL_Rect searchBounds{
        cursorX,
        bounds.y + bounds.h - Scale(52),
        searchWidth,
        Scale(40)};

    auto searchResult = searchField_.Render(renderer, theme, interactions, searchBounds, searchValue, searchFocused, timeSeconds);
    result.searchFieldRect = searchResult.inputRect;

    const int actionsCenterY = bounds.y + bounds.h / 2;
    const int buttonSize = Scale(40);
    SDL_Rect bellButton{bounds.x + bounds.w - padding - buttonSize * 2 - Scale(12), actionsCenterY - buttonSize / 2, buttonSize, buttonSize};
    SDL_Rect profileButton{bounds.x + bounds.w - padding - buttonSize, actionsCenterY - buttonSize / 2, buttonSize, buttonSize};

    SDL_Color bellColor = colony::color::Mix(theme.buttonGhost, theme.channelBadge, 0.3f);
    SDL_SetRenderDrawColor(renderer, bellColor.r, bellColor.g, bellColor.b, 220);
    colony::drawing::RenderFilledRoundedRect(renderer, bellButton, buttonSize / 2);
    SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, 180);
    colony::drawing::RenderRoundedRect(renderer, bellButton, buttonSize / 2);

    SDL_Rect clapper{bellButton.x + buttonSize / 2 - Scale(4), bellButton.y + buttonSize / 2 - Scale(4), Scale(8), Scale(12)};
    SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &clapper);

    SDL_Color profileFill = colony::color::Mix(theme.buttonPrimary, theme.buttonGhost, 0.4f);
    SDL_SetRenderDrawColor(renderer, profileFill.r, profileFill.g, profileFill.b, 230);
    colony::drawing::RenderFilledRoundedRect(renderer, profileButton, buttonSize / 2);
    SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, 180);
    colony::drawing::RenderRoundedRect(renderer, profileButton, buttonSize / 2);

    SDL_Rect profileInitial{
        profileButton.x + profileButton.w / 2 - Scale(6),
        profileButton.y + profileButton.h / 2 - Scale(6),
        Scale(12),
        Scale(12)};
    colony::drawing::RenderFilledRoundedRect(renderer, profileInitial, Scale(6));

    result.profileButtonRect = profileButton;
    return result;
}

} // namespace colony::ui
