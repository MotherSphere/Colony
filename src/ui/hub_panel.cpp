#include "ui/hub_panel.hpp"

#include "ui/layout.hpp"

#include "utils/color.hpp"
#include "utils/drawing.hpp"
#include "utils/text_wrapping.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace colony::ui
{
void HubPanelRenderer::Build(
    SDL_Renderer* renderer,
    const HubContent& content,
    TTF_Font* headlineFont,
    TTF_Font* heroBodyFont,
    TTF_Font* tileTitleFont,
    TTF_Font* tileBodyFont,
    const ThemeColors& theme)
{
    heroBodyFont_ = heroBodyFont;
    tileBodyFont_ = tileBodyFont;

    hero_.headline = colony::CreateTextTexture(renderer, headlineFont, content.headline, theme.heroTitle);
    hero_.description = content.description;
    hero_.descriptionWidth = 0;
    hero_.descriptionLines.clear();
    hero_.highlightChips.clear();
    hero_.primaryActionLabel = {};
    hero_.primaryActionDescription = content.primaryActionDescription;
    hero_.actionDescriptionWidth = 0;
    hero_.actionDescriptionLines.clear();

    search_.placeholder = content.searchPlaceholder;
    search_.lastQuery.clear();
    search_.queryTexture = {};
    if (tileBodyFont_ != nullptr && !search_.placeholder.empty())
    {
        search_.placeholderTexture =
            colony::CreateTextTexture(renderer, tileBodyFont_, search_.placeholder, theme.statusBarText);
    }
    else
    {
        search_.placeholderTexture = {};
    }

    if (heroBodyFont_ != nullptr)
    {
        hero_.highlightChips.reserve(content.highlights.size());
        for (const auto& highlight : content.highlights)
        {
            hero_.highlightChips.emplace_back(
                colony::CreateTextTexture(renderer, heroBodyFont_, highlight, theme.statusBarText));
        }
    }
    if (!content.primaryActionLabel.empty() && tileTitleFont != nullptr)
    {
        hero_.primaryActionLabel =
            colony::CreateTextTexture(renderer, tileTitleFont, content.primaryActionLabel, theme.heroTitle);
    }

    branches_.clear();
    branches_.reserve(content.branches.size());
    for (const auto& branchContent : content.branches)
    {
        BranchChrome branch;
        branch.id = branchContent.id;
        branch.description = branchContent.description;
        branch.accent = branchContent.accent;
        branch.descriptionWidth = 0;
        branch.bodyLines.clear();
        branch.title = colony::CreateTextTexture(renderer, tileTitleFont, branchContent.title, theme.heroTitle);
        branch.tagChips.clear();
        branch.channelLabelText = branchContent.channelLabel;
        branch.programLabelText = branchContent.programLabel;
        branch.detailBullets = branchContent.detailBullets;
        branch.channelLabel = {};
        branch.programLabel = {};
        branch.detailBulletLines.clear();
        branch.detailBodyLines.clear();
        branch.detailBodyWidth = 0;
        branch.detailBulletWidth = 0;
        if (tileBodyFont_ != nullptr)
        {
            for (const auto& tag : branchContent.tags)
            {
                branch.tagChips.emplace_back(
                    colony::CreateTextTexture(renderer, tileBodyFont_, tag, theme.libraryCardActive));
            }
            if (!branch.channelLabelText.empty())
            {
                branch.channelLabel =
                    colony::CreateTextTexture(renderer, tileBodyFont_, branch.channelLabelText, theme.statusBarText);
            }
            if (!branch.programLabelText.empty())
            {
                branch.programLabel =
                    colony::CreateTextTexture(renderer, tileBodyFont_, branch.programLabelText, theme.statusBarText);
            }
        }
        if (!branchContent.actionLabel.empty() && tileBodyFont_ != nullptr)
        {
            branch.actionLabel =
                colony::CreateTextTexture(renderer, tileBodyFont_, branchContent.actionLabel, theme.heroTitle);
        }
        if (!branchContent.metrics.empty() && tileBodyFont_ != nullptr)
        {
            branch.metricsLabel =
                colony::CreateTextTexture(renderer, tileBodyFont_, branchContent.metrics, theme.statusBarText);
        }
        if (tileTitleFont != nullptr)
        {
            std::string glyph = branchContent.title.empty() ? branchContent.id : branchContent.title;
            if (!glyph.empty())
            {
                const char32_t codePoint = static_cast<unsigned char>(glyph.front());
                std::string glyphText;
                if ((codePoint & 0x80) == 0)
                {
                    glyphText.assign(1, static_cast<char>(std::toupper(static_cast<unsigned char>(glyph.front()))));
                }
                else
                {
                    glyphText = glyph.substr(0, std::min<std::size_t>(glyph.size(), 4));
                }
                branch.iconGlyph = colony::CreateTextTexture(renderer, tileTitleFont, glyphText, theme.heroTitle);
            }
        }
        branches_.emplace_back(std::move(branch));
    }

    widgets_.clear();
    widgets_.reserve(content.widgets.size());
    for (const auto& widgetContent : content.widgets)
    {
        WidgetChrome widget;
        widget.id = widgetContent.id;
        widget.title = colony::CreateTextTexture(renderer, tileTitleFont, widgetContent.title, theme.heroTitle);
        widget.description = widgetContent.description;
        widget.descriptionWidth = 0;
        widget.descriptionLines.clear();
        widget.items = widgetContent.items;
        widget.itemsWidth = 0;
        widget.itemLines.clear();
        widget.accent = widgetContent.accent;
        widgets_.emplace_back(std::move(widget));
    }
}

HubRenderResult HubPanelRenderer::Render(
    SDL_Renderer* renderer,
    const ThemeColors& theme,
    const SDL_Rect& bounds,
    double timeSeconds,
    int hoveredBranchIndex,
    int activeBranchIndex,
    int detailBranchIndex,
    int scrollOffset,
    bool heroCollapsed,
    std::string_view searchQuery,
    bool searchFocused,
    int widgetPage,
    int widgetsPerPage) const
{
    HubRenderResult result{};

    if (!renderer)
    {
        return result;
    }

    widgetsPerPage = std::max(1, widgetsPerPage);
    const int widgetCount = static_cast<int>(widgets_.size());
    const int widgetPageCount = widgetCount == 0 ? 0 : (widgetCount + widgetsPerPage - 1) / widgetsPerPage;
    if (widgetPageCount > 0)
    {
        widgetPage = std::clamp(widgetPage, 0, widgetPageCount - 1);
    }
    else
    {
        widgetPage = 0;
    }
    result.widgetPageCount = widgetPageCount;

    if (detailBranchIndex < 0 || detailBranchIndex >= static_cast<int>(branches_.size()))
    {
        detailBranchIndex = activeBranchIndex;
    }

    const int clampedScrollOffset = std::max(0, scrollOffset);

    SDL_SetRenderDrawColor(renderer, theme.background.r, theme.background.g, theme.background.b, theme.background.a);
    SDL_RenderFillRect(renderer, &bounds);

    int heroHeight = heroCollapsed ? std::max(Scale(180), bounds.h / 3) : std::max(Scale(320), bounds.h / 2);
    const int heroMaxHeight = std::max(bounds.h - Scale(heroCollapsed ? 60 : 80), heroCollapsed ? Scale(220) : Scale(320));
    heroHeight = std::min(heroHeight, heroMaxHeight);
    heroHeight = std::clamp(heroHeight, std::min(bounds.h, heroCollapsed ? Scale(160) : Scale(240)), bounds.h);
    if (heroHeight <= 0)
    {
        heroHeight = bounds.h;
    }

    SDL_Rect heroRect{bounds.x, bounds.y, bounds.w, heroHeight};
    result.heroRect = heroRect;

    SDL_Color gradientStart = theme.heroGradientFallbackStart;
    SDL_Color gradientEnd = theme.heroGradientFallbackEnd;
    SDL_SetRenderDrawColor(renderer, gradientStart.r, gradientStart.g, gradientStart.b, gradientStart.a);
    SDL_RenderFillRect(renderer, &heroRect);

    SDL_Rect bottomBlend = heroRect;
    bottomBlend.h /= 2;
    bottomBlend.y += bottomBlend.h;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, gradientEnd.r, gradientEnd.g, gradientEnd.b, 180);
    SDL_RenderFillRect(renderer, &bottomBlend);

    const auto resolveAccent = [&](const BranchChrome& branch) {
        if (branch.accent.a == 0)
        {
            return theme.channelBadge;
        }
        return branch.accent;
    };

    SDL_Color accentColor = branches_.empty() ? theme.channelBadge : resolveAccent(branches_.front());
    const int accentStripWidth = Scale(64);
    SDL_Rect accentStrip{
        heroRect.x + heroRect.w - accentStripWidth - Scale(32),
        heroRect.y + Scale(32),
        accentStripWidth,
        heroRect.h - Scale(64)};
    if (accentStrip.w > 0 && accentStrip.h > 0)
    {
        SDL_Color stripFill = colony::color::Mix(accentColor, gradientEnd, 0.25f);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, stripFill.r, stripFill.g, stripFill.b, 200);
        colony::drawing::RenderFilledRoundedRect(renderer, accentStrip, Scale(24));
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    const int heroPadding = Scale(heroCollapsed ? 32 : 48);
    const int heroContentWidth = std::max(0, heroRect.w - heroPadding * 2);
    const int heroContentX = heroRect.x + heroPadding;
    int heroCursorY = heroRect.y + heroPadding;
    const int heroTextWidth = heroRect.w >= Scale(900) ? heroContentWidth / 2 : heroContentWidth;

    RebuildHeroDescription(renderer, heroTextWidth, theme.heroBody);

    if (hero_.headline.texture)
    {
        SDL_Rect accentRule{heroContentX, heroCursorY - Scale(18), Scale(54), Scale(6)};
        SDL_Color accentRuleColor = colony::color::Mix(accentColor, theme.heroTitle, 0.35f);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, accentRuleColor.r, accentRuleColor.g, accentRuleColor.b, 140);
        colony::drawing::RenderFilledRoundedRect(renderer, accentRule, accentRule.h / 2);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_Rect headlineRect{heroContentX, heroCursorY, hero_.headline.width, hero_.headline.height};
        colony::RenderTexture(renderer, hero_.headline, headlineRect);
        heroCursorY += headlineRect.h + Scale(heroCollapsed ? 8 : 20);
    }

    const int heroLineSkip = heroBodyFont_ ? TTF_FontLineSkip(heroBodyFont_) : 0;
    if (!heroCollapsed)
    {
        for (std::size_t i = 0; i < hero_.descriptionLines.size(); ++i)
        {
            const auto& lineTexture = hero_.descriptionLines[i];
            SDL_Rect lineRect{heroContentX, heroCursorY, lineTexture.width, lineTexture.height};
            colony::RenderTexture(renderer, lineTexture, lineRect);
            heroCursorY += lineTexture.height;
            if (heroLineSkip > 0 && i + 1 < hero_.descriptionLines.size())
            {
                heroCursorY += std::max(0, heroLineSkip - lineTexture.height);
            }
            else if (i + 1 < hero_.descriptionLines.size())
            {
                heroCursorY += Scale(8);
            }
        }

        heroCursorY += Scale(18);

        if (!hero_.highlightChips.empty())
        {
            const int chipPaddingX = Scale(14);
            const int chipPaddingY = Scale(8);
            const int chipGap = Scale(10);
            const int chipMaxWidth = heroTextWidth;
            int chipCursorX = heroContentX;
            int chipCursorY = heroCursorY;
            int chipLineHeight = 0;
            SDL_Color chipFill = colony::color::Mix(accentColor, theme.libraryBackground, 0.2f);
            SDL_Color chipOutline = colony::color::Mix(accentColor, theme.border, 0.3f);
            for (const auto& chipTexture : hero_.highlightChips)
            {
                const int chipWidth = chipTexture.width + chipPaddingX * 2;
                const int chipHeight = chipTexture.height + chipPaddingY * 2;
                if (chipCursorX > heroContentX && chipCursorX + chipWidth > heroContentX + chipMaxWidth)
                {
                    chipCursorX = heroContentX;
                    chipCursorY += chipLineHeight + chipGap;
                    chipLineHeight = 0;
                }
                SDL_Rect chipRect{chipCursorX, chipCursorY, chipWidth, chipHeight};
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, chipFill.r, chipFill.g, chipFill.b, 200);
                colony::drawing::RenderFilledRoundedRect(renderer, chipRect, Scale(16));
                SDL_SetRenderDrawColor(renderer, chipOutline.r, chipOutline.g, chipOutline.b, 180);
                colony::drawing::RenderRoundedRect(renderer, chipRect, Scale(16));
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                SDL_Rect chipTextRect{
                    chipRect.x + chipPaddingX,
                    chipRect.y + chipPaddingY,
                    chipTexture.width,
                    chipTexture.height};
                colony::RenderTexture(renderer, chipTexture, chipTextRect);

                chipCursorX += chipWidth + chipGap;
                chipLineHeight = std::max(chipLineHeight, chipHeight);
            }
            heroCursorY = chipCursorY + chipLineHeight + Scale(20);
        }
    }

    if (hero_.primaryActionLabel.texture && !heroCollapsed)
    {
        const int buttonPaddingX = Scale(32);
        const int buttonPaddingY = Scale(16);
        const int buttonExtra = Scale(32);
        const int buttonWidth = hero_.primaryActionLabel.width + buttonPaddingX * 2 + buttonExtra;
        const int buttonHeight = hero_.primaryActionLabel.height + buttonPaddingY * 2;
        SDL_Rect buttonRect{heroContentX, heroCursorY, buttonWidth, buttonHeight};

        SDL_Color buttonFill = colony::color::Mix(accentColor, theme.libraryBackground, 0.25f);
        SDL_Color buttonOutline = colony::color::Mix(accentColor, theme.border, 0.3f);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, buttonFill.r, buttonFill.g, buttonFill.b, 220);
        colony::drawing::RenderFilledRoundedRect(renderer, buttonRect, buttonRect.h / 2);
        SDL_SetRenderDrawColor(renderer, buttonOutline.r, buttonOutline.g, buttonOutline.b, 200);
        colony::drawing::RenderRoundedRect(renderer, buttonRect, buttonRect.h / 2);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_Rect labelRect{
            buttonRect.x + buttonPaddingX,
            buttonRect.y + buttonPaddingY,
            hero_.primaryActionLabel.width,
            hero_.primaryActionLabel.height};
        colony::RenderTexture(renderer, hero_.primaryActionLabel, labelRect);

        const int arrowSize = Scale(18);
        SDL_Rect arrowRect{
            buttonRect.x + buttonRect.w - arrowSize - Scale(18),
            buttonRect.y + buttonRect.h / 2 - arrowSize / 2,
            arrowSize,
            arrowSize};
        SDL_SetRenderDrawColor(renderer, buttonOutline.r, buttonOutline.g, buttonOutline.b, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(renderer, arrowRect.x, arrowRect.y + arrowRect.h / 2, arrowRect.x + arrowRect.w, arrowRect.y + arrowRect.h / 2);
        SDL_RenderDrawLine(
            renderer,
            arrowRect.x + arrowRect.w / 2,
            arrowRect.y,
            arrowRect.x + arrowRect.w,
            arrowRect.y + arrowRect.h / 2);
        SDL_RenderDrawLine(
            renderer,
            arrowRect.x + arrowRect.w / 2,
            arrowRect.y + arrowRect.h,
            arrowRect.x + arrowRect.w,
            arrowRect.y + arrowRect.h / 2);

        heroCursorY = buttonRect.y + buttonRect.h + Scale(18);
    }

    RebuildHeroActionDescription(renderer, heroTextWidth, theme.statusBarText);
    if (!heroCollapsed && !hero_.actionDescriptionLines.empty())
    {
        const int actionLineSkip = heroBodyFont_ ? TTF_FontLineSkip(heroBodyFont_) : 0;
        for (std::size_t i = 0; i < hero_.actionDescriptionLines.size(); ++i)
        {
            const auto& lineTexture = hero_.actionDescriptionLines[i];
            SDL_Rect lineRect{heroContentX, heroCursorY, lineTexture.width, lineTexture.height};
            colony::RenderTexture(renderer, lineTexture, lineRect);
            heroCursorY += lineTexture.height;
            if (i + 1 < hero_.actionDescriptionLines.size())
            {
                if (actionLineSkip > 0)
                {
                    heroCursorY += std::max(0, actionLineSkip - lineTexture.height);
                }
                else
                {
                    heroCursorY += Scale(6);
                }
            }
        }
        heroCursorY += Scale(16);
    }

    const int toggleSize = Scale(48);
    SDL_Rect toggleRect{
        heroRect.x + heroRect.w - heroPadding - toggleSize,
        heroRect.y + heroRect.h - toggleSize - heroPadding / 2,
        toggleSize,
        toggleSize};
    result.heroToggleRect = toggleRect;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Color toggleFill = colony::color::Mix(accentColor, theme.libraryBackground, heroCollapsed ? 0.18f : 0.28f);
    SDL_Color toggleOutline = colony::color::Mix(accentColor, theme.border, 0.35f);
    SDL_SetRenderDrawColor(renderer, toggleFill.r, toggleFill.g, toggleFill.b, 220);
    colony::drawing::RenderFilledRoundedRect(renderer, toggleRect, Scale(16));
    SDL_SetRenderDrawColor(renderer, toggleOutline.r, toggleOutline.g, toggleOutline.b, 200);
    colony::drawing::RenderRoundedRect(renderer, toggleRect, Scale(16));
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, SDL_ALPHA_OPAQUE);
    const int chevronWidth = Scale(16);
    const int chevronHeight = Scale(10);
    const int chevronSpacing = Scale(4);
    int centerX = toggleRect.x + toggleRect.w / 2;
    int centerY = toggleRect.y + toggleRect.h / 2;
    auto drawChevron = [&](int offsetY, bool pointingDown) {
        const int halfWidth = chevronWidth / 2;
        const int direction = pointingDown ? 1 : -1;
        SDL_RenderDrawLine(
            renderer,
            centerX - halfWidth,
            centerY + offsetY - direction * chevronHeight / 2,
            centerX,
            centerY + offsetY + direction * chevronHeight / 2);
        SDL_RenderDrawLine(
            renderer,
            centerX + halfWidth,
            centerY + offsetY - direction * chevronHeight / 2,
            centerX,
            centerY + offsetY + direction * chevronHeight / 2);
    };
    drawChevron(-chevronSpacing, heroCollapsed);
    drawChevron(chevronSpacing, heroCollapsed);

    const int searchBarWidth = std::min(heroContentWidth, Scale(420));
    const int searchBarHeight = Scale(54);
    SDL_Rect searchRect{
        heroContentX + heroContentWidth - searchBarWidth,
        heroRect.y + heroPadding,
        searchBarWidth,
        searchBarHeight};
    result.searchInputRect = searchRect;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Color searchFill = colony::color::Mix(theme.libraryBackground, theme.background, 0.35f);
    SDL_SetRenderDrawColor(renderer, searchFill.r, searchFill.g, searchFill.b, 230);
    colony::drawing::RenderFilledRoundedRect(renderer, searchRect, Scale(22));
    SDL_Color searchOutline = searchFocused ? accentColor : theme.border;
    SDL_SetRenderDrawColor(renderer, searchOutline.r, searchOutline.g, searchOutline.b, searchFocused ? 220 : 180);
    colony::drawing::RenderRoundedRect(renderer, searchRect, Scale(22));
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    if (search_.lastQuery != searchQuery)
    {
        search_.lastQuery = std::string(searchQuery);
        if (tileBodyFont_ != nullptr && !search_.lastQuery.empty())
        {
            search_.queryTexture =
                colony::CreateTextTexture(renderer, tileBodyFont_, search_.lastQuery, theme.heroTitle);
        }
        else
        {
            search_.queryTexture = {};
        }
    }

    const int searchPaddingX = Scale(18);
    const int searchPaddingY = Scale(12);
    const int searchIconSize = Scale(18);
    SDL_Rect iconRect{
        searchRect.x + searchPaddingX,
        searchRect.y + searchRect.h / 2 - searchIconSize / 2,
        searchIconSize,
        searchIconSize};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, theme.libraryBackground.r, theme.libraryBackground.g, theme.libraryBackground.b, 220);
    colony::drawing::RenderFilledRoundedRect(renderer, iconRect, iconRect.w / 2);
    SDL_SetRenderDrawColor(renderer, theme.muted.r, theme.muted.g, theme.muted.b, SDL_ALPHA_OPAQUE);
    colony::drawing::RenderRoundedRect(renderer, iconRect, iconRect.w / 2);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_RenderDrawLine(
        renderer,
        iconRect.x + iconRect.w - Scale(4),
        iconRect.y + iconRect.h - Scale(4),
        iconRect.x + iconRect.w + Scale(4),
        iconRect.y + iconRect.h + Scale(4));

    SDL_Rect searchTextRect{
        searchRect.x + searchPaddingX + searchIconSize + Scale(12),
        searchRect.y + searchPaddingY,
        searchRect.w - searchPaddingX * 2,
        searchRect.h - searchPaddingY * 2};
    searchTextRect.w -= searchIconSize + Scale(12);

    if (!search_.queryTexture.texture && search_.placeholderTexture.texture)
    {
        SDL_Rect placeholderRect{
            searchTextRect.x,
            searchTextRect.y + searchTextRect.h / 2 - search_.placeholderTexture.height / 2,
            search_.placeholderTexture.width,
            search_.placeholderTexture.height};
        colony::RenderTexture(renderer, search_.placeholderTexture, placeholderRect);
    }
    else if (search_.queryTexture.texture)
    {
        SDL_Rect queryRect{
            searchTextRect.x,
            searchTextRect.y + searchTextRect.h / 2 - search_.queryTexture.height / 2,
            search_.queryTexture.width,
            search_.queryTexture.height};
        colony::RenderTexture(renderer, search_.queryTexture, queryRect);
        if (searchFocused)
        {
            const double blink = std::fmod(timeSeconds, 1.0);
            if (blink < 0.6)
            {
                SDL_Rect caretRect{queryRect.x + queryRect.w + Scale(4), queryRect.y, Scale(2), queryRect.h};
                SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, SDL_ALPHA_OPAQUE);
                SDL_RenderFillRect(renderer, &caretRect);
            }
        }
    }
    else if (searchFocused)
    {
        const double blink = std::fmod(timeSeconds, 1.0);
        if (blink < 0.6)
        {
            SDL_Rect caretRect{searchTextRect.x, searchTextRect.y, Scale(2), searchTextRect.h};
            SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(renderer, &caretRect);
        }
    }

    if (!search_.queryTexture.texture)
    {
        result.searchClearRect = SDL_Rect{0, 0, 0, 0};
    }
    else
    {
        const int clearSize = Scale(20);
        SDL_Rect clearRect{
            searchRect.x + searchRect.w - searchPaddingX - clearSize,
            searchRect.y + searchRect.h / 2 - clearSize / 2,
            clearSize,
            clearSize};
        result.searchClearRect = clearRect;
        SDL_SetRenderDrawColor(renderer, theme.muted.r, theme.muted.g, theme.muted.b, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(
            renderer,
            clearRect.x + Scale(4),
            clearRect.y + Scale(4),
            clearRect.x + clearRect.w - Scale(4),
            clearRect.y + clearRect.h - Scale(4));
        SDL_RenderDrawLine(
            renderer,
            clearRect.x + Scale(4),
            clearRect.y + clearRect.h - Scale(4),
            clearRect.x + clearRect.w - Scale(4),
            clearRect.y + Scale(4));
    }

    const int scrollTop = heroRect.y + heroRect.h + Scale(heroCollapsed ? 24 : 40);
    const int scrollViewportHeight = std::max(0, bounds.y + bounds.h - scrollTop);
    result.scrollViewport = SDL_Rect{bounds.x, scrollTop, bounds.w, scrollViewportHeight};

    SDL_Rect clipRect = result.scrollViewport;
    if (clipRect.h > 0)
    {
        SDL_RenderSetClipRect(renderer, &clipRect);
    }

    const int gridPaddingX = Scale(40);
    const int gridWidth = std::max(0, bounds.w - gridPaddingX * 2);
    const int tileGap = Scale(24);
    int columns = 1;
    if (gridWidth >= Scale(960))
    {
        columns = 3;
    }
    else if (gridWidth >= Scale(600))
    {
        columns = 2;
    }

    int baseTileWidth = columns > 0 ? (gridWidth - tileGap * (columns - 1)) / std::max(columns, 1) : gridWidth;
    if (columns == 1)
    {
        baseTileWidth = gridWidth;
    }
    int tileWidth = std::max(Scale(240), baseTileWidth);
    if (gridWidth > 0)
    {
        tileWidth = std::min(tileWidth, gridWidth);
    }

    const bool showSideColumn = (detailBranchIndex >= 0 || widgetCount > 0) && columns >= 2;
    const int sideColumnIndex = showSideColumn ? columns - 1 : -1;
    const int branchColumnCount = showSideColumn ? columns - 1 : columns;

    std::vector<int> columnOffsets(static_cast<std::size_t>(columns), scrollTop + clampedScrollOffset);
    std::vector<int> columnPositions(static_cast<std::size_t>(columns), 0);
    for (int c = 0; c < columns; ++c)
    {
        columnPositions[static_cast<std::size_t>(c)] = bounds.x + gridPaddingX + c * (tileWidth + tileGap);
    }

    std::vector<int> branchColumns;
    branchColumns.reserve(static_cast<std::size_t>(std::max(branchColumnCount, 1)));
    for (int c = 0; c < columns; ++c)
    {
        if (c == sideColumnIndex)
        {
            continue;
        }
        branchColumns.push_back(c);
    }
    if (branchColumns.empty())
    {
        branchColumns.push_back(0);
    }

    result.branchHitboxes.clear();
    result.branchHitboxes.reserve(branches_.size());

    const int tilePadding = Scale(28);
    const int iconSize = Scale(60);
    const int iconSpacing = Scale(24);
    const int tagPaddingX = Scale(14);
    const int tagPaddingY = Scale(8);
    const int tagGap = Scale(8);

    for (std::size_t index = 0; index < branches_.size(); ++index)
    {
        const int columnCandidate = branchColumns[static_cast<std::size_t>(index % branchColumns.size())];
        int bestColumn = columnCandidate;
        for (int candidate : branchColumns)
        {
            if (columnOffsets[static_cast<std::size_t>(candidate)] < columnOffsets[static_cast<std::size_t>(bestColumn)])
            {
                bestColumn = candidate;
            }
        }

        BranchChrome& branch = branches_[index];
        const int textWidth = std::max(0, tileWidth - tilePadding * 2 - iconSize - iconSpacing);
        RebuildBranchDescription(renderer, branch, textWidth, theme.heroBody);

        int tileHeight = tilePadding * 2 + iconSize;
        if (!branch.tagChips.empty())
        {
            int chipCursorWidth = 0;
            int chipLineHeight = 0;
            for (const auto& chip : branch.tagChips)
            {
                const int chipWidth = chip.width + tagPaddingX * 2;
                const int chipHeight = chip.height + tagPaddingY * 2;
                if (chipCursorWidth > 0 && chipCursorWidth + chipWidth > textWidth)
                {
                    tileHeight += chipLineHeight + tagGap;
                    chipCursorWidth = 0;
                    chipLineHeight = 0;
                }
                chipCursorWidth += chipWidth + tagGap;
                chipLineHeight = std::max(chipLineHeight, chipHeight);
            }
            tileHeight += chipLineHeight + Scale(18);
        }

        if (!branch.bodyLines.empty())
        {
            const int lineSkip = tileBodyFont_ ? TTF_FontLineSkip(tileBodyFont_) : 0;
            for (std::size_t i = 0; i < branch.bodyLines.size(); ++i)
            {
                tileHeight += branch.bodyLines[i].height;
                if (i + 1 < branch.bodyLines.size())
                {
                    tileHeight += lineSkip > 0 ? std::max(0, lineSkip - branch.bodyLines[i].height) : Scale(6);
                }
            }
            tileHeight += Scale(24);
        }

        if (branch.metricsLabel.texture)
        {
            tileHeight += Scale(18) + branch.metricsLabel.height;
        }

        const int actionButtonHeight = branch.actionLabel.texture ? branch.actionLabel.height + Scale(16) * 2 : Scale(42);
        tileHeight += actionButtonHeight + Scale(24);
        tileHeight = std::max(tileHeight, Scale(220));

        SDL_Rect tileRect{
            columnPositions[static_cast<std::size_t>(bestColumn)],
            columnOffsets[static_cast<std::size_t>(bestColumn)] - clampedScrollOffset,
            tileWidth,
            tileHeight};
        columnOffsets[static_cast<std::size_t>(bestColumn)] += tileHeight + tileGap;

        const bool isHovered = hoveredBranchIndex == static_cast<int>(index);
        const bool isActive = activeBranchIndex == static_cast<int>(index);

        SDL_Rect drawRect = tileRect;
        if (isHovered)
        {
            drawRect.y -= Scale(2);
        }

        SDL_Color branchAccent = resolveAccent(branch);
        SDL_Color baseFill = isActive
            ? theme.libraryCardActive
            : (isHovered ? theme.libraryCardHover : theme.libraryCard);
        SDL_Color outline = isActive ? branchAccent : theme.border;
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, baseFill.r, baseFill.g, baseFill.b, 230);
        colony::drawing::RenderFilledRoundedRect(renderer, drawRect, Scale(22));
        SDL_SetRenderDrawColor(renderer, outline.r, outline.g, outline.b, isActive ? 220 : 160);
        colony::drawing::RenderRoundedRect(renderer, drawRect, Scale(22));
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_Rect accentBar{drawRect.x, drawRect.y, Scale(6), drawRect.h};
        SDL_SetRenderDrawColor(renderer, branchAccent.r, branchAccent.g, branchAccent.b, SDL_ALPHA_OPAQUE);
        colony::drawing::RenderFilledRoundedRect(renderer, accentBar, Scale(6), colony::drawing::CornerTopLeft | colony::drawing::CornerBottomLeft);

        SDL_Rect iconRect{drawRect.x + tilePadding, drawRect.y + tilePadding, iconSize, iconSize};
        SDL_Color iconFill = colony::color::Mix(branchAccent, theme.background, 0.2f);
        SDL_Color iconOutline = colony::color::Mix(branchAccent, theme.border, 0.4f);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        colony::drawing::RenderFilledRoundedRect(renderer, iconRect, iconRect.w / 2);
        SDL_SetRenderDrawColor(renderer, iconOutline.r, iconOutline.g, iconOutline.b, 200);
        colony::drawing::RenderRoundedRect(renderer, iconRect, iconRect.w / 2);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        if (branch.iconGlyph.texture)
        {
            SDL_Rect glyphRect{
                iconRect.x + iconRect.w / 2 - branch.iconGlyph.width / 2,
                iconRect.y + iconRect.h / 2 - branch.iconGlyph.height / 2,
                branch.iconGlyph.width,
                branch.iconGlyph.height};
            colony::RenderTexture(renderer, branch.iconGlyph, glyphRect);
        }

        const int textStartX = iconRect.x + iconRect.w + iconSpacing;
        int textCursorY = drawRect.y + tilePadding;

        SDL_Rect tileAccentBar{textStartX, textCursorY - Scale(14), Scale(48), Scale(6)};
        SDL_Color barColor = colony::color::Mix(branchAccent, theme.heroTitle, 0.45f);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, barColor.r, barColor.g, barColor.b, 160);
        colony::drawing::RenderFilledRoundedRect(renderer, tileAccentBar, tileAccentBar.h / 2);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        if (branch.title.texture)
        {
            SDL_Rect titleRect{textStartX, textCursorY, branch.title.width, branch.title.height};
            colony::RenderTexture(renderer, branch.title, titleRect);
            textCursorY += branch.title.height;
        }

        if (!branch.tagChips.empty())
        {
            if (branch.title.texture)
            {
                textCursorY += Scale(10);
            }
            int chipCursorX = textStartX;
            int chipCursorY = textCursorY;
            int chipLineHeight = 0;
            for (const auto& chip : branch.tagChips)
            {
                const int chipWidth = chip.width + tagPaddingX * 2;
                const int chipHeight = chip.height + tagPaddingY * 2;
                if (chipCursorX > textStartX && chipCursorX + chipWidth > textStartX + textWidth)
                {
                    chipCursorX = textStartX;
                    chipCursorY += chipLineHeight + tagGap;
                    chipLineHeight = 0;
                }
                SDL_Rect chipRect{chipCursorX, chipCursorY, chipWidth, chipHeight};
                SDL_Color chipFill = colony::color::Mix(branchAccent, theme.libraryBackground, 0.2f);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, chipFill.r, chipFill.g, chipFill.b, 210);
                colony::drawing::RenderFilledRoundedRect(renderer, chipRect, Scale(14));
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                SDL_Rect chipTextRect{chipRect.x + tagPaddingX, chipRect.y + tagPaddingY, chip.width, chip.height};
                colony::RenderTexture(renderer, chip, chipTextRect);

                chipCursorX += chipWidth + tagGap;
                chipLineHeight = std::max(chipLineHeight, chipHeight);
            }
            textCursorY = chipCursorY + chipLineHeight;
        }

        textCursorY = std::max(textCursorY, iconRect.y + iconRect.h);
        if (!branch.bodyLines.empty())
        {
            textCursorY += Scale(14);
        }

        for (const auto& lineTexture : branch.bodyLines)
        {
            SDL_Rect bodyRect{textStartX, textCursorY, lineTexture.width, lineTexture.height};
            colony::RenderTexture(renderer, lineTexture, bodyRect);
            textCursorY += lineTexture.height;
            if (&lineTexture != &branch.bodyLines.back())
            {
                const int lineSkip = tileBodyFont_ ? TTF_FontLineSkip(tileBodyFont_) : 0;
                if (lineSkip > 0)
                {
                    textCursorY += std::max(0, lineSkip - lineTexture.height);
                }
                else
                {
                    textCursorY += Scale(6);
                }
            }
        }

        if (branch.metricsLabel.texture)
        {
            textCursorY += Scale(18);
            SDL_Rect metricsRect{textStartX, textCursorY, branch.metricsLabel.width, branch.metricsLabel.height};
            colony::RenderTexture(renderer, branch.metricsLabel, metricsRect);
            textCursorY += branch.metricsLabel.height;
        }

        const int buttonPaddingX = Scale(18);
        const int buttonPaddingY = Scale(12);
        const int buttonIconSize = Scale(16);
        SDL_Rect actionRect{
            drawRect.x + drawRect.w - buttonPaddingX * 2 - buttonIconSize - Scale(12)
                - (branch.actionLabel.texture ? branch.actionLabel.width : Scale(120)),
            drawRect.y + drawRect.h - (branch.actionLabel.texture ? branch.actionLabel.height + buttonPaddingY * 2 : Scale(42))
                - tilePadding,
            branch.actionLabel.texture ? branch.actionLabel.width + buttonPaddingX * 2 + buttonIconSize + Scale(12) : Scale(180),
            branch.actionLabel.texture ? branch.actionLabel.height + buttonPaddingY * 2 : Scale(42)};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_Color buttonFill = colony::color::Mix(branchAccent, theme.libraryBackground, 0.3f);
        SDL_Color buttonOutline = colony::color::Mix(branchAccent, theme.border, 0.35f);
        colony::drawing::RenderFilledRoundedRect(renderer, actionRect, Scale(18));
        SDL_SetRenderDrawColor(renderer, buttonOutline.r, buttonOutline.g, buttonOutline.b, 210);
        colony::drawing::RenderRoundedRect(renderer, actionRect, Scale(18));
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        if (branch.actionLabel.texture)
        {
            SDL_Rect actionLabelRect{
                actionRect.x + buttonPaddingX,
                actionRect.y + buttonPaddingY,
                branch.actionLabel.width,
                branch.actionLabel.height};
            colony::RenderTexture(renderer, branch.actionLabel, actionLabelRect);
        }

        SDL_Rect buttonArrowRect{
            actionRect.x + actionRect.w - buttonIconSize - buttonPaddingX,
            actionRect.y + actionRect.h / 2 - buttonIconSize / 2,
            buttonIconSize,
            buttonIconSize};
        SDL_SetRenderDrawColor(renderer, buttonOutline.r, buttonOutline.g, buttonOutline.b, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(renderer, buttonArrowRect.x, buttonArrowRect.y + buttonArrowRect.h / 2, buttonArrowRect.x + buttonArrowRect.w, buttonArrowRect.y + buttonArrowRect.h / 2);
        SDL_RenderDrawLine(
            renderer,
            buttonArrowRect.x + buttonArrowRect.w / 2,
            buttonArrowRect.y,
            buttonArrowRect.x + buttonArrowRect.w,
            buttonArrowRect.y + buttonArrowRect.h / 2);
        SDL_RenderDrawLine(
            renderer,
            buttonArrowRect.x + buttonArrowRect.w / 2,
            buttonArrowRect.y + buttonArrowRect.h,
            buttonArrowRect.x + buttonArrowRect.w,
            buttonArrowRect.y + buttonArrowRect.h / 2);


        SDL_Rect screenRect = drawRect;
        if (screenRect.y + screenRect.h >= clipRect.y && screenRect.y <= clipRect.y + clipRect.h)
        {
            result.branchHitboxes.push_back(HubRenderResult::BranchHitbox{branch.id, screenRect, static_cast<int>(index)});
        }
    }

    if (branches_.empty() && tileBodyFont_ != nullptr)
    {
        const char* emptyMessage = searchQuery.empty() ? "Aucune destination disponible" : "Aucun résultat";
        colony::TextTexture emptyText = colony::CreateTextTexture(renderer, tileBodyFont_, emptyMessage, theme.statusBarText);
        SDL_Rect messageRect{
            bounds.x + bounds.w / 2 - emptyText.width / 2,
            scrollTop - clampedScrollOffset + Scale(40),
            emptyText.width,
            emptyText.height};
        colony::RenderTexture(renderer, emptyText, messageRect);
    }

    int branchContentBottom = scrollTop;
    for (int c : branchColumns)
    {
        branchContentBottom = std::max(branchContentBottom, columnOffsets[static_cast<std::size_t>(c)] - clampedScrollOffset);
    }

    int sideColumnCursor = sideColumnIndex >= 0 ? columnOffsets[static_cast<std::size_t>(sideColumnIndex)] - clampedScrollOffset
                                               : branchContentBottom;

    const int bulletSize = Scale(8);
    const int bulletIndent = Scale(18);
    const int itemLineSkip = tileBodyFont_ ? TTF_FontLineSkip(tileBodyFont_) : 0;

    auto renderWidget = [&](WidgetChrome& widget, int widgetX, int widgetWidth, int& widgetCursorY) {
        const int widgetPadding = Scale(28);
        const int widgetSpacing = Scale(24);

        RebuildWidgetDescription(renderer, widget, widgetWidth - widgetPadding * 2, theme.heroBody);
        RebuildWidgetItems(renderer, widget, widgetWidth - widgetPadding * 2 - bulletIndent, theme.statusBarText);

        int widgetHeight = widgetPadding * 2;
        if (widget.title.texture)
        {
            widgetHeight += widget.title.height + Scale(12);
        }
        for (const auto& line : widget.descriptionLines)
        {
            widgetHeight += line.height;
        }
        if (!widget.descriptionLines.empty())
        {
            widgetHeight += Scale(12);
        }
        for (const auto& itemLines : widget.itemLines)
        {
            widgetHeight += Scale(12);
            for (const auto& line : itemLines)
            {
                widgetHeight += line.height;
            }
        }

        SDL_Rect widgetRect{widgetX, widgetCursorY, widgetWidth, widgetHeight};
        widgetCursorY = widgetRect.y + widgetRect.h + widgetSpacing;

        SDL_Color widgetAccent = widget.accent.a == 0 ? theme.channelBadge : widget.accent;
        SDL_Color widgetFill = theme.libraryCard;
        SDL_Color widgetOutline = theme.border;
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, widgetFill.r, widgetFill.g, widgetFill.b, 235);
        colony::drawing::RenderFilledRoundedRect(renderer, widgetRect, Scale(22));
        SDL_SetRenderDrawColor(renderer, widgetOutline.r, widgetOutline.g, widgetOutline.b, 170);
        colony::drawing::RenderRoundedRect(renderer, widgetRect, Scale(22));
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_Rect widgetAccentBar{widgetRect.x + widgetPadding, widgetRect.y + widgetPadding - Scale(10), widgetRect.w - widgetPadding * 2, Scale(4)};
        SDL_SetRenderDrawColor(renderer, widgetAccent.r, widgetAccent.g, widgetAccent.b, SDL_ALPHA_OPAQUE);
        colony::drawing::RenderFilledRoundedRect(renderer, widgetAccentBar, Scale(2));

        int widgetTextX = widgetRect.x + widgetPadding;
        int widgetCursor = widgetRect.y + widgetPadding;
        if (widget.title.texture)
        {
            SDL_Rect titleRect{widgetTextX, widgetCursor, widget.title.width, widget.title.height};
            colony::RenderTexture(renderer, widget.title, titleRect);
            widgetCursor += widget.title.height + Scale(12);
        }

        if (!widget.descriptionLines.empty())
        {
            for (const auto& line : widget.descriptionLines)
            {
                SDL_Rect lineRect{widgetTextX, widgetCursor, line.width, line.height};
                colony::RenderTexture(renderer, line, lineRect);
                widgetCursor += line.height;
            }
            widgetCursor += Scale(12);
        }

        if (!widget.itemLines.empty())
        {
            for (const auto& itemLines : widget.itemLines)
            {
                int bulletCenterY = widgetCursor;
                if (!itemLines.empty())
                {
                    bulletCenterY += itemLines.front().height / 2;
                }
                SDL_Rect bulletRect{widgetTextX, bulletCenterY - bulletSize / 2, bulletSize, bulletSize};
                SDL_SetRenderDrawColor(renderer, widgetAccent.r, widgetAccent.g, widgetAccent.b, SDL_ALPHA_OPAQUE);
                colony::drawing::RenderFilledRoundedRect(renderer, bulletRect, bulletRect.w / 2);

                int itemCursorY = widgetCursor;
                for (std::size_t lineIndex = 0; lineIndex < itemLines.size(); ++lineIndex)
                {
                    const auto& line = itemLines[lineIndex];
                    SDL_Rect lineRect{widgetTextX + bulletIndent, itemCursorY, line.width, line.height};
                    colony::RenderTexture(renderer, line, lineRect);
                    itemCursorY += line.height;
                    if (lineIndex + 1 < itemLines.size())
                    {
                        itemCursorY += itemLineSkip > 0 ? std::max(0, itemLineSkip - line.height) : Scale(4);
                    }
                }
                widgetCursor = itemCursorY + Scale(12);
            }
        }
    };

    if (sideColumnIndex >= 0)
    {
        const int sideX = columnPositions[static_cast<std::size_t>(sideColumnIndex)];
        const int sideWidth = tileWidth;
        int cursorY = sideColumnCursor;

        if (detailBranchIndex >= 0 && detailBranchIndex < static_cast<int>(branches_.size()))
        {
            BranchChrome& detailBranch = branches_[static_cast<std::size_t>(detailBranchIndex)];
            const int detailPadding = Scale(28);
            const int detailTextWidth = std::max(0, sideWidth - detailPadding * 2);
            RebuildBranchDetailDescription(renderer, detailBranch, detailTextWidth, theme.heroBody);

            int detailHeight = detailPadding * 2;
            if (detailBranch.title.texture)
            {
                detailHeight += detailBranch.title.height + Scale(12);
            }
            if (detailBranch.channelLabel.texture)
            {
                detailHeight += detailBranch.channelLabel.height + Scale(6);
            }
            if (detailBranch.programLabel.texture)
            {
                detailHeight += detailBranch.programLabel.height + Scale(6);
            }
            for (const auto& line : detailBranch.detailBodyLines)
            {
                detailHeight += line.height;
            }
            if (!detailBranch.detailBodyLines.empty())
            {
                detailHeight += Scale(16);
            }
            for (const auto& bullet : detailBranch.detailBulletLines)
            {
                detailHeight += Scale(12);
                for (const auto& line : bullet)
                {
                    detailHeight += line.height;
                }
            }
            if (detailBranch.metricsLabel.texture)
            {
                detailHeight += Scale(18) + detailBranch.metricsLabel.height;
            }
            const int detailButtonHeight = Scale(52);
            detailHeight += detailButtonHeight + Scale(12);

            SDL_Rect detailRect{sideX, cursorY, sideWidth, detailHeight};
            result.detailPanelRect = detailRect;
            cursorY = detailRect.y + detailRect.h + Scale(32);

            SDL_Color detailAccent = resolveAccent(detailBranch);
            SDL_Color detailFill = theme.libraryCard;
            SDL_Color detailOutline = theme.border;
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, detailFill.r, detailFill.g, detailFill.b, 240);
            colony::drawing::RenderFilledRoundedRect(renderer, detailRect, Scale(24));
            SDL_SetRenderDrawColor(renderer, detailOutline.r, detailOutline.g, detailOutline.b, 180);
            colony::drawing::RenderRoundedRect(renderer, detailRect, Scale(24));
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            int detailTextX = detailRect.x + detailPadding;
            int detailCursorY = detailRect.y + detailPadding;

            if (detailBranch.title.texture)
            {
                SDL_Rect titleRect{detailTextX, detailCursorY, detailBranch.title.width, detailBranch.title.height};
                colony::RenderTexture(renderer, detailBranch.title, titleRect);
                detailCursorY += detailBranch.title.height + Scale(12);
            }

            if (detailBranch.channelLabel.texture)
            {
                SDL_Rect channelRect{detailTextX, detailCursorY, detailBranch.channelLabel.width, detailBranch.channelLabel.height};
                colony::RenderTexture(renderer, detailBranch.channelLabel, channelRect);
                detailCursorY += detailBranch.channelLabel.height + Scale(6);
            }

            if (detailBranch.programLabel.texture)
            {
                SDL_Rect programRect{detailTextX, detailCursorY, detailBranch.programLabel.width, detailBranch.programLabel.height};
                colony::RenderTexture(renderer, detailBranch.programLabel, programRect);
                detailCursorY += detailBranch.programLabel.height + Scale(6);
            }

            if (!detailBranch.detailBodyLines.empty())
            {
                for (const auto& line : detailBranch.detailBodyLines)
                {
                    SDL_Rect bodyRect{detailTextX, detailCursorY, line.width, line.height};
                    colony::RenderTexture(renderer, line, bodyRect);
                    detailCursorY += line.height;
                }
                detailCursorY += Scale(16);
            }

            if (!detailBranch.detailBulletLines.empty())
            {
                for (const auto& bullet : detailBranch.detailBulletLines)
                {
                    detailCursorY += Scale(12);
                    int bulletCenterY = detailCursorY;
                    if (!bullet.empty())
                    {
                        bulletCenterY += bullet.front().height / 2;
                    }
                    SDL_Rect bulletRect{detailTextX, bulletCenterY - bulletSize / 2, bulletSize, bulletSize};
                    SDL_SetRenderDrawColor(renderer, detailAccent.r, detailAccent.g, detailAccent.b, SDL_ALPHA_OPAQUE);
                    colony::drawing::RenderFilledRoundedRect(renderer, bulletRect, bulletRect.w / 2);

                    int bulletTextY = detailCursorY;
                    for (const auto& line : bullet)
                    {
                        SDL_Rect lineRect{detailTextX + bulletIndent, bulletTextY, line.width, line.height};
                        colony::RenderTexture(renderer, line, lineRect);
                        bulletTextY += line.height;
                    }
                    detailCursorY = bulletTextY;
                }
            }

            if (detailBranch.metricsLabel.texture)
            {
                detailCursorY += Scale(18);
                SDL_Rect metricsRect{detailTextX, detailCursorY, detailBranch.metricsLabel.width, detailBranch.metricsLabel.height};
                colony::RenderTexture(renderer, detailBranch.metricsLabel, metricsRect);
                detailCursorY += detailBranch.metricsLabel.height + Scale(12);
            }

            SDL_Rect ctaRect{
                detailRect.x + detailPadding,
                detailRect.y + detailRect.h - detailButtonHeight - detailPadding,
                detailRect.w - detailPadding * 2,
                detailButtonHeight};
            result.detailActionRect = ctaRect;
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_Color ctaFill = colony::color::Mix(detailAccent, theme.libraryBackground, 0.3f);
            SDL_Color ctaOutline = colony::color::Mix(detailAccent, theme.border, 0.35f);
            colony::drawing::RenderFilledRoundedRect(renderer, ctaRect, Scale(20));
            SDL_SetRenderDrawColor(renderer, ctaOutline.r, ctaOutline.g, ctaOutline.b, 210);
            colony::drawing::RenderRoundedRect(renderer, ctaRect, Scale(20));
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            if (tileBodyFont_ != nullptr)
            {
                const char* ctaLabel = detailBranch.actionLabel.texture ? "Ouvrir la destination" : "Découvrir";
                colony::TextTexture ctaText =
                    colony::CreateTextTexture(renderer, tileBodyFont_, ctaLabel, theme.heroTitle);
                SDL_Rect ctaTextRect{
                    ctaRect.x + ctaRect.w / 2 - ctaText.width / 2,
                    ctaRect.y + ctaRect.h / 2 - ctaText.height / 2,
                    ctaText.width,
                    ctaText.height};
                colony::RenderTexture(renderer, ctaText, ctaTextRect);
            }
        }

        if (widgetPageCount > 0)
        {
            const int firstWidgetIndex = widgetPage * widgetsPerPage;
            const int lastWidgetIndex = std::min(firstWidgetIndex + widgetsPerPage, widgetCount);
            int widgetCursor = cursorY;
            for (int i = firstWidgetIndex; i < lastWidgetIndex; ++i)
            {
                renderWidget(widgets_[static_cast<std::size_t>(i)], sideX, sideWidth, widgetCursor);
            }
            cursorY = widgetCursor;

            const int pagerHeight = Scale(40);
            SDL_Rect pagerRect{sideX, cursorY, sideWidth, pagerHeight};
            cursorY += pagerHeight + Scale(16);

            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, theme.libraryCard.r, theme.libraryCard.g, theme.libraryCard.b, 210);
            colony::drawing::RenderFilledRoundedRect(renderer, pagerRect, Scale(18));
            SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, 160);
            colony::drawing::RenderRoundedRect(renderer, pagerRect, Scale(18));
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            const int buttonSize = Scale(28);
            SDL_Rect prevRect{pagerRect.x + Scale(12), pagerRect.y + pagerRect.h / 2 - buttonSize / 2, buttonSize, buttonSize};
            SDL_Rect nextRect{pagerRect.x + pagerRect.w - buttonSize - Scale(12), pagerRect.y + pagerRect.h / 2 - buttonSize / 2, buttonSize, buttonSize};

            auto drawPagerButton = [&](const SDL_Rect& rect, bool enabled, bool forward) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(
                    renderer,
                    theme.libraryBackground.r,
                    theme.libraryBackground.g,
                    theme.libraryBackground.b,
                    enabled ? 230 : 120);
                colony::drawing::RenderFilledRoundedRect(renderer, rect, Scale(12));
                SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, 160);
                colony::drawing::RenderRoundedRect(renderer, rect, Scale(12));
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                const int arrowPad = Scale(6);
                SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, enabled ? SDL_ALPHA_OPAQUE : 140);
                if (forward)
                {
                    SDL_RenderDrawLine(renderer, rect.x + arrowPad, rect.y + rect.h / 2, rect.x + rect.w - arrowPad, rect.y + rect.h / 2);
                    SDL_RenderDrawLine(renderer, rect.x + rect.w - arrowPad, rect.y + rect.h / 2, rect.x + rect.w / 2, rect.y + arrowPad);
                    SDL_RenderDrawLine(renderer, rect.x + rect.w - arrowPad, rect.y + rect.h / 2, rect.x + rect.w / 2, rect.y + rect.h - arrowPad);
                }
                else
                {
                    SDL_RenderDrawLine(renderer, rect.x + rect.w - arrowPad, rect.y + rect.h / 2, rect.x + arrowPad, rect.y + rect.h / 2);
                    SDL_RenderDrawLine(renderer, rect.x + arrowPad, rect.y + rect.h / 2, rect.x + rect.w / 2, rect.y + arrowPad);
                    SDL_RenderDrawLine(renderer, rect.x + arrowPad, rect.y + rect.h / 2, rect.x + rect.w / 2, rect.y + rect.h - arrowPad);
                }
            };

            drawPagerButton(prevRect, widgetPage > 0, false);
            drawPagerButton(nextRect, widgetPage + 1 < widgetPageCount, true);

            result.widgetPagerHitboxes.push_back({HubRenderResult::WidgetPagerHitbox::Type::Previous, prevRect, widgetPage - 1, widgetPage > 0});
            result.widgetPagerHitboxes.push_back({HubRenderResult::WidgetPagerHitbox::Type::Next, nextRect, widgetPage + 1, widgetPage + 1 < widgetPageCount});

            const int dotCount = widgetPageCount;
            if (dotCount > 0)
            {
                const int dotSize = Scale(12);
                const int spacing = Scale(12);
                const int totalWidth = dotCount * dotSize + (dotCount - 1) * spacing;
                int dotX = pagerRect.x + pagerRect.w / 2 - totalWidth / 2;
                const int dotY = pagerRect.y + pagerRect.h / 2 - dotSize / 2;
                for (int i = 0; i < dotCount; ++i)
                {
                    SDL_Rect dotRect{dotX, dotY, dotSize, dotSize};
                    SDL_Color dotColor = i == widgetPage ? accentColor : theme.border;
                    SDL_SetRenderDrawColor(renderer, dotColor.r, dotColor.g, dotColor.b, SDL_ALPHA_OPAQUE);
                    colony::drawing::RenderFilledRoundedRect(renderer, dotRect, dotRect.w / 2);
                    result.widgetPagerHitboxes.push_back({HubRenderResult::WidgetPagerHitbox::Type::Page, dotRect, i, true});
                    dotX += dotSize + spacing;
                }
            }

            columnOffsets[static_cast<std::size_t>(sideColumnIndex)] = cursorY + clampedScrollOffset;
        }
        else
        {
            columnOffsets[static_cast<std::size_t>(sideColumnIndex)] = cursorY + clampedScrollOffset;
        }
    }
    else
    {
        int widgetCursor = branchContentBottom;
        for (auto& widget : widgets_)
        {
            renderWidget(widget, bounds.x + gridPaddingX, gridWidth, widgetCursor);
        }
        branchContentBottom = std::max(branchContentBottom, widgetCursor);
    }

    if (clipRect.h > 0)
    {
        SDL_RenderSetClipRect(renderer, nullptr);
    }

    int gridBottom = branchContentBottom;
    if (sideColumnIndex >= 0)
    {
        gridBottom = std::max(gridBottom, columnOffsets[static_cast<std::size_t>(sideColumnIndex)] - clampedScrollOffset);
    }

    result.scrollableContentHeight = std::max(0, gridBottom - scrollTop);
    result.visibleContentHeight = std::max(0, scrollViewportHeight);

    return result;
}

