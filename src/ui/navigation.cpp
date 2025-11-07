#include "ui/navigation.hpp"

#include "ui/layout.hpp"

#include "utils/color.hpp"
#include "utils/drawing.hpp"

#include <algorithm>
#include <cmath>

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

    chrome_.settingsLabel = {};
    if (navFont)
    {
        const auto settingsIt = std::find_if(content.channels.begin(), content.channels.end(), [](const colony::Channel& channel) {
            return channel.id == "settings";
        });
        if (settingsIt != content.channels.end())
        {
            chrome_.settingsLabel = colony::CreateTextTexture(renderer, navFont, settingsIt->label, theme.navText);
        }
    }

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
    const std::unordered_map<std::string, ProgramVisuals>& programVisuals,
    double timeSeconds) const
{
    std::vector<SDL_Rect> buttonRects(content.channels.size());

    const int navPadding = Scale(20);
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

    int channelStartY = navPadding + (chrome_.brand.height > 0 ? chrome_.brand.height + Scale(24) : Scale(38));
    const int channelButtonSize = Scale(40);
    const int channelSpacing = Scale(26);

    const auto renderChannelButton = [&](int index, int y) {
        const bool isActive = static_cast<int>(index) == activeChannelIndex;
        const float wave = static_cast<float>(std::sin(timeSeconds * 1.4 + index));
        const int bobOffset = static_cast<int>(std::round(wave * Scale(3)));
        const float glow = static_cast<float>(0.35 + 0.35 * std::sin(timeSeconds * 2.0 + index));
        SDL_Rect buttonRect{
            navRailRect.x + (navRailRect.w - channelButtonSize) / 2,
            y + bobOffset,
            channelButtonSize,
            channelButtonSize};

        SDL_Color baseColor = channelAccentColor(index);
        SDL_Color fillColor = isActive ? colony::color::Mix(baseColor, theme.heroTitle, 0.25f + glow * 0.2f)
                                       : colony::color::Mix(baseColor, theme.navRail, 0.15f + glow * 0.15f);
        SDL_SetRenderDrawColor(renderer, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, buttonRect, 14);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        colony::drawing::RenderRoundedRect(renderer, buttonRect, 14);

        if (isActive)
        {
            SDL_Rect haloRect = buttonRect;
            haloRect.x -= Scale(4);
            haloRect.y -= Scale(4);
            haloRect.w += Scale(8);
            haloRect.h += Scale(8);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
            SDL_Color haloColor = colony::color::Mix(fillColor, theme.heroTitle, 0.5f);
            SDL_SetRenderDrawColor(renderer, haloColor.r, haloColor.g, haloColor.b, 48);
            colony::drawing::RenderFilledRoundedRect(renderer, haloRect, 18);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }

        if (content.channels[index].id == "settings" && chrome_.settingsLabel.texture)
        {
            SDL_Rect labelRect{
                buttonRect.x + (buttonRect.w - chrome_.settingsLabel.width) / 2,
                buttonRect.y + buttonRect.h + Scale(6),
                chrome_.settingsLabel.width,
                chrome_.settingsLabel.height};
            colony::RenderTexture(renderer, chrome_.settingsLabel, labelRect);
        }

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
        const int settingsLabelPadding = chrome_.settingsLabel.texture ? chrome_.settingsLabel.height + Scale(12) : 0;
        const int settingsTargetY =
            navRailRect.y + navRailRect.h - statusBarHeight - channelButtonSize - navPadding - settingsLabelPadding;
        const int settingsY = std::max(channelStartY, settingsTargetY);
        renderChannelButton(settingsChannelIndex, settingsY);
    }

    return buttonRects;
}

} // namespace colony::ui
