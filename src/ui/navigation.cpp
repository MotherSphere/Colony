#include "ui/navigation.hpp"

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

    const int navPadding = 24;
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

    int channelStartY = navPadding + (chrome_.brand.height > 0 ? chrome_.brand.height + 26 : 40);
    const int channelButtonSize = 46;
    const int channelSpacing = 24;

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
            colony::drawing::RenderFilledRoundedRect(renderer, buttonRect, 14);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        colony::drawing::RenderRoundedRect(renderer, buttonRect, 14);

        SDL_Rect iconRect{buttonRect.x + 10, buttonRect.y + 10, buttonRect.w - 20, buttonRect.h - 20};
        SDL_Color iconColor = colony::color::Mix(theme.navText, fillColor, 0.25f);
        SDL_SetRenderDrawColor(renderer, iconColor.r, iconColor.g, iconColor.b, iconColor.a);
        colony::drawing::RenderRoundedRect(renderer, iconRect, 10);
        switch (i % 3)
        {
        case 0:
        {
            SDL_Point tip{iconRect.x + iconRect.w - 6, iconRect.y + iconRect.h / 2};
            SDL_Point baseTop{iconRect.x + 6, iconRect.y + 6};
            SDL_Point baseBottom{iconRect.x + 6, iconRect.y + iconRect.h - 6};
            SDL_RenderDrawLine(renderer, baseTop.x, baseTop.y, tip.x, tip.y);
            SDL_RenderDrawLine(renderer, tip.x, tip.y, baseBottom.x, baseBottom.y);
            SDL_RenderDrawLine(renderer, baseBottom.x, baseBottom.y, baseTop.x, baseTop.y);
            break;
        }
        case 1:
        {
            const int barWidth = 6;
            const int gap = 4;
            SDL_Rect bar1{iconRect.x + 4, iconRect.y + 6, barWidth, iconRect.h - 12};
            SDL_Rect bar2{bar1.x + barWidth + gap, iconRect.y + 4, barWidth, iconRect.h - 8};
            SDL_Rect bar3{bar2.x + barWidth + gap, iconRect.y + 10, barWidth, iconRect.h - 20};
            colony::drawing::RenderFilledRoundedRect(renderer, bar1, 3);
            colony::drawing::RenderFilledRoundedRect(renderer, bar2, 3);
            colony::drawing::RenderFilledRoundedRect(renderer, bar3, 3);
            break;
        }
        default:
        {
            SDL_Rect waveRect{iconRect.x + 6, iconRect.y + iconRect.h / 2 - 6, iconRect.w - 12, 12};
            colony::drawing::RenderRoundedRect(renderer, waveRect, 6);
            SDL_RenderDrawLine(
                renderer,
                waveRect.x,
                waveRect.y + waveRect.h / 2,
                waveRect.x + waveRect.w,
                waveRect.y + waveRect.h / 2 - 4);
            SDL_RenderDrawLine(
                renderer,
                waveRect.x,
                waveRect.y + waveRect.h / 2,
                waveRect.x + waveRect.w,
                waveRect.y + waveRect.h / 2 + 4);
            break;
        }
        }

        if (i < chrome_.channelLabels.size() && chrome_.channelLabels[i].texture)
        {
            const auto& label = chrome_.channelLabels[i];
            const float availableWidth = static_cast<float>(std::max(navRailRect.w - 16, 0));
            float scale = 1.0f;
            if (availableWidth > 0.0f && label.width > availableWidth)
            {
                scale = availableWidth / static_cast<float>(label.width);
            }

            const int labelWidth = static_cast<int>(std::round(label.width * scale));
            const int labelHeight = std::max(1, static_cast<int>(std::round(label.height * scale)));
            SDL_Rect labelRect{
                navRailRect.x + (navRailRect.w - labelWidth) / 2,
                buttonRect.y + buttonRect.h + 6,
                labelWidth,
                labelHeight};
            colony::RenderTexture(renderer, label, labelRect);
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
        colony::drawing::RenderFilledRoundedRect(renderer, avatarRect, avatarSize / 2);

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
