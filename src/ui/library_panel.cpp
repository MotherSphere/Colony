#include "ui/library_panel.hpp"

#include "utils/color.hpp"

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
    SDL_RenderFillRect(renderer, &filterRect);
    SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
    SDL_RenderDrawRect(renderer, &filterRect);
    if (chrome_.filterLabel.texture)
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
        SDL_RenderFillRect(renderer, &tileRect);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        SDL_RenderDrawRect(renderer, &tileRect);

        SDL_Rect accentStrip{tileRect.x, tileRect.y, 6, tileRect.h};
        SDL_SetRenderDrawColor(renderer, programIt->second.accent.r, programIt->second.accent.g, programIt->second.accent.b, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer, &accentStrip);

        int textX = tileRect.x + 18;
        int textY = tileRect.y + 14;
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

        result.tileRects.emplace_back(tileRect);
        libraryCursorY += tileHeight + tileSpacing;
    }

    return result;
}

} // namespace colony::ui
