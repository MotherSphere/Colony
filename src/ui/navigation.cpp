#include "ui/navigation.hpp"

#include "utils/color.hpp"

#include <algorithm>

namespace colony::ui
{

NavigationChrome BuildNavigationChrome(
    SDL_Renderer* renderer,
    TTF_Font* brandFont,
    TTF_Font* navFont,
    TTF_Font* metaFont,
    const colony::AppContent& content,
    const ThemeColors& theme)
{
    NavigationChrome chrome;
    chrome.brand = colony::CreateTextTexture(renderer, brandFont, content.brandName, theme.heroTitle);
    chrome.channelLabels.reserve(content.channels.size());
    for (const auto& channel : content.channels)
    {
        chrome.channelLabels.emplace_back(colony::CreateTextTexture(renderer, navFont, channel.label, theme.navText));
    }
    chrome.userName = colony::CreateTextTexture(renderer, navFont, content.user.name, theme.heroTitle);
    chrome.userStatus = colony::CreateTextTexture(renderer, metaFont, content.user.status, theme.muted);
    return chrome;
}

std::vector<SDL_Rect> RenderNavigationRail(
    SDL_Renderer* renderer,
    const ThemeColors& theme,
    const SDL_Rect& navRailRect,
    int statusBarHeight,
    const NavigationChrome& chrome,
    const colony::AppContent& content,
    const std::vector<int>& channelSelections,
    int activeChannelIndex,
    const std::unordered_map<std::string, ProgramVisuals>& programVisuals)
{
    std::vector<SDL_Rect> buttonRects(content.channels.size());

    const int navPadding = 28;
    if (chrome.brand.texture)
    {
        SDL_Rect brandRect{
            navRailRect.x + (navRailRect.w - chrome.brand.width) / 2,
            navPadding,
            chrome.brand.width,
            chrome.brand.height};
        colony::RenderTexture(renderer, chrome.brand, brandRect);
    }

    auto channelAccentColor = [&](int index) {
        const auto& channel = content.channels[index];
        if (channel.programs.empty())
        {
            return theme.channelBadge;
        }
        const auto& programs = channel.programs;
        const int selected = programs.empty() ? 0 : std::clamp(channelSelections[index], 0, static_cast<int>(programs.size()) - 1);
        const std::string& programId = programs[selected];
        if (const auto it = programVisuals.find(programId); it != programVisuals.end())
        {
            return colony::color::Mix(it->second.accent, theme.channelBadge, 0.25f);
        }
        return theme.channelBadge;
    };

    int channelStartY = navPadding + (chrome.brand.height > 0 ? chrome.brand.height + 32 : 48);
    const int channelButtonSize = 48;
    const int channelSpacing = 32;

    for (std::size_t i = 0; i < content.channels.size(); ++i)
    {
        const bool isActive = static_cast<int>(i) == activeChannelIndex;
        SDL_Rect buttonRect{
            navRailRect.x + (navRailRect.w - channelButtonSize) / 2,
            channelStartY,
            channelButtonSize,
            channelButtonSize};

        SDL_Color baseColor = channelAccentColor(static_cast<int>(i));
        SDL_Color fillColor = isActive ? colony::color::Mix(baseColor, theme.heroTitle, 0.15f) : baseColor;
        SDL_SetRenderDrawColor(renderer, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
        SDL_RenderFillRect(renderer, &buttonRect);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        SDL_RenderDrawRect(renderer, &buttonRect);

        if (i < chrome.channelLabels.size() && chrome.channelLabels[i].texture)
        {
            SDL_Rect labelRect{
                navRailRect.x + (navRailRect.w - chrome.channelLabels[i].width) / 2,
                buttonRect.y + buttonRect.h + 6,
                chrome.channelLabels[i].width,
                chrome.channelLabels[i].height};
            colony::RenderTexture(renderer, chrome.channelLabels[i], labelRect);
        }

        buttonRects[i] = buttonRect;
        channelStartY += channelButtonSize + channelSpacing;
    }

    if (chrome.userName.texture)
    {
        const int avatarSize = 14;
        SDL_Rect avatarRect{
            navRailRect.x + (navRailRect.w - avatarSize) / 2,
            navRailRect.y + navRailRect.h - statusBarHeight - 40,
            avatarSize,
            avatarSize};
        SDL_SetRenderDrawColor(renderer, 90, 214, 102, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer, &avatarRect);

        SDL_Rect nameRect{
            navRailRect.x + (navRailRect.w - chrome.userName.width) / 2,
            avatarRect.y + avatarRect.h + 8,
            chrome.userName.width,
            chrome.userName.height};
        colony::RenderTexture(renderer, chrome.userName, nameRect);

        if (chrome.userStatus.texture)
        {
            SDL_Rect statusRect{
                navRailRect.x + (navRailRect.w - chrome.userStatus.width) / 2,
                nameRect.y + nameRect.h + 4,
                chrome.userStatus.width,
                chrome.userStatus.height};
            colony::RenderTexture(renderer, chrome.userStatus, statusRect);
        }
    }

    return buttonRects;
}

} // namespace colony::ui
