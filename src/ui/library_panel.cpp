#include "ui/library_panel.hpp"

#include "ui/layout.hpp"

#include "utils/color.hpp"
#include "utils/drawing.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace colony::ui
{

void LibraryPanelRenderer::Build(
    SDL_Renderer* renderer,
    TTF_Font* bodyFont,
    const ThemeColors& theme,
    const std::function<std::string(std::string_view)>& localize)
{
    chrome_.filterLabel = colony::CreateTextTexture(renderer, bodyFont, localize("library.filter_label"), theme.muted);
}

LibraryRenderResult LibraryPanelRenderer::Render(
    SDL_Renderer* renderer,
    const ThemeColors& theme,
    const SDL_Rect& libraryRect,
    const colony::AppContent& content,
    int activeChannelIndex,
    const std::vector<int>& channelSelections,
    const std::unordered_map<std::string, ProgramVisuals>& programVisuals,
    TTF_Font* channelFont,
    bool showAddButton,
    double timeSeconds,
    double deltaSeconds) const
{
    LibraryRenderResult result;
    result.addButtonRect.reset();
    const auto& activeChannel = content.channels[activeChannelIndex];

    const int libraryPadding = Scale(22);
    int libraryCursorY = libraryPadding;

    colony::TextTexture channelTitleTexture = colony::CreateTextTexture(renderer, channelFont, activeChannel.label, theme.heroTitle);
    if (channelTitleTexture.texture)
    {
        SDL_Rect channelTitleRect{libraryRect.x + libraryPadding, libraryCursorY, channelTitleTexture.width, channelTitleTexture.height};
        colony::RenderTexture(renderer, channelTitleTexture, channelTitleRect);
        libraryCursorY += channelTitleRect.h + Scale(18);
    }

    SDL_Rect filterRect{libraryRect.x + libraryPadding, libraryCursorY, libraryRect.w - 2 * libraryPadding, Scale(32)};
    SDL_SetRenderDrawColor(renderer, theme.libraryCard.r, theme.libraryCard.g, theme.libraryCard.b, theme.libraryCard.a);
    colony::drawing::RenderFilledRoundedRect(renderer, filterRect, 12);
    SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
    colony::drawing::RenderRoundedRect(renderer, filterRect, 12);
    const int filterIconSize = filterRect.h - Scale(10);
    if (filterIconSize > 0)
    {
        SDL_Rect filterIconRect{
            filterRect.x + Scale(10),
            filterRect.y + (filterRect.h - filterIconSize) / 2,
            filterIconSize,
            filterIconSize};
        SDL_Color filterIconColor = colony::color::Mix(theme.muted, theme.heroTitle, 0.25f);
        SDL_Color filterIconFill = colony::color::Mix(theme.libraryCard, filterIconColor, 0.45f);
        SDL_SetRenderDrawColor(renderer, filterIconFill.r, filterIconFill.g, filterIconFill.b, filterIconFill.a);
        colony::drawing::RenderFilledRoundedRect(renderer, filterIconRect, std::max(filterIconSize / 3, Scale(3)));
        SDL_SetRenderDrawColor(renderer, filterIconColor.r, filterIconColor.g, filterIconColor.b, filterIconColor.a);
        colony::drawing::RenderRoundedRect(renderer, filterIconRect, std::max(filterIconSize / 3, Scale(3)));
        SDL_RenderDrawLine(
            renderer,
            filterIconRect.x + filterIconRect.w - Scale(4),
            filterIconRect.y + filterIconRect.h - Scale(4),
            filterIconRect.x + filterIconRect.w + Scale(2),
            filterIconRect.y + filterIconRect.h + Scale(2));

        int labelRightBound = filterRect.x + filterRect.w - Scale(10);
        if (showAddButton)
        {
            const int buttonSize = std::max(Scale(30), filterRect.h - Scale(6));
            SDL_Rect addButtonRect{
                filterRect.x + filterRect.w - buttonSize - Scale(8),
                filterRect.y + (filterRect.h - buttonSize) / 2,
                buttonSize,
                buttonSize};
            result.addButtonRect = addButtonRect;

            SDL_Color buttonFill = colony::color::Mix(theme.libraryCardActive, theme.libraryBackground, 0.5f);
            SDL_SetRenderDrawColor(renderer, buttonFill.r, buttonFill.g, buttonFill.b, buttonFill.a);
            colony::drawing::RenderFilledRoundedRect(renderer, addButtonRect, 10);
            SDL_SetRenderDrawColor(renderer, theme.channelBadge.r, theme.channelBadge.g, theme.channelBadge.b, theme.channelBadge.a);
            colony::drawing::RenderRoundedRect(renderer, addButtonRect, 10);

            SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, SDL_ALPHA_OPAQUE);
            const int centerX = addButtonRect.x + addButtonRect.w / 2;
            const int centerY = addButtonRect.y + addButtonRect.h / 2;
            const int glyphExtent = addButtonRect.w / 3;
            SDL_RenderDrawLine(renderer, centerX - glyphExtent, centerY, centerX + glyphExtent, centerY);
            SDL_RenderDrawLine(renderer, centerX, centerY - glyphExtent, centerX, centerY + glyphExtent);

            labelRightBound = addButtonRect.x - Scale(10);
        }

        if (chrome_.filterLabel.texture && filterIconRect.x + filterIconRect.w + Scale(10) < labelRightBound)
        {
            SDL_Rect filterLabelRect{
                filterIconRect.x + filterIconRect.w + Scale(10),
                filterRect.y + (filterRect.h - chrome_.filterLabel.height) / 2,
                chrome_.filterLabel.width,
                chrome_.filterLabel.height};

            if (filterLabelRect.x + filterLabelRect.w > labelRightBound)
            {
                SDL_Rect clipRect{
                    filterLabelRect.x,
                    filterLabelRect.y,
                    std::max(0, labelRightBound - filterLabelRect.x),
                    chrome_.filterLabel.height};
                if (clipRect.w > 0)
                {
                    SDL_RenderSetClipRect(renderer, &clipRect);
                    colony::RenderTexture(renderer, chrome_.filterLabel, filterLabelRect);
                    SDL_RenderSetClipRect(renderer, nullptr);
                }
            }
            else
            {
                colony::RenderTexture(renderer, chrome_.filterLabel, filterLabelRect);
            }
        }
    }
    else if (chrome_.filterLabel.texture)
    {
        SDL_Rect filterLabelRect{
            filterRect.x + Scale(10),
            filterRect.y + (filterRect.h - chrome_.filterLabel.height) / 2,
            chrome_.filterLabel.width,
            chrome_.filterLabel.height};
        int labelRightBound = filterRect.x + filterRect.w - Scale(10);
        if (showAddButton)
        {
            const int buttonSize = std::max(Scale(30), filterRect.h - Scale(6));
            SDL_Rect addButtonRect{
                filterRect.x + filterRect.w - buttonSize - Scale(8),
                filterRect.y + (filterRect.h - buttonSize) / 2,
                buttonSize,
                buttonSize};
            result.addButtonRect = addButtonRect;

            SDL_Color buttonFill = colony::color::Mix(theme.libraryCardActive, theme.libraryBackground, 0.5f);
            SDL_SetRenderDrawColor(renderer, buttonFill.r, buttonFill.g, buttonFill.b, buttonFill.a);
            colony::drawing::RenderFilledRoundedRect(renderer, addButtonRect, 10);
            SDL_SetRenderDrawColor(renderer, theme.channelBadge.r, theme.channelBadge.g, theme.channelBadge.b, theme.channelBadge.a);
            colony::drawing::RenderRoundedRect(renderer, addButtonRect, 10);

            SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, SDL_ALPHA_OPAQUE);
            const int centerX = addButtonRect.x + addButtonRect.w / 2;
            const int centerY = addButtonRect.y + addButtonRect.h / 2;
            const int glyphExtent = addButtonRect.w / 3;
            SDL_RenderDrawLine(renderer, centerX - glyphExtent, centerY, centerX + glyphExtent, centerY);
            SDL_RenderDrawLine(renderer, centerX, centerY - glyphExtent, centerX, centerY + glyphExtent);

            labelRightBound = addButtonRect.x - Scale(10);
        }

        if (filterLabelRect.x < labelRightBound)
        {
            if (filterLabelRect.x + filterLabelRect.w > labelRightBound)
            {
                SDL_Rect clipRect{
                    filterLabelRect.x,
                    filterLabelRect.y,
                    std::max(0, labelRightBound - filterLabelRect.x),
                    chrome_.filterLabel.height};
                if (clipRect.w > 0)
                {
                    SDL_RenderSetClipRect(renderer, &clipRect);
                    colony::RenderTexture(renderer, chrome_.filterLabel, filterLabelRect);
                    SDL_RenderSetClipRect(renderer, nullptr);
                }
            }
            else
            {
                colony::RenderTexture(renderer, chrome_.filterLabel, filterLabelRect);
            }
        }
    }
    libraryCursorY += filterRect.h + Scale(18);

    result.tileRects.reserve(activeChannel.programs.size());
    (void)deltaSeconds;

    const int tileHeight = Scale(82);
    const int tileSpacing = Scale(14);

    for (std::size_t index = 0; index < activeChannel.programs.size(); ++index)
    {
        const std::string& programId = activeChannel.programs[index];
        const auto programIt = programVisuals.find(programId);
        if (programIt == programVisuals.end())
        {
            continue;
        }
        const bool isActiveProgram = static_cast<int>(index) == channelSelections[activeChannelIndex];

        SDL_Rect tileRect{
            libraryRect.x + libraryPadding,
            libraryCursorY,
            libraryRect.w - 2 * libraryPadding,
            tileHeight};

        SDL_Color baseColor = isActiveProgram ? theme.libraryCardActive : theme.libraryCard;
        if (isActiveProgram)
        {
            const float glow = static_cast<float>(0.35 + 0.35 * std::sin(timeSeconds * 1.5 + index));
            baseColor = colony::color::Mix(baseColor, programIt->second.accent, 0.25f + glow * 0.25f);
        }
        else
        {
            const float shimmer = static_cast<float>(0.1 + 0.1 * std::sin(timeSeconds + index));
            baseColor = colony::color::Mix(baseColor, theme.libraryCardActive, shimmer);
        }

        SDL_SetRenderDrawColor(renderer, baseColor.r, baseColor.g, baseColor.b, baseColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, tileRect, 14);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        colony::drawing::RenderRoundedRect(renderer, tileRect, 14);

        const int accentWidth = Scale(4);
        SDL_Rect accentStrip{tileRect.x, tileRect.y, accentWidth, tileRect.h};
        SDL_SetRenderDrawColor(renderer, programIt->second.accent.r, programIt->second.accent.g, programIt->second.accent.b, SDL_ALPHA_OPAQUE);
        colony::drawing::RenderFilledRoundedRect(renderer, accentStrip, 3);

        const int iconSize = Scale(46);
        const int iconOffset = static_cast<int>(std::round(std::sin(timeSeconds * 2.0 + index) * Scale(3)));
        SDL_Rect iconRect{
            tileRect.x + Scale(14),
            tileRect.y + (tileRect.h - iconSize) / 2 + iconOffset,
            iconSize,
            iconSize};
        SDL_Color iconFill = colony::color::Mix(programIt->second.accent, baseColor, 0.25f);
        SDL_SetRenderDrawColor(renderer, iconFill.r, iconFill.g, iconFill.b, iconFill.a);
        colony::drawing::RenderFilledRoundedRect(renderer, iconRect, 14);
        SDL_SetRenderDrawColor(renderer, programIt->second.accent.r, programIt->second.accent.g, programIt->second.accent.b, SDL_ALPHA_OPAQUE);
        colony::drawing::RenderRoundedRect(renderer, iconRect, 14);
        const int glyphSize = Scale(16);
        SDL_Rect glyphRect{
            iconRect.x + iconRect.w / 2 - glyphSize / 2,
            iconRect.y + iconRect.h / 2 - glyphSize / 2,
            glyphSize,
            glyphSize};
        SDL_SetRenderDrawColor(renderer, theme.libraryCard.r, theme.libraryCard.g, theme.libraryCard.b, theme.libraryCard.a);
        colony::drawing::RenderRoundedRect(renderer, glyphRect, 6);
        SDL_RenderDrawLine(
            renderer,
            glyphRect.x,
            glyphRect.y + glyphRect.h,
            glyphRect.x + glyphRect.w,
            glyphRect.y);

        int textX = iconRect.x + iconRect.w + Scale(12);
        SDL_Rect textClip{
            textX,
            tileRect.y + Scale(10),
            tileRect.x + tileRect.w - textX - Scale(14),
            tileRect.h - Scale(20)};
        const bool hasTextClip = textClip.w > 0 && textClip.h > 0;
        if (hasTextClip)
        {
            SDL_RenderSetClipRect(renderer, &textClip);
        }
        int textY = hasTextClip ? textClip.y : tileRect.y + Scale(12);
        if (programIt->second.tileTitle.texture)
        {
            SDL_Rect titleRect{textX, textY, programIt->second.tileTitle.width, programIt->second.tileTitle.height};
            colony::RenderTexture(renderer, programIt->second.tileTitle, titleRect);
            textY += titleRect.h + Scale(4);
        }
        if (programIt->second.tileSubtitle.texture)
        {
            SDL_Rect subtitleRect{textX, textY, programIt->second.tileSubtitle.width, programIt->second.tileSubtitle.height};
            colony::RenderTexture(renderer, programIt->second.tileSubtitle, subtitleRect);
            textY += subtitleRect.h + Scale(4);
        }
        if (programIt->second.tileMeta.texture)
        {
            SDL_Rect metaRect{textX, textY, programIt->second.tileMeta.width, programIt->second.tileMeta.height};
            colony::RenderTexture(renderer, programIt->second.tileMeta, metaRect);
        }

        if (hasTextClip)
        {
            SDL_RenderSetClipRect(renderer, nullptr);
        }
        result.tileRects.emplace_back(tileRect);
        libraryCursorY += tileHeight + tileSpacing;
    }

    return result;
}

} // namespace colony::ui
