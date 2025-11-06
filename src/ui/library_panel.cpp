#include "ui/library_panel.hpp"

#include "utils/color.hpp"
#include "utils/drawing.hpp"

#include <algorithm>
#include <string>

namespace colony::ui
{

void LibraryPanelRenderer::Build(SDL_Renderer* renderer, TTF_Font* bodyFont, const ThemeColors& theme)
{
    chrome_.filterLabel = colony::CreateTextTexture(renderer, bodyFont, "Installed programs", theme.muted);
}

LibraryRenderResult LibraryPanelRenderer::Render(
    SDL_Renderer* renderer,
    const ThemeColors& theme,
    const SDL_Rect& libraryRect,
    const colony::AppContent& content,
    int activeChannelIndex,
    const std::vector<int>& channelSelections,
    const std::unordered_map<std::string, ProgramVisuals>& programVisuals,
    TTF_Font* channelFont) const
{
    LibraryRenderResult result;
    const auto& activeChannel = content.channels[activeChannelIndex];

    const int libraryPadding = 28;
    int libraryCursorY = libraryPadding;

    colony::TextTexture channelTitleTexture = colony::CreateTextTexture(renderer, channelFont, activeChannel.label, theme.heroTitle);
    if (channelTitleTexture.texture)
    {
        SDL_Rect channelTitleRect{libraryRect.x + libraryPadding, libraryCursorY, channelTitleTexture.width, channelTitleTexture.height};
        colony::RenderTexture(renderer, channelTitleTexture, channelTitleRect);
        libraryCursorY += channelTitleRect.h + 24;
    }

    SDL_Rect filterRect{libraryRect.x + libraryPadding, libraryCursorY, libraryRect.w - 2 * libraryPadding, 36};
    SDL_SetRenderDrawColor(renderer, theme.libraryCard.r, theme.libraryCard.g, theme.libraryCard.b, theme.libraryCard.a);
    colony::drawing::RenderFilledRoundedRect(renderer, filterRect, 12);
    SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
    colony::drawing::RenderRoundedRect(renderer, filterRect, 12);
    const int filterIconSize = filterRect.h - 12;
    if (filterIconSize > 0)
    {
        SDL_Rect filterIconRect{
            filterRect.x + 10,
            filterRect.y + (filterRect.h - filterIconSize) / 2,
            filterIconSize,
            filterIconSize};
        SDL_Color filterIconColor = colony::color::Mix(theme.muted, theme.heroTitle, 0.25f);
        SDL_Color filterIconFill = colony::color::Mix(theme.libraryCard, filterIconColor, 0.45f);
        SDL_SetRenderDrawColor(renderer, filterIconFill.r, filterIconFill.g, filterIconFill.b, filterIconFill.a);
        colony::drawing::RenderFilledRoundedRect(renderer, filterIconRect, std::max(filterIconSize / 3, 4));
        SDL_SetRenderDrawColor(renderer, filterIconColor.r, filterIconColor.g, filterIconColor.b, filterIconColor.a);
        colony::drawing::RenderRoundedRect(renderer, filterIconRect, std::max(filterIconSize / 3, 4));
        SDL_RenderDrawLine(
            renderer,
            filterIconRect.x + filterIconRect.w - 4,
            filterIconRect.y + filterIconRect.h - 4,
            filterIconRect.x + filterIconRect.w + 2,
            filterIconRect.y + filterIconRect.h + 2);

        if (chrome_.filterLabel.texture)
        {
            SDL_Rect filterLabelRect{
                filterIconRect.x + filterIconRect.w + 10,
                filterRect.y + (filterRect.h - chrome_.filterLabel.height) / 2,
                chrome_.filterLabel.width,
                chrome_.filterLabel.height};
            colony::RenderTexture(renderer, chrome_.filterLabel, filterLabelRect);
        }
    }
    else if (chrome_.filterLabel.texture)
    {
        SDL_Rect filterLabelRect{
            filterRect.x + 12,
            filterRect.y + (filterRect.h - chrome_.filterLabel.height) / 2,
            chrome_.filterLabel.width,
            chrome_.filterLabel.height};
        colony::RenderTexture(renderer, chrome_.filterLabel, filterLabelRect);
    }
    libraryCursorY += filterRect.h + 24;

    result.tileRects.reserve(activeChannel.programs.size());
    const int tileHeight = 100;
    const int tileSpacing = 18;

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
            baseColor = colony::color::Mix(baseColor, programIt->second.accent, 0.2f);
        }

        SDL_SetRenderDrawColor(renderer, baseColor.r, baseColor.g, baseColor.b, baseColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, tileRect, 14);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        colony::drawing::RenderRoundedRect(renderer, tileRect, 14);

        SDL_Rect accentStrip{tileRect.x, tileRect.y, 6, tileRect.h};
        SDL_SetRenderDrawColor(renderer, programIt->second.accent.r, programIt->second.accent.g, programIt->second.accent.b, SDL_ALPHA_OPAQUE);
        colony::drawing::RenderFilledRoundedRect(renderer, accentStrip, 3);

        SDL_Rect iconRect{
            tileRect.x + 18,
            tileRect.y + (tileRect.h - 56) / 2,
            56,
            56};
        SDL_Color iconFill = colony::color::Mix(programIt->second.accent, baseColor, 0.25f);
        SDL_SetRenderDrawColor(renderer, iconFill.r, iconFill.g, iconFill.b, iconFill.a);
        colony::drawing::RenderFilledRoundedRect(renderer, iconRect, 14);
        SDL_SetRenderDrawColor(renderer, programIt->second.accent.r, programIt->second.accent.g, programIt->second.accent.b, SDL_ALPHA_OPAQUE);
        colony::drawing::RenderRoundedRect(renderer, iconRect, 14);
        SDL_Rect glyphRect{
            iconRect.x + iconRect.w / 2 - 10,
            iconRect.y + iconRect.h / 2 - 10,
            20,
            20};
        SDL_SetRenderDrawColor(renderer, theme.libraryCard.r, theme.libraryCard.g, theme.libraryCard.b, theme.libraryCard.a);
        colony::drawing::RenderRoundedRect(renderer, glyphRect, 6);
        SDL_RenderDrawLine(
            renderer,
            glyphRect.x,
            glyphRect.y + glyphRect.h,
            glyphRect.x + glyphRect.w,
            glyphRect.y);

        int textX = iconRect.x + iconRect.w + 16;
        SDL_Rect textClip{
            textX,
            tileRect.y + 12,
            tileRect.x + tileRect.w - textX - 18,
            tileRect.h - 24};
        const bool hasTextClip = textClip.w > 0 && textClip.h > 0;
        if (hasTextClip)
        {
            SDL_RenderSetClipRect(renderer, &textClip);
        }
        int textY = hasTextClip ? textClip.y : tileRect.y + 14;
        if (programIt->second.tileTitle.texture)
        {
            SDL_Rect titleRect{textX, textY, programIt->second.tileTitle.width, programIt->second.tileTitle.height};
            colony::RenderTexture(renderer, programIt->second.tileTitle, titleRect);
            textY += titleRect.h + 6;
        }
        if (programIt->second.tileSubtitle.texture)
        {
            SDL_Rect subtitleRect{textX, textY, programIt->second.tileSubtitle.width, programIt->second.tileSubtitle.height};
            colony::RenderTexture(renderer, programIt->second.tileSubtitle, subtitleRect);
            textY += subtitleRect.h + 6;
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
