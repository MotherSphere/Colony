#include "ui/hub_panel.hpp"

#include "ui/layout.hpp"

#include "utils/color.hpp"
#include "utils/drawing.hpp"
#include "utils/text_wrapping.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <numbers>

namespace colony::ui
{
namespace
{
SDL_Color MixWithBackground(SDL_Color foreground, SDL_Color background, float factor)
{
    return colony::color::Mix(foreground, background, std::clamp(factor, 0.0f, 1.0f));
}
} // namespace

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
        if (tileBodyFont_ != nullptr)
        {
            for (const auto& tag : branchContent.tags)
            {
                branch.tagChips.emplace_back(
                    colony::CreateTextTexture(renderer, tileBodyFont_, tag, theme.libraryCardActive));
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
    int activeBranchIndex) const
{
    HubRenderResult result;

    int heroHeight = std::max(Scale(320), bounds.h / 2);
    const int maxHeroHeight = std::max(bounds.h - Scale(80), Scale(280));
    if (maxHeroHeight > 0)
    {
        heroHeight = std::min(heroHeight, maxHeroHeight);
    }
    heroHeight = std::clamp(heroHeight, std::min(bounds.h, Scale(240)), bounds.h);
    if (heroHeight <= 0)
    {
        heroHeight = bounds.h;
    }

    SDL_Rect heroRect{bounds.x, bounds.y, bounds.w, heroHeight};
    result.heroRect = heroRect;

    SDL_SetRenderDrawColor(renderer, theme.background.r, theme.background.g, theme.background.b, theme.background.a);
    SDL_RenderFillRect(renderer, &bounds);

    SDL_Color gradientStart = theme.heroGradientFallbackStart;
    SDL_Color gradientEnd = theme.heroGradientFallbackEnd;
    SDL_SetRenderDrawColor(renderer, gradientEnd.r, gradientEnd.g, gradientEnd.b, gradientEnd.a);
    SDL_RenderFillRect(renderer, &heroRect);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int layer = 0; layer < 4; ++layer)
    {
        const float t = static_cast<float>(layer + 1) / 4.0f;
        SDL_Rect layerRect = heroRect;
        layerRect.x += Scale(8 * layer);
        layerRect.y += Scale(6 * layer);
        layerRect.w -= Scale(16 * layer);
        layerRect.h -= Scale(12 * layer);
        if (layerRect.w <= 0 || layerRect.h <= 0)
        {
            continue;
        }
        SDL_Color layerColor = colony::color::Mix(gradientStart, gradientEnd, t * 0.7f);
        const Uint8 alpha = static_cast<Uint8>(100 - layer * 18);
        SDL_SetRenderDrawColor(renderer, layerColor.r, layerColor.g, layerColor.b, alpha);
        SDL_RenderFillRect(renderer, &layerRect);
    }

    const auto resolveAccent = [&](const BranchChrome& branch) {
        if (branch.accent.a == 0)
        {
            return theme.channelBadge;
        }
        return branch.accent;
    };

    SDL_Color accentColor = branches_.empty() ? theme.channelBadge : resolveAccent(branches_.front());
    const float orbit = static_cast<float>(std::sin(timeSeconds * 0.7));
    const int accentDiameter = Scale(240);
    SDL_Rect accentDisc{heroRect.x + heroRect.w - accentDiameter - Scale(80),
                        heroRect.y + heroRect.h / 2 - accentDiameter / 2 + static_cast<int>(orbit * Scale(16)),
                        accentDiameter,
                        accentDiameter};
    SDL_Color accentFill = colony::color::Mix(accentColor, gradientEnd, 0.35f);
    SDL_SetRenderDrawColor(renderer, accentFill.r, accentFill.g, accentFill.b, 72);
    colony::drawing::RenderFilledRoundedRect(renderer, accentDisc, accentDiameter / 2);

    SDL_Rect accentDiscSmall = accentDisc;
    accentDiscSmall.x -= Scale(40);
    accentDiscSmall.y += Scale(60);
    accentDiscSmall.w = accentDiscSmall.h = accentDiameter / 2;
    SDL_Color accentFillSmall = colony::color::Mix(accentColor, theme.heroTitle, 0.2f);
    SDL_SetRenderDrawColor(renderer, accentFillSmall.r, accentFillSmall.g, accentFillSmall.b, 64);
    colony::drawing::RenderFilledRoundedRect(renderer, accentDiscSmall, accentDiscSmall.w / 2);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    const int particleCount = 6;
    for (int i = 0; i < particleCount; ++i)
    {
        const float offset = static_cast<float>(i) / static_cast<float>(particleCount);
        const float phase = static_cast<float>(std::fmod(timeSeconds * 0.35 + offset, 1.0));
        const float theta = phase * std::numbers::pi_v<float> * 2.0f;
        const int radius = accentDiameter / 2 - Scale(24);
        const int centerX = accentDisc.x + accentDiameter / 2;
        const int centerY = accentDisc.y + accentDiameter / 2;
        const int particleX = centerX + static_cast<int>(std::cos(theta) * radius);
        const int particleY = centerY + static_cast<int>(std::sin(theta) * radius);
        const int particleSize = Scale(8);
        SDL_Rect particleRect{
            particleX - particleSize / 2,
            particleY - particleSize / 2,
            particleSize,
            particleSize};
        SDL_Color particleColor = colony::color::Mix(accentColor, gradientStart, 0.4f);
        const Uint8 particleAlpha = static_cast<Uint8>(120 * (0.5f + 0.5f * std::sin(theta + timeSeconds * 1.2)));
        SDL_SetRenderDrawColor(renderer, particleColor.r, particleColor.g, particleColor.b, particleAlpha);
        colony::drawing::RenderFilledRoundedRect(renderer, particleRect, particleSize / 2);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    const int heroPadding = Scale(48);
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
        heroCursorY += headlineRect.h + Scale(20);
    }

    const int heroLineSkip = heroBodyFont_ ? TTF_FontLineSkip(heroBodyFont_) : 0;
    for (std::size_t i = 0; i < hero_.descriptionLines.size(); ++i)
    {
        const auto& lineTexture = hero_.descriptionLines[i];
        SDL_Rect lineRect{heroContentX, heroCursorY, lineTexture.width, lineTexture.height};
        colony::RenderTexture(renderer, lineTexture, lineRect);
        heroCursorY += lineRect.h;
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
        const int chipPaddingX = Scale(16);
        const int chipPaddingY = Scale(10);
        const int chipGap = Scale(12);
        const int chipMaxWidth = heroTextWidth;
        int chipCursorX = heroContentX;
        int chipCursorY = heroCursorY;
        int chipLineHeight = 0;
        for (std::size_t i = 0; i < hero_.highlightChips.size(); ++i)
        {
            const auto& chipTexture = hero_.highlightChips[i];
            const int chipWidth = chipTexture.width + chipPaddingX * 2;
            const int chipHeight = chipTexture.height + chipPaddingY * 2;
            if (chipCursorX > heroContentX && chipCursorX + chipWidth > heroContentX + chipMaxWidth)
            {
                chipCursorX = heroContentX;
                chipCursorY += chipLineHeight + chipGap;
                chipLineHeight = 0;
            }
            SDL_Rect chipRect{chipCursorX, chipCursorY, chipWidth, chipHeight};
            const float shimmer = 0.4f + 0.3f * static_cast<float>(std::sin(timeSeconds * 2.1 + static_cast<double>(i)));
            SDL_Color chipFill = MixWithBackground(accentColor, theme.heroTitle, shimmer);
            SDL_Color chipOutline = colony::color::Mix(accentColor, theme.heroTitle, 0.55f);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, chipFill.r, chipFill.g, chipFill.b, 180);
            colony::drawing::RenderFilledRoundedRect(renderer, chipRect, chipRect.h / 2);
            SDL_SetRenderDrawColor(renderer, chipOutline.r, chipOutline.g, chipOutline.b, 200);
            colony::drawing::RenderRoundedRect(renderer, chipRect, chipRect.h / 2);
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

    if (hero_.primaryActionLabel.texture)
    {
        const int buttonPaddingX = Scale(32);
        const int buttonPaddingY = Scale(16);
        const int buttonExtra = Scale(32);
        const int buttonWidth = hero_.primaryActionLabel.width + buttonPaddingX * 2 + buttonExtra;
        const int buttonHeight = hero_.primaryActionLabel.height + buttonPaddingY * 2;
        SDL_Rect buttonRect{heroContentX, heroCursorY, buttonWidth, buttonHeight};

        const float pulse = 0.5f + 0.5f * static_cast<float>(std::sin(timeSeconds * 2.4));
        SDL_Color buttonFill = MixWithBackground(accentColor, theme.heroTitle, 0.32f + 0.12f * pulse);
        SDL_Color buttonOutline = colony::color::Mix(accentColor, theme.heroTitle, 0.6f);
        SDL_Color buttonGlow = colony::color::Mix(accentColor, theme.heroTitle, 0.45f);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, buttonFill.r, buttonFill.g, buttonFill.b, 205);
        colony::drawing::RenderFilledRoundedRect(renderer, buttonRect, buttonRect.h / 2);
        SDL_SetRenderDrawColor(renderer, buttonGlow.r, buttonGlow.g, buttonGlow.b, 160);
        colony::drawing::RenderRoundedRect(renderer, buttonRect, buttonRect.h / 2);
        SDL_SetRenderDrawColor(renderer, buttonOutline.r, buttonOutline.g, buttonOutline.b, 220);
        colony::drawing::RenderRoundedRect(renderer, buttonRect, buttonRect.h / 2 + Scale(1));
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
    if (!hero_.actionDescriptionLines.empty())
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

    SDL_Rect heroBottomGlow{heroContentX, heroCursorY, heroTextWidth, Scale(6)};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    SDL_Color heroGlowColor = colony::color::Mix(accentColor, theme.heroTitle, 0.3f);
    SDL_SetRenderDrawColor(renderer, heroGlowColor.r, heroGlowColor.g, heroGlowColor.b, 70);
    SDL_RenderFillRect(renderer, &heroBottomGlow);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    const int gridTop = heroRect.y + heroRect.h + Scale(40);
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

    int baseTileWidth = columns > 0 ? (gridWidth - tileGap * (columns - 1)) / columns : gridWidth;
    if (columns == 1)
    {
        baseTileWidth = gridWidth;
    }
    int tileWidth = std::max(Scale(240), baseTileWidth);
    if (gridWidth > 0)
    {
        tileWidth = std::min(tileWidth, gridWidth);
    }
    const int widgetColumnIndex = (columns >= 3 && !widgets_.empty()) ? columns - 1 : -1;
    std::vector<int> columnOffsets(static_cast<std::size_t>(columns), gridTop);
    result.branchHitboxes.clear();
    result.branchHitboxes.reserve(branches_.size());

    for (std::size_t index = 0; index < branches_.size(); ++index)
    {
        const int columnIndex = columns > 0 ? static_cast<int>(index % static_cast<std::size_t>(columns)) : 0;
        int column = columnIndex;
        if (columns > 1)
        {
            bool assigned = false;
            for (int c = 0; c < columns; ++c)
            {
                if (c == widgetColumnIndex)
                {
                    continue;
                }
                if (!assigned || columnOffsets[static_cast<std::size_t>(c)]
                        < columnOffsets[static_cast<std::size_t>(column)])
                {
                    column = c;
                    assigned = true;
                }
            }
            if (!assigned)
            {
                column = columnIndex;
                if (column == widgetColumnIndex)
                {
                    column = (column + 1) % columns;
                }
            }
        }

        BranchChrome& branch = branches_[index];
        const int tilePadding = Scale(28);
        const int iconSize = Scale(56);
        const int iconSpacing = Scale(18);
        const int textWidth = std::max(0, tileWidth - tilePadding * 2 - iconSize - iconSpacing);
        RebuildBranchDescription(renderer, branch, textWidth, theme.muted);

        int tileHeight = tilePadding * 2;
        if (branch.title.texture)
        {
            tileHeight += branch.title.height;
        }
        const int tagPaddingX = Scale(12);
        const int tagPaddingY = Scale(6);
        const int tagGap = Scale(8);
        if (!branch.tagChips.empty())
        {
            if (branch.title.texture)
            {
                tileHeight += Scale(10);
            }
            int chipCursorX = 0;
            int chipLineHeight = 0;
            int tagBlockHeight = 0;
            for (const auto& chip : branch.tagChips)
            {
                const int chipWidth = chip.width + tagPaddingX * 2;
                const int chipHeight = chip.height + tagPaddingY * 2;
                if (chipCursorX > 0 && chipCursorX + chipWidth > textWidth)
                {
                    tagBlockHeight += chipLineHeight + tagGap;
                    chipCursorX = 0;
                    chipLineHeight = 0;
                }
                chipCursorX += chipWidth + (chipCursorX > 0 ? tagGap : 0);
                chipLineHeight = std::max(chipLineHeight, chipHeight);
            }
            tagBlockHeight += chipLineHeight;
            tileHeight += tagBlockHeight;
        }
        if (!branch.bodyLines.empty())
        {
            tileHeight += Scale(14);
            const int lineSkip = tileBodyFont_ ? TTF_FontLineSkip(tileBodyFont_) : 0;
            for (std::size_t lineIndex = 0; lineIndex < branch.bodyLines.size(); ++lineIndex)
            {
                tileHeight += branch.bodyLines[lineIndex].height;
                if (lineIndex + 1 < branch.bodyLines.size())
                {
                    if (lineSkip > 0)
                    {
                        tileHeight += std::max(0, lineSkip - branch.bodyLines[lineIndex].height);
                    }
                    else
                    {
                        tileHeight += Scale(6);
                    }
                }
            }
        }
        if (branch.metricsLabel.texture)
        {
            tileHeight += Scale(18);
            tileHeight += branch.metricsLabel.height;
        }
        const int actionButtonHeight = branch.actionLabel.texture ? branch.actionLabel.height + Scale(16) * 2 : Scale(42);
        tileHeight += actionButtonHeight + Scale(24);
        tileHeight = std::max(tileHeight, tilePadding * 2 + iconSize);
        tileHeight = std::max(tileHeight, Scale(220));

        const int tileX = bounds.x + gridPaddingX + column * (tileWidth + tileGap);
        const int tileY = columnOffsets[static_cast<std::size_t>(column)];
        SDL_Rect tileRect{tileX, tileY, tileWidth, tileHeight};
        columnOffsets[static_cast<std::size_t>(column)] = tileRect.y + tileRect.h + tileGap;

        const bool isHovered = hoveredBranchIndex == static_cast<int>(index);
        const bool isActive = activeBranchIndex == static_cast<int>(index);
        const float bobWave = static_cast<float>(std::sin(timeSeconds * 1.4 + static_cast<double>(index)));
        int animatedYOffset = static_cast<int>(std::round(bobWave * Scale(3)));
        if (isHovered)
        {
            animatedYOffset -= Scale(4);
        }
        SDL_Rect drawRect = tileRect;
        drawRect.y += animatedYOffset;
        const int hoverExpansion = isHovered ? Scale(6) : 0;
        const int activeExpansion = isActive ? Scale(4) : 0;
        const int expansion = std::max(hoverExpansion, activeExpansion);
        drawRect.x -= expansion;
        drawRect.y -= expansion;
        drawRect.w += expansion * 2;
        drawRect.h += expansion * 2;

        SDL_Rect shadowRect = drawRect;
        shadowRect.x += Scale(4);
        shadowRect.y += Scale(8);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, 42);
        colony::drawing::RenderFilledRoundedRect(renderer, shadowRect, Scale(26));

        SDL_Color branchAccent = resolveAccent(branch);
        float emphasis = isActive ? 0.5f : isHovered ? 0.34f : 0.24f;
        SDL_Color baseFill = MixWithBackground(branchAccent, theme.libraryCard, emphasis);
        SDL_Color outline = colony::color::Mix(branchAccent, theme.heroTitle, isActive ? 0.55f : 0.32f);
        SDL_Color hoverGlow = colony::color::Mix(branchAccent, theme.heroTitle, 0.4f);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, baseFill.r, baseFill.g, baseFill.b, baseFill.a);
        colony::drawing::RenderFilledRoundedRect(renderer, drawRect, Scale(26));
        SDL_SetRenderDrawColor(renderer, outline.r, outline.g, outline.b, outline.a);
        colony::drawing::RenderRoundedRect(renderer, drawRect, Scale(26));

        SDL_Rect glowRect = drawRect;
        glowRect.h = drawRect.h / 2;
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
        const Uint8 glowAlpha = static_cast<Uint8>(isHovered ? 110 : 70);
        SDL_Color glowColor = colony::color::Mix(branchAccent, theme.heroTitle, 0.35f);
        SDL_SetRenderDrawColor(renderer, glowColor.r, glowColor.g, glowColor.b, glowAlpha);
        colony::drawing::RenderFilledRoundedRect(renderer, glowRect, Scale(26));
        if (isActive)
        {
            SDL_SetRenderDrawColor(renderer, hoverGlow.r, hoverGlow.g, hoverGlow.b, 130);
            colony::drawing::RenderRoundedRect(renderer, drawRect, Scale(28));
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        const int iconX = drawRect.x + tilePadding;
        const int iconY = drawRect.y + tilePadding;
        SDL_Rect iconRect{iconX, iconY, iconSize, iconSize};
        SDL_Color iconFill = MixWithBackground(branchAccent, theme.heroTitle, 0.28f);
        SDL_Color iconOutline = colony::color::Mix(branchAccent, theme.heroTitle, 0.52f);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        colony::drawing::RenderFilledRoundedRect(renderer, iconRect, iconRect.w / 2);
        SDL_SetRenderDrawColor(renderer, iconOutline.r, iconOutline.g, iconOutline.b, 220);
        colony::drawing::RenderRoundedRect(renderer, iconRect, iconRect.w / 2);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        if (branch.iconGlyph.texture)
        {
            SDL_Rect glyphRect{iconRect.x + iconRect.w / 2 - branch.iconGlyph.width / 2,
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
                SDL_Color chipFill = MixWithBackground(branchAccent, theme.heroTitle, 0.22f);
                SDL_Color chipOutline = colony::color::Mix(branchAccent, theme.heroTitle, 0.38f);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, chipFill.r, chipFill.g, chipFill.b, 200);
                colony::drawing::RenderFilledRoundedRect(renderer, chipRect, chipRect.h / 2);
                SDL_SetRenderDrawColor(renderer, chipOutline.r, chipOutline.g, chipOutline.b, 210);
                colony::drawing::RenderRoundedRect(renderer, chipRect, chipRect.h / 2);
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
        const int buttonHeight = branch.actionLabel.texture ? branch.actionLabel.height + buttonPaddingY * 2 : Scale(42);
        const int buttonWidth = branch.actionLabel.texture
            ? branch.actionLabel.width + buttonPaddingX * 2 + buttonIconSize + Scale(12)
            : Scale(180);
        SDL_Rect actionRect{
            drawRect.x + drawRect.w - buttonWidth - tilePadding,
            drawRect.y + drawRect.h - buttonHeight - tilePadding,
            buttonWidth,
            buttonHeight};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_Color buttonFill = MixWithBackground(branchAccent, theme.heroTitle, 0.32f);
        SDL_Color buttonOutline = colony::color::Mix(branchAccent, theme.heroTitle, 0.5f);
        colony::drawing::RenderFilledRoundedRect(renderer, actionRect, actionRect.h / 2);
        SDL_SetRenderDrawColor(renderer, buttonOutline.r, buttonOutline.g, buttonOutline.b, 220);
        colony::drawing::RenderRoundedRect(renderer, actionRect, actionRect.h / 2);
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
        SDL_RenderDrawLine(
            renderer,
            buttonArrowRect.x,
            buttonArrowRect.y + buttonArrowRect.h / 2,
            buttonArrowRect.x + buttonArrowRect.w,
            buttonArrowRect.y + buttonArrowRect.h / 2);
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

        if (isHovered)
        {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
            SDL_Color haloColor = colony::color::Mix(branchAccent, theme.heroTitle, 0.42f);
            SDL_SetRenderDrawColor(renderer, haloColor.r, haloColor.g, haloColor.b, 120);
            colony::drawing::RenderRoundedRect(renderer, drawRect, Scale(30));
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }

        result.branchHitboxes.push_back(HubRenderResult::BranchHitbox{branch.id, drawRect});
    }

    int gridBottom = gridTop;
    for (int offset : columnOffsets)
    {
        gridBottom = std::max(gridBottom, offset);
    }

    if (!widgets_.empty())
    {
        const int widgetPadding = Scale(26);
        const int bulletIndent = Scale(22);
        const int bulletSize = Scale(8);
        const int itemLineSkip = tileBodyFont_ ? TTF_FontLineSkip(tileBodyFont_) : 0;

        auto renderWidget = [&](WidgetChrome& widget, int widgetX, int widgetWidth, int& cursorY) {
            const int widgetTextWidth = std::max(0, widgetWidth - widgetPadding * 2);
            RebuildWidgetDescription(renderer, widget, widgetTextWidth, theme.muted);
            RebuildWidgetItems(renderer, widget, widgetTextWidth - bulletIndent, theme.statusBarText);

            int widgetHeight = widgetPadding * 2;
            if (widget.title.texture)
            {
                widgetHeight += widget.title.height;
            }
            if (!widget.descriptionLines.empty())
            {
                widgetHeight += Scale(12);
                for (const auto& line : widget.descriptionLines)
                {
                    widgetHeight += line.height;
                }
            }
            if (!widget.itemLines.empty())
            {
                widgetHeight += Scale(18);
                for (const auto& itemLines : widget.itemLines)
                {
                    int blockHeight = 0;
                    for (std::size_t i = 0; i < itemLines.size(); ++i)
                    {
                        blockHeight += itemLines[i].height;
                        if (i + 1 < itemLines.size())
                        {
                            blockHeight += itemLineSkip > 0 ? std::max(0, itemLineSkip - itemLines[i].height) : Scale(4);
                        }
                    }
                    widgetHeight += blockHeight + Scale(12);
                }
            }
            widgetHeight = std::max(widgetHeight, Scale(220));

            SDL_Rect widgetRect{widgetX, cursorY, widgetWidth, widgetHeight};
            cursorY += widgetHeight + tileGap;

            SDL_Color widgetAccent = widget.accent.a == 0 ? theme.channelBadge : widget.accent;
            SDL_Color widgetFill = MixWithBackground(widgetAccent, theme.libraryCard, 0.24f);
            SDL_Color widgetOutline = colony::color::Mix(widgetAccent, theme.heroTitle, 0.42f);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            colony::drawing::RenderFilledRoundedRect(renderer, widgetRect, Scale(24));
            SDL_SetRenderDrawColor(renderer, widgetOutline.r, widgetOutline.g, widgetOutline.b, 210);
            colony::drawing::RenderRoundedRect(renderer, widgetRect, Scale(24));
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            const int widgetTextX = widgetRect.x + widgetPadding;
            int widgetCursorY = widgetRect.y + widgetPadding;

            SDL_Rect widgetAccentBar{widgetTextX, widgetCursorY - Scale(12), Scale(40), Scale(5)};
            SDL_Color widgetBar = colony::color::Mix(widgetAccent, theme.heroTitle, 0.4f);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, widgetBar.r, widgetBar.g, widgetBar.b, 150);
            colony::drawing::RenderFilledRoundedRect(renderer, widgetAccentBar, widgetAccentBar.h / 2);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            if (widget.title.texture)
            {
                SDL_Rect titleRect{widgetTextX, widgetCursorY, widget.title.width, widget.title.height};
                colony::RenderTexture(renderer, widget.title, titleRect);
                widgetCursorY += widget.title.height;
            }

            if (!widget.descriptionLines.empty())
            {
                widgetCursorY += Scale(12);
                for (const auto& line : widget.descriptionLines)
                {
                    SDL_Rect lineRect{widgetTextX, widgetCursorY, line.width, line.height};
                    colony::RenderTexture(renderer, line, lineRect);
                    widgetCursorY += line.height;
                }
            }

            if (!widget.itemLines.empty())
            {
                widgetCursorY += Scale(18);
                for (const auto& itemLines : widget.itemLines)
                {
                    int bulletCenterY = widgetCursorY;
                    if (!itemLines.empty())
                    {
                        bulletCenterY += itemLines.front().height / 2;
                    }
                    SDL_Rect bulletRect{
                        widgetTextX,
                        bulletCenterY - bulletSize / 2,
                        bulletSize,
                        bulletSize};
                    SDL_SetRenderDrawColor(renderer, widgetAccent.r, widgetAccent.g, widgetAccent.b, SDL_ALPHA_OPAQUE);
                    colony::drawing::RenderFilledRoundedRect(renderer, bulletRect, bulletRect.w / 2);

                    int itemCursorY = widgetCursorY;
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
                    widgetCursorY = itemCursorY + Scale(12);
                }
            }
        };

        if (widgetColumnIndex >= 0)
        {
            int widgetX = bounds.x + gridPaddingX + widgetColumnIndex * (tileWidth + tileGap);
            int widgetWidth = tileWidth;
            int widgetY = columnOffsets[static_cast<std::size_t>(widgetColumnIndex)];
            for (auto& widget : widgets_)
            {
                renderWidget(widget, widgetX, widgetWidth, widgetY);
            }
        }
        else
        {
            int widgetWidth = gridWidth;
            int widgetX = bounds.x + gridPaddingX;
            int widgetY = gridBottom;
            for (auto& widget : widgets_)
            {
                renderWidget(widget, widgetX, widgetWidth, widgetY);
            }
        }
    }

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