void HubPanelRenderer::RebuildHeroDescription(SDL_Renderer* renderer, int maxWidth, SDL_Color color) const
{
    if (heroBodyFont_ == nullptr)
    {
        hero_.descriptionLines.clear();
        hero_.descriptionWidth = 0;
        return;
    }

    if (hero_.descriptionWidth == maxWidth && !hero_.descriptionLines.empty())
    {
        return;
    }

    hero_.descriptionWidth = maxWidth;
    hero_.descriptionLines.clear();

    if (maxWidth <= 0 || hero_.description.empty())
    {
        return;
    }

    const auto wrapped = colony::WrapTextToWidth(heroBodyFont_, hero_.description, maxWidth);
    hero_.descriptionLines.reserve(wrapped.size());
    for (const auto& line : wrapped)
    {
        hero_.descriptionLines.emplace_back(colony::CreateTextTexture(renderer, heroBodyFont_, line, color));
    }
}

void HubPanelRenderer::RebuildHeroActionDescription(SDL_Renderer* renderer, int maxWidth, SDL_Color color) const
{
    if (heroBodyFont_ == nullptr)
    {
        hero_.actionDescriptionLines.clear();
        hero_.actionDescriptionWidth = 0;
        return;
    }

    if (hero_.actionDescriptionWidth == maxWidth && !hero_.actionDescriptionLines.empty())
    {
        return;
    }

    hero_.actionDescriptionWidth = maxWidth;
    hero_.actionDescriptionLines.clear();

    if (maxWidth <= 0 || hero_.primaryActionDescription.empty())
    {
        return;
    }

    const auto wrapped = colony::WrapTextToWidth(heroBodyFont_, hero_.primaryActionDescription, maxWidth);
    hero_.actionDescriptionLines.reserve(wrapped.size());
    for (const auto& line : wrapped)
    {
        hero_.actionDescriptionLines.emplace_back(colony::CreateTextTexture(renderer, heroBodyFont_, line, color));
    }
}

