#include "ui/navigation.hpp"

#include "utils/color.hpp"

#include <algorithm>

namespace colony::ui
{

void NavigationRail::Build(
    SDL_Renderer* renderer,
    TTF_Font* brandFont,
    TTF_Font* navFont,
    TTF_Font* metaFont,
    const colony::AppContent& content,
    const ThemeColors& theme)
{
    chrome_.brand = colony::CreateTextTexture(renderer, brandFont, content.brandName, theme.heroTitle);
    chrome_.channelLabels.clear();
    chrome_.channelLabels.reserve(content.channels.size());
    for (const auto& channel : content.channels)
    {
        chrome_.channelLabels.emplace_back(colony::CreateTextTexture(renderer, navFont, channel.label, theme.navText));
    }
    chrome_.userName = colony::CreateTextTexture(renderer, navFont, content.user.name, theme.heroTitle);
    chrome_.userStatus = colony::CreateTextTexture(renderer, metaFont, content.user.status, theme.muted);
}

std::vector<SDL_Rect> NavigationRail::Render(
    SDL_Renderer* renderer,
    const ThemeColors& theme,
    const SDL_Rect& navRailRect,
    int statusBarHeight,
    const colony::AppContent& content,
    const std::vector<int>& channelSelections,
    int activeChannelIndex,
    const std::unordered_map<std::string, ProgramVisuals>& programVisuals) const
{
    std::vector<SDL_Rect> buttonRects(content.channels.size());

    const int navPadding = 28;
    if (chrome_.brand.texture)
    {
        SDL_Rect brandRect{
            navRailRect.x + (navRailRect.w - chrome_.brand.width) / 2,
            navPadding,
            chrome_.brand.width,
            chrome_.brand.height};
        colony::RenderTexture(renderer, chrome_.brand, brandRect);
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

    int channelStartY = navPadding + (chrome_.brand.height > 0 ? chrome_.brand.height + 32 : 48);
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

        if (i < chrome_.channelLabels.size() && chrome_.channelLabels[i].texture)
        {
            SDL_Rect labelRect{
                navRailRect.x + (navRailRect.w - chrome_.channelLabels[i].width) / 2,
                buttonRect.y + buttonRect.h + 6,
                chrome_.channelLabels[i].width,
                chrome_.channelLabels[i].height};
            colony::RenderTexture(renderer, chrome_.channelLabels[i], labelRect);
        }

        buttonRects[i] = buttonRect;
        channelStartY += channelButtonSize + channelSpacing;
    }

    if (chrome_.userName.texture)
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
            navRailRect.x + (navRailRect.w - chrome_.userName.width) / 2,
            avatarRect.y + avatarRect.h + 8,
            chrome_.userName.width,
            chrome_.userName.height};
        colony::RenderTexture(renderer, chrome_.userName, nameRect);

        if (chrome_.userStatus.texture)
        {
            SDL_Rect statusRect{
                navRailRect.x + (navRailRect.w - chrome_.userStatus.width) / 2,
                nameRect.y + nameRect.h + 4,
                chrome_.userStatus.width,
                chrome_.userStatus.height};
            colony::RenderTexture(renderer, chrome_.userStatus, statusRect);
        }
    }

    return buttonRects;
}

} // namespace colony::ui
