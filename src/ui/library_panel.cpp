#include "ui/library_panel.hpp"

#include "frontend/components/empty_state_card.hpp"
#include "ui/layout.hpp"

#include "utils/color.hpp"
#include "utils/drawing.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace colony::ui
{
namespace
{
constexpr int kRightRoundedCorners = colony::drawing::CornerTopRight | colony::drawing::CornerBottomRight;
}


void LibraryPanelRenderer::Build(
    SDL_Renderer* renderer,
    TTF_Font* bodyFont,
    const ThemeColors& theme,
    const std::function<std::string(std::string_view)>& localize)
{
    std::string placeholder = localize("library.filter_placeholder");
    if (placeholder.empty())
    {
        placeholder = localize("library.filter_label");
    }
    if (placeholder.empty())
    {
        placeholder = "Search library";
    }
    chrome_.filterPlaceholder = colony::CreateTextTexture(renderer, bodyFont, placeholder, theme.muted);
}

LibraryRenderResult LibraryPanelRenderer::Render(
    SDL_Renderer* renderer,
    const ThemeColors& theme,
    const SDL_Rect& libraryRect,
    const colony::AppContent& content,
    int activeChannelIndex,
    const std::unordered_map<std::string, ProgramVisuals>& programVisuals,
    TTF_Font* channelFont,
    TTF_Font* bodyFont,
    bool showAddButton,
    double timeSeconds,
    double deltaSeconds,
    std::string_view filterText,
    bool filterFocused,
    const std::vector<colony::frontend::models::LibraryProgramEntry>& programs,
    const std::vector<colony::frontend::models::LibrarySortChip>& sortChips) const
{
    LibraryRenderResult result;
    result.addButtonRect.reset();
    result.filterInputRect.reset();
    result.sortChipHitboxes.clear();
    result.programIds.clear();
    result.programIds.reserve(programs.size());

    const int libraryPadding = Scale(28);
    int libraryCursorY = libraryPadding;

    if (activeChannelIndex >= 0 && activeChannelIndex < static_cast<int>(content.channels.size()))
    {
        const auto& activeChannel = content.channels[activeChannelIndex];
        colony::TextTexture channelTitleTexture = colony::CreateTextTexture(renderer, channelFont, activeChannel.label, theme.heroTitle);
        if (channelTitleTexture.texture)
        {
            SDL_Rect channelTitleRect{libraryRect.x + libraryPadding, libraryCursorY, channelTitleTexture.width, channelTitleTexture.height};
            colony::RenderTexture(renderer, channelTitleTexture, channelTitleRect);
            libraryCursorY += channelTitleRect.h + Scale(22);
        }
    }

    const int filterHeight = Scale(44);
    SDL_Rect filterRect{libraryRect.x + libraryPadding, libraryCursorY, libraryRect.w - 2 * libraryPadding, filterHeight};

    SDL_Color filterBase = colony::color::Mix(theme.libraryCard, theme.libraryBackground, 0.35f);
    SDL_Color filterFill = filterFocused ? colony::color::Mix(theme.libraryCardActive, theme.libraryBackground, 0.4f) : filterBase;
    SDL_Color filterBorder = filterFocused ? colony::color::Mix(theme.channelBadge, theme.heroTitle, 0.25f)
                                           : colony::color::Mix(theme.border, theme.libraryCardActive, 0.25f);

    SDL_Rect filterShadow = filterRect;
    filterShadow.y += Scale(3);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, filterBorder.r, filterBorder.g, filterBorder.b, 40);
    colony::drawing::RenderFilledRoundedRect(renderer, filterShadow, 14);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    SDL_SetRenderDrawColor(renderer, filterFill.r, filterFill.g, filterFill.b, filterFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, filterRect, 14);
    SDL_SetRenderDrawColor(renderer, filterBorder.r, filterBorder.g, filterBorder.b, filterBorder.a);
    colony::drawing::RenderRoundedRect(renderer, filterRect, 14);

    const int filterIconSize = filterRect.h - Scale(12);
    int labelRightBound = filterRect.x + filterRect.w - Scale(14);
    int textStartX = filterRect.x + Scale(16);
    if (filterIconSize > 0)
    {
        SDL_Rect filterIconRect{
            filterRect.x + Scale(10),
            filterRect.y + (filterRect.h - filterIconSize) / 2,
            filterIconSize,
            filterIconSize};
        SDL_Color filterIconColor = colony::color::Mix(theme.channelBadge, theme.heroTitle, 0.2f);
        SDL_Color filterIconFill = colony::color::Mix(filterFill, filterIconColor, 0.5f);
        SDL_SetRenderDrawColor(renderer, filterIconFill.r, filterIconFill.g, filterIconFill.b, filterIconFill.a);
        const int iconCorner = std::max(filterIconSize / 3, Scale(4));
        colony::drawing::RenderFilledRoundedRect(renderer, filterIconRect, iconCorner);
        SDL_SetRenderDrawColor(renderer, filterIconColor.r, filterIconColor.g, filterIconColor.b, filterIconColor.a);
        colony::drawing::RenderRoundedRect(renderer, filterIconRect, iconCorner);
        SDL_RenderDrawLine(
            renderer,
            filterIconRect.x + filterIconRect.w - Scale(6),
            filterIconRect.y + filterIconRect.h - Scale(6),
            filterIconRect.x + filterIconRect.w + Scale(4),
            filterIconRect.y + filterIconRect.h + Scale(4));
        textStartX = filterIconRect.x + filterIconRect.w + Scale(14);
        if (showAddButton)
        {
            const int buttonSize = std::max(Scale(32), filterRect.h - Scale(8));
            SDL_Rect addButtonRect{
                filterRect.x + filterRect.w - buttonSize - Scale(10),
                filterRect.y + (filterRect.h - buttonSize) / 2,
                buttonSize,
                buttonSize};
            result.addButtonRect = addButtonRect;

            SDL_Color buttonFill = colony::color::Mix(theme.channelBadge, theme.libraryCardActive, 0.35f);
            SDL_Color buttonOutline = colony::color::Mix(theme.channelBadge, theme.heroTitle, 0.2f);
            SDL_SetRenderDrawColor(renderer, buttonFill.r, buttonFill.g, buttonFill.b, buttonFill.a);
            colony::drawing::RenderFilledRoundedRect(renderer, addButtonRect, 12);
            SDL_SetRenderDrawColor(renderer, buttonOutline.r, buttonOutline.g, buttonOutline.b, buttonOutline.a);
            colony::drawing::RenderRoundedRect(renderer, addButtonRect, 12);

            SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, SDL_ALPHA_OPAQUE);
            const int centerX = addButtonRect.x + addButtonRect.w / 2;
            const int centerY = addButtonRect.y + addButtonRect.h / 2;
            const int glyphExtent = addButtonRect.w / 3;
            SDL_RenderDrawLine(renderer, centerX - glyphExtent, centerY, centerX + glyphExtent, centerY);
            SDL_RenderDrawLine(renderer, centerX, centerY - glyphExtent, centerX, centerY + glyphExtent);

            labelRightBound = addButtonRect.x - Scale(10);
        }
    }
    else
    {
        if (showAddButton)
        {
            const int buttonSize = std::max(Scale(32), filterRect.h - Scale(8));
            SDL_Rect addButtonRect{
                filterRect.x + filterRect.w - buttonSize - Scale(10),
                filterRect.y + (filterRect.h - buttonSize) / 2,
                buttonSize,
                buttonSize};
            result.addButtonRect = addButtonRect;

            SDL_Color buttonFill = colony::color::Mix(theme.channelBadge, theme.libraryCardActive, 0.35f);
            SDL_Color buttonOutline = colony::color::Mix(theme.channelBadge, theme.heroTitle, 0.2f);
            SDL_SetRenderDrawColor(renderer, buttonFill.r, buttonFill.g, buttonFill.b, buttonFill.a);
            colony::drawing::RenderFilledRoundedRect(renderer, addButtonRect, 12);
            SDL_SetRenderDrawColor(renderer, buttonOutline.r, buttonOutline.g, buttonOutline.b, buttonOutline.a);
            colony::drawing::RenderRoundedRect(renderer, addButtonRect, 12);

            SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, SDL_ALPHA_OPAQUE);
            const int centerX = addButtonRect.x + addButtonRect.w / 2;
            const int centerY = addButtonRect.y + addButtonRect.h / 2;
            const int glyphExtent = addButtonRect.w / 3;
            SDL_RenderDrawLine(renderer, centerX - glyphExtent, centerY, centerX + glyphExtent, centerY);
            SDL_RenderDrawLine(renderer, centerX, centerY - glyphExtent, centerX, centerY + glyphExtent);

            labelRightBound = addButtonRect.x - Scale(10);
        }
        textStartX = filterRect.x + Scale(16);
    }

    if (labelRightBound > filterRect.x)
    {
        const int textClipWidth = std::max(0, labelRightBound - textStartX);
        SDL_Rect textClip{textStartX, filterRect.y + Scale(6), textClipWidth, filterRect.h - Scale(12)};

        colony::TextTexture inputTexture = colony::CreateTextTexture(renderer, bodyFont, filterText, theme.heroTitle);
        const bool hasInputText = inputTexture.texture.get() != nullptr && inputTexture.width > 0;

        if (textClip.w > 0)
        {
            SDL_RenderSetClipRect(renderer, &textClip);
        }

        if (hasInputText)
        {
            SDL_Rect textRect{
                textStartX,
                filterRect.y + (filterRect.h - inputTexture.height) / 2,
                inputTexture.width,
                inputTexture.height};
            colony::RenderTexture(renderer, inputTexture, textRect);
        }
        else if (chrome_.filterPlaceholder.texture)
        {
            SDL_Rect placeholderRect{
                textStartX,
                filterRect.y + (filterRect.h - chrome_.filterPlaceholder.height) / 2,
                chrome_.filterPlaceholder.width,
                chrome_.filterPlaceholder.height};
            colony::RenderTexture(renderer, chrome_.filterPlaceholder, placeholderRect);
        }

        if (textClip.w > 0)
        {
            SDL_RenderSetClipRect(renderer, nullptr);
        }

        if (filterFocused)
        {
            const bool caretVisible = std::fmod(timeSeconds, 1.0) < 0.5;
            if (caretVisible)
            {
                if (textClipWidth > 0)
                {
                    const int caretOffset = hasInputText ? std::min(inputTexture.width, textClipWidth - Scale(2)) : 0;
                    const int caretX = textStartX + std::max(0, caretOffset) + Scale(2);
                    SDL_Rect caretClip{textStartX, filterRect.y + Scale(6), textClipWidth, filterRect.h - Scale(12)};
                    SDL_RenderSetClipRect(renderer, &caretClip);
                    SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, theme.heroTitle.a);
                    SDL_RenderDrawLine(
                        renderer,
                        caretX,
                        filterRect.y + Scale(6),
                        caretX,
                        filterRect.y + filterRect.h - Scale(6));
                    SDL_RenderSetClipRect(renderer, nullptr);
                }
            }
        }

        SDL_Rect focusRect{filterRect.x, filterRect.y, std::max(0, labelRightBound - filterRect.x), filterRect.h};
        if (focusRect.w > 0)
        {
            result.filterInputRect = focusRect;
        }
    }
    libraryCursorY += filterRect.h + Scale(22);

    const int chipPaddingX = Scale(18);
    const int chipHeight = Scale(34);
    const int chipSpacing = Scale(12);
    const int chipAreaStart = libraryRect.x + libraryPadding;
    const int chipAreaEnd = libraryRect.x + libraryRect.w - libraryPadding;
    const int chipAreaWidth = std::max(0, chipAreaEnd - chipAreaStart);
    int chipCursorX = chipAreaStart;
    int chipCursorY = libraryCursorY;
    bool renderedChips = false;

    for (const auto& chip : sortChips)
    {
        colony::TextTexture chipTexture = colony::CreateTextTexture(renderer, bodyFont, chip.label, chip.active ? theme.heroTitle : theme.muted);
        const int naturalWidth = chipTexture.texture ? chipTexture.width + chipPaddingX * 2 : chipHeight;
        const int minChipWidth = chipAreaWidth > 0 ? std::min(Scale(72), chipAreaWidth) : 0;
        const int desiredWidth = std::max(naturalWidth, minChipWidth);

        if (chipCursorX + desiredWidth > chipAreaEnd)
        {
            chipCursorX = chipAreaStart;
            chipCursorY += chipHeight + chipSpacing;
        }

        const int rowAvailableWidth = std::max(0, chipAreaEnd - chipCursorX);
        if (rowAvailableWidth <= 0)
        {
            break;
        }

        const int chipWidth = std::min(desiredWidth, rowAvailableWidth);

        SDL_Rect chipRect{chipCursorX, chipCursorY, chipWidth, chipHeight};
        SDL_Color chipFill = chip.active ? colony::color::Mix(theme.libraryCardActive, theme.channelBadge, 0.2f)
                                         : colony::color::Mix(theme.libraryCard, theme.libraryBackground, 0.3f);
        SDL_Color chipBorder = chip.active ? colony::color::Mix(theme.channelBadge, theme.heroTitle, 0.3f)
                                           : colony::color::Mix(theme.border, theme.libraryCardActive, 0.25f);

        SDL_Rect chipShadow = chipRect;
        chipShadow.y += Scale(2);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, chipBorder.r, chipBorder.g, chipBorder.b, 40);
        colony::drawing::RenderFilledRoundedRect(renderer, chipShadow, 14);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_SetRenderDrawColor(renderer, chipFill.r, chipFill.g, chipFill.b, chipFill.a);
        colony::drawing::RenderFilledRoundedRect(renderer, chipRect, 14);
        SDL_SetRenderDrawColor(renderer, chipBorder.r, chipBorder.g, chipBorder.b, chipBorder.a);
        colony::drawing::RenderRoundedRect(renderer, chipRect, 14);

        if (chipTexture.texture)
        {
            const int paddedClipWidth = chipRect.w - chipPaddingX * 2;
            if (paddedClipWidth > 0)
            {
                SDL_Rect clipRect{
                    chipRect.x + chipPaddingX,
                    chipRect.y,
                    paddedClipWidth,
                    chipRect.h};
                SDL_RenderSetClipRect(renderer, &clipRect);

                SDL_Rect textRect{
                    chipRect.x + (chipRect.w - chipTexture.width) / 2,
                    chipRect.y + (chipRect.h - chipTexture.height) / 2,
                    chipTexture.width,
                    chipTexture.height};

                if (chipTexture.width > paddedClipWidth)
                {
                    textRect.x = clipRect.x;
                }

                colony::RenderTexture(renderer, chipTexture, textRect);
                SDL_RenderSetClipRect(renderer, nullptr);
            }
        }

        result.sortChipHitboxes.push_back(LibraryRenderResult::SortChipHitbox{chipRect, chip.option});
        chipCursorX += chipWidth + chipSpacing;
        renderedChips = true;
    }

    if (renderedChips)
    {
        libraryCursorY = chipCursorY + chipHeight + Scale(18);
    }

    result.tileRects.reserve(programs.size());
    (void)deltaSeconds;

    const int tileHeight = Scale(92);
    const int tileSpacing = Scale(18);

    bool renderedAny = false;
    for (std::size_t index = 0; index < programs.size(); ++index)
    {
        const std::string& programId = programs[index].programId;
        const auto programIt = programVisuals.find(programId);
        if (programIt == programVisuals.end())
        {
            continue;
        }
        const bool isActiveProgram = programs[index].selected;

        SDL_Rect tileRect{
            libraryRect.x + libraryPadding,
            libraryCursorY,
            libraryRect.w - 2 * libraryPadding,
            tileHeight};

        SDL_Rect tileShadow = tileRect;
        tileShadow.y += Scale(3);
        tileShadow.h += Scale(2);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, 50);
        colony::drawing::RenderFilledRoundedRect(renderer, tileShadow, 18, kRightRoundedCorners);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_Color baseColor = colony::color::Mix(theme.libraryCard, theme.libraryBackground, 0.25f);
        if (isActiveProgram)
        {
            const float glow = static_cast<float>(0.35 + 0.35 * std::sin(timeSeconds * 1.5 + index));
            baseColor = colony::color::Mix(theme.libraryCardActive, programIt->second.accent, 0.3f + glow * 0.25f);
        }
        else
        {
            const float shimmer = static_cast<float>(0.08 + 0.1 * std::sin(timeSeconds + index));
            baseColor = colony::color::Mix(baseColor, theme.libraryCardActive, shimmer);
        }

        SDL_Color tileOutline = isActiveProgram ? colony::color::Mix(programIt->second.accent, theme.heroTitle, 0.3f)
                                                : colony::color::Mix(theme.border, theme.libraryCardActive, 0.3f);

        SDL_SetRenderDrawColor(renderer, baseColor.r, baseColor.g, baseColor.b, baseColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, tileRect, 18, kRightRoundedCorners);
        SDL_SetRenderDrawColor(renderer, tileOutline.r, tileOutline.g, tileOutline.b, tileOutline.a);
        colony::drawing::RenderRoundedRect(renderer, tileRect, 18, kRightRoundedCorners);

        const int accentWidth = Scale(6);
        SDL_Rect accentStrip{tileRect.x, tileRect.y, accentWidth, tileRect.h};
        SDL_Color accentColor = colony::color::Mix(programIt->second.accent, theme.heroTitle, 0.15f);
        SDL_SetRenderDrawColor(renderer, accentColor.r, accentColor.g, accentColor.b, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer, &accentStrip);

        const int iconSize = Scale(50);
        const int iconOffset = static_cast<int>(std::round(std::sin(timeSeconds * 2.0 + index) * Scale(3)));
        SDL_Rect iconRect{
            tileRect.x + Scale(16),
            tileRect.y + (tileRect.h - iconSize) / 2 + iconOffset,
            iconSize,
            iconSize};
        SDL_Color iconFill = colony::color::Mix(programIt->second.accent, baseColor, 0.3f);
        SDL_SetRenderDrawColor(renderer, iconFill.r, iconFill.g, iconFill.b, iconFill.a);
        colony::drawing::RenderFilledRoundedRect(renderer, iconRect, 16);
        SDL_SetRenderDrawColor(renderer, accentColor.r, accentColor.g, accentColor.b, SDL_ALPHA_OPAQUE);
        colony::drawing::RenderRoundedRect(renderer, iconRect, 16);
        const int glyphSize = Scale(18);
        SDL_Rect glyphRect{
            iconRect.x + iconRect.w / 2 - glyphSize / 2,
            iconRect.y + iconRect.h / 2 - glyphSize / 2,
            glyphSize,
            glyphSize};
        SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, theme.heroTitle.a);
        colony::drawing::RenderRoundedRect(renderer, glyphRect, 8);
        SDL_RenderDrawLine(renderer, glyphRect.x, glyphRect.y + glyphRect.h, glyphRect.x + glyphRect.w, glyphRect.y);

        int textX = iconRect.x + iconRect.w + Scale(16);
        SDL_Rect textClip{
            textX,
            tileRect.y + Scale(12),
            tileRect.x + tileRect.w - textX - Scale(18),
            tileRect.h - Scale(24)};
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
            textY += titleRect.h + Scale(6);
        }
        if (programIt->second.tileSubtitle.texture)
        {
            SDL_Rect subtitleRect{textX, textY, programIt->second.tileSubtitle.width, programIt->second.tileSubtitle.height};
            colony::RenderTexture(renderer, programIt->second.tileSubtitle, subtitleRect);
            textY += subtitleRect.h + Scale(6);
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
        result.programIds.emplace_back(programId);
        libraryCursorY += tileHeight + tileSpacing;
        renderedAny = true;
    }

    if (!renderedAny)
    {
        const SDL_Rect cardBounds{
            libraryRect.x + libraryPadding,
            libraryCursorY,
            libraryRect.w - 2 * libraryPadding,
            Scale(180)};
        const bool hasFilter = !filterText.empty();
        if (hasFilter)
        {
            const std::string title = "No matches";
            const std::string message = "Try adjusting your search or sort options.";
            frontend::components::RenderEmptyStateCard(
                renderer,
                theme,
                cardBounds,
                channelFont,
                bodyFont,
                title,
                message,
                timeSeconds);
        }
    }

    return result;
}

} // namespace colony::ui
