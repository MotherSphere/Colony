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

    const int libraryPadding = Scale(22);
    int libraryCursorY = libraryPadding;

    if (activeChannelIndex >= 0 && activeChannelIndex < static_cast<int>(content.channels.size()))
    {
        const auto& activeChannel = content.channels[activeChannelIndex];
        colony::TextTexture channelTitleTexture = colony::CreateTextTexture(renderer, channelFont, activeChannel.label, theme.heroTitle);
        if (channelTitleTexture.texture)
        {
            SDL_Rect channelTitleRect{libraryRect.x + libraryPadding, libraryCursorY, channelTitleTexture.width, channelTitleTexture.height};
            colony::RenderTexture(renderer, channelTitleTexture, channelTitleRect);
            libraryCursorY += channelTitleRect.h + Scale(18);
        }
    }

    SDL_Rect filterRect{libraryRect.x + libraryPadding, libraryCursorY, libraryRect.w - 2 * libraryPadding, Scale(32)};
    SDL_Color filterFill = filterFocused ? colony::color::Mix(theme.libraryCardActive, theme.libraryBackground, 0.55f)
                                         : theme.libraryCard;
    SDL_SetRenderDrawColor(renderer, filterFill.r, filterFill.g, filterFill.b, filterFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, filterRect, 12);
    SDL_Color filterBorder = filterFocused ? theme.channelBadge : theme.border;
    SDL_SetRenderDrawColor(renderer, filterBorder.r, filterBorder.g, filterBorder.b, filterBorder.a);
    colony::drawing::RenderRoundedRect(renderer, filterRect, 12);
    const int filterIconSize = filterRect.h - Scale(10);
    int labelRightBound = filterRect.x + filterRect.w - Scale(10);
    int textStartX = filterRect.x + Scale(12);
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
        textStartX = filterIconRect.x + filterIconRect.w + Scale(10);
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
    }
    else
    {
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
        textStartX = filterRect.x + Scale(10);
    }

    if (labelRightBound > filterRect.x)
    {
        const int textClipWidth = std::max(0, labelRightBound - textStartX);
        SDL_Rect textClip{textStartX, filterRect.y + Scale(4), textClipWidth, filterRect.h - Scale(8)};

        colony::TextTexture inputTexture = colony::CreateTextTexture(renderer, bodyFont, filterText, theme.heroTitle);
        const bool hasInputText = inputTexture.texture != nullptr && inputTexture.width > 0;

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
    libraryCursorY += filterRect.h + Scale(18);

    const int chipPaddingX = Scale(14);
    const int chipHeight = Scale(30);
    const int chipSpacing = Scale(10);
    int chipCursorX = libraryRect.x + libraryPadding;
    int chipCursorY = libraryCursorY;
    const int chipMaxX = libraryRect.x + libraryRect.w - libraryPadding;
    bool renderedChips = false;

    for (const auto& chip : sortChips)
    {
        colony::TextTexture chipTexture = colony::CreateTextTexture(renderer, bodyFont, chip.label, chip.active ? theme.heroTitle : theme.muted);
        const int chipWidth = std::max(chipTexture.texture ? chipTexture.width + chipPaddingX * 2 : chipHeight, Scale(72));

        if (chipCursorX + chipWidth > chipMaxX)
        {
            chipCursorX = libraryRect.x + libraryPadding;
            chipCursorY += chipHeight + chipSpacing;
        }

        SDL_Rect chipRect{chipCursorX, chipCursorY, chipWidth, chipHeight};
        SDL_Color chipFill = chip.active ? theme.libraryCardActive : theme.libraryCard;
        SDL_Color chipBorder = chip.active ? theme.channelBadge : theme.border;

        SDL_SetRenderDrawColor(renderer, chipFill.r, chipFill.g, chipFill.b, chipFill.a);
        colony::drawing::RenderFilledRoundedRect(renderer, chipRect, 12);
        SDL_SetRenderDrawColor(renderer, chipBorder.r, chipBorder.g, chipBorder.b, chipBorder.a);
        colony::drawing::RenderRoundedRect(renderer, chipRect, 12);

        if (chipTexture.texture)
        {
            SDL_Rect textRect{
                chipRect.x + (chipRect.w - chipTexture.width) / 2,
                chipRect.y + (chipRect.h - chipTexture.height) / 2,
                chipTexture.width,
                chipTexture.height};
            colony::RenderTexture(renderer, chipTexture, textRect);
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

    const int tileHeight = Scale(82);
    const int tileSpacing = Scale(14);

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
        colony::drawing::RenderFilledRoundedRect(renderer, tileRect, 14, kRightRoundedCorners);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        colony::drawing::RenderRoundedRect(renderer, tileRect, 14, kRightRoundedCorners);

        const int accentWidth = Scale(4);
        SDL_Rect accentStrip{tileRect.x, tileRect.y, accentWidth, tileRect.h};
        SDL_SetRenderDrawColor(renderer, programIt->second.accent.r, programIt->second.accent.g, programIt->second.accent.b, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer, &accentStrip);

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
        const std::string title = hasFilter ? "No matches" : "Nothing here yet";
        const std::string message = hasFilter ? "Try adjusting your search or sort options."
                                              : "Add applications to populate this channel.";
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

    return result;
}

} // namespace colony::ui
