#include "ui/panels/navigation.hpp"

#include "ui/layout.hpp"

#include "utils/color.hpp"
#include "utils/drawing.hpp"

#include <algorithm>
#include <cmath>

namespace colony::ui::panels
{

void NavigationRailPanel::Build(
    SDL_Renderer* renderer,
    TTF_Font* brandFont,
    TTF_Font* navFont,
    TTF_Font* metaFont,
    const colony::AppContent& content,
    const ThemeColors& theme,
    const Typography& typography)
{
    chrome_.brand = colony::CreateTextTexture(renderer, brandFont, content.brandName, theme.heroTitle);
    chrome_.items.clear();
    chrome_.items.reserve(content.channels.size());

    (void)metaFont;
    (void)typography;

    if (!navFont)
    {
        return;
    }

    for (const auto& channel : content.channels)
    {
        frontend::components::SidebarItem item;
        item.Build(renderer, navFont, channel.id, channel.label, theme);
        chrome_.items.emplace_back(std::move(item));
    }
}

NavigationRenderResult NavigationRailPanel::Render(
    SDL_Renderer* renderer,
    const ThemeColors& theme,
    const Typography& typography,
    const InteractionColors& interactions,
    const SDL_Rect& navRailRect,
    int statusBarHeight,
    const colony::AppContent& content,
    const std::vector<int>& channelSelections,
    int activeChannelIndex,
    const std::unordered_map<std::string, ProgramVisuals>& programVisuals,
    double timeSeconds) const
{
    NavigationRenderResult result;
    result.channelButtonRects.resize(content.channels.size());

    const int navPadding = Scale(24);
    if (chrome_.brand.texture)
    {
        SDL_Rect brandRect{
            navRailRect.x + Scale(22),
            navPadding,
            chrome_.brand.width,
            chrome_.brand.height};
        colony::RenderTexture(renderer, chrome_.brand, brandRect);
        result.hubButtonRect = brandRect;
    }
    else
    {
        result.hubButtonRect.reset();
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

    const int brandSpacing = chrome_.brand.height > 0 ? chrome_.brand.height + Scale(28) : Scale(32);
    int channelStartY = navPadding + brandSpacing;
    const int itemHeight = Scale(64);
    const int itemSpacing = Scale(12);
    const int itemWidth = navRailRect.w - Scale(24);

    SDL_Point mousePosition{0, 0};
    SDL_GetMouseState(&mousePosition.x, &mousePosition.y);

    for (std::size_t index = 0; index < chrome_.items.size() && index < result.channelButtonRects.size(); ++index)
    {
        SDL_Rect itemRect{
            navRailRect.x + Scale(12),
            channelStartY,
            itemWidth,
            itemHeight};
        const bool isActive = static_cast<int>(index) == activeChannelIndex;
        const bool isHovered = SDL_PointInRect(&mousePosition, &itemRect) != 0;
        SDL_Color accent = channelAccentColor(static_cast<int>(index));
        chrome_.items[index].Render(renderer, theme, typography, interactions, itemRect, accent, isActive, isHovered, timeSeconds);
        result.channelButtonRects[index] = itemRect;
        channelStartY += itemHeight + itemSpacing;
    }

    return result;
}

} // namespace colony::ui::panels