void HubPanelRenderer::RebuildBranchDescription(SDL_Renderer* renderer, BranchChrome& branch, int maxWidth, SDL_Color color) const
{
    if (tileBodyFont_ == nullptr)
    {
        branch.bodyLines.clear();
        branch.descriptionWidth = 0;
        return;
    }

    if (branch.descriptionWidth == maxWidth && !branch.bodyLines.empty())
    {
        return;
    }

    branch.descriptionWidth = maxWidth;
    branch.bodyLines.clear();

    if (maxWidth <= 0 || branch.description.empty())
    {
        return;
    }

    const auto wrapped = colony::WrapTextToWidth(tileBodyFont_, branch.description, maxWidth);
    branch.bodyLines.reserve(wrapped.size());
    for (const auto& line : wrapped)
    {
        branch.bodyLines.emplace_back(colony::CreateTextTexture(renderer, tileBodyFont_, line, color));
    }
}

void HubPanelRenderer::RebuildBranchDetailDescription(SDL_Renderer* renderer, BranchChrome& branch, int maxWidth, SDL_Color color)
    const
{
    if (tileBodyFont_ == nullptr)
    {
        branch.detailBodyLines.clear();
        branch.detailBulletLines.clear();
        branch.detailBodyWidth = 0;
        branch.detailBulletWidth = 0;
        return;
    }

    if (branch.detailBodyWidth != maxWidth)
    {
        branch.detailBodyWidth = maxWidth;
        branch.detailBodyLines.clear();
        if (maxWidth > 0 && !branch.description.empty())
        {
            const auto wrapped = colony::WrapTextToWidth(tileBodyFont_, branch.description, maxWidth);
            branch.detailBodyLines.reserve(wrapped.size());
            for (const auto& line : wrapped)
            {
                branch.detailBodyLines.emplace_back(colony::CreateTextTexture(renderer, tileBodyFont_, line, color));
            }
        }
    }

    const int bulletIndent = Scale(18);
    const int bulletContentWidth = std::max(0, maxWidth - bulletIndent);
    if (branch.detailBulletWidth != bulletContentWidth)
    {
        branch.detailBulletWidth = bulletContentWidth;
        branch.detailBulletLines.clear();
        branch.detailBulletLines.reserve(branch.detailBullets.size());
        for (const auto& bullet : branch.detailBullets)
        {
            std::vector<colony::TextTexture> bulletLines;
            if (!bullet.empty() && bulletContentWidth > 0)
            {
                const auto wrapped = colony::WrapTextToWidth(tileBodyFont_, bullet, bulletContentWidth);
                bulletLines.reserve(wrapped.size());
                for (const auto& line : wrapped)
                {
                    bulletLines.emplace_back(colony::CreateTextTexture(renderer, tileBodyFont_, line, color));
                }
            }
            else if (!bullet.empty())
            {
                bulletLines.emplace_back(colony::CreateTextTexture(renderer, tileBodyFont_, bullet, color));
            }
            branch.detailBulletLines.emplace_back(std::move(bulletLines));
        }
    }
}

