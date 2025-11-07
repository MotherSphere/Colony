#include "ui/navigation.hpp"

#include "utils/color.hpp"
#include "utils/drawing.hpp"

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

    (void)navFont;
    (void)metaFont;
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

    const auto renderChannelButton = [&](int index, int y) {
        const bool isActive = static_cast<int>(index) == activeChannelIndex;
        SDL_Rect buttonRect{
            navRailRect.x + (navRailRect.w - channelButtonSize) / 2,
            y,
            channelButtonSize,
            channelButtonSize};

        SDL_Color baseColor = channelAccentColor(index);
        SDL_Color fillColor = isActive ? colony::color::Mix(baseColor, theme.heroTitle, 0.15f) : baseColor;
        SDL_SetRenderDrawColor(renderer, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, buttonRect, 14);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        colony::drawing::RenderRoundedRect(renderer, buttonRect, 14);

        buttonRects[static_cast<std::size_t>(index)] = buttonRect;
        return y + channelButtonSize + channelSpacing;
    };

    int settingsChannelIndex = -1;
    for (std::size_t i = 0; i < content.channels.size(); ++i)
    {
        if (content.channels[i].id == "settings")
        {
            settingsChannelIndex = static_cast<int>(i);
            continue;
        }

        channelStartY = renderChannelButton(static_cast<int>(i), channelStartY);
    }

    if (settingsChannelIndex != -1)
    {
        const int settingsTargetY = navRailRect.y + navRailRect.h - statusBarHeight - channelButtonSize - navPadding;
        const int settingsY = std::max(channelStartY, settingsTargetY);
        renderChannelButton(settingsChannelIndex, settingsY);
    }

    return buttonRects;
}

} // namespace colony::ui