void HubPanelRenderer::RebuildWidgetDescription(SDL_Renderer* renderer, WidgetChrome& widget, int maxWidth, SDL_Color color) const
{
    if (tileBodyFont_ == nullptr)
    {
        widget.descriptionLines.clear();
        widget.descriptionWidth = 0;
        return;
    }

    if (widget.descriptionWidth == maxWidth && !widget.descriptionLines.empty())
    {
        return;
    }

    widget.descriptionWidth = maxWidth;
    widget.descriptionLines.clear();

    if (maxWidth <= 0 || widget.description.empty())
    {
        return;
    }

    const auto wrapped = colony::WrapTextToWidth(tileBodyFont_, widget.description, maxWidth);
    widget.descriptionLines.reserve(wrapped.size());
    for (const auto& line : wrapped)
    {
        widget.descriptionLines.emplace_back(colony::CreateTextTexture(renderer, tileBodyFont_, line, color));
    }
}

void HubPanelRenderer::RebuildWidgetItems(SDL_Renderer* renderer, WidgetChrome& widget, int maxWidth, SDL_Color color) const
{
    if (tileBodyFont_ == nullptr)
    {
        widget.itemLines.clear();
        widget.itemsWidth = 0;
        return;
    }

    if (widget.itemsWidth == maxWidth && !widget.itemLines.empty())
    {
        return;
    }

    widget.itemsWidth = maxWidth;
    widget.itemLines.clear();

    if (maxWidth <= 0)
    {
        return;
    }

    widget.itemLines.reserve(widget.items.size());
    for (const auto& item : widget.items)
    {
        if (item.empty())
        {
            widget.itemLines.emplace_back();
            continue;
        }

        const auto wrapped = colony::WrapTextToWidth(tileBodyFont_, item, maxWidth);
        std::vector<colony::TextTexture> wrappedLines;
        wrappedLines.reserve(wrapped.size());
        for (const auto& line : wrapped)
        {
            wrappedLines.emplace_back(colony::CreateTextTexture(renderer, tileBodyFont_, line, color));
        }
        widget.itemLines.emplace_back(std::move(wrappedLines));
    }
}

} // namespace colony::ui

