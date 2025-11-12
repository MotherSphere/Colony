#include "ui/hub_panel.hpp"

#include "ui/layout.hpp"

#include "utils/color.hpp"
#include "utils/drawing.hpp"
#include "utils/text_wrapping.hpp"

#include <algorithm>
#include <cmath>

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
    hero_.eyebrow = {};
    hero_.metaLine = {};
    hero_.description = content.description;
    hero_.descriptionWidth = 0;
    hero_.descriptionLines.clear();

    if (tileBodyFont_ != nullptr)
    {
        SDL_Color eyebrowColor = colony::color::Mix(theme.heroTitle, theme.heroGradientFallbackEnd, 0.5f);
        hero_.eyebrow = colony::CreateTextTexture(renderer, tileBodyFont_, "Control Hub Overview", eyebrowColor);

        if (!content.branches.empty())
        {
            const int branchCount = static_cast<int>(content.branches.size());
            std::string metaLineText = std::to_string(branchCount) + (branchCount == 1 ? " destination ready for you" : " destinations ready for you");
            SDL_Color metaColor = colony::color::Mix(theme.heroBody, theme.heroTitle, 0.35f);
            hero_.metaLine = colony::CreateTextTexture(renderer, tileBodyFont_, metaLineText, metaColor);
        }
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
        if (tileBodyFont_ != nullptr)
        {
            SDL_Color chipTextColor = theme.background;
            branch.chipLabel = colony::CreateTextTexture(renderer, tileBodyFont_, branchContent.title, chipTextColor);
        }
        branches_.emplace_back(std::move(branch));
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
    const int heroPadding = Scale(48);
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

    SDL_Rect heroGrid{heroRect.x + heroPadding / 2, heroRect.y + heroPadding / 2, heroRect.w - heroPadding, heroRect.h - heroPadding};
    if (heroGrid.w > 0 && heroGrid.h > 0)
    {
        SDL_Color gridColor = colony::color::Mix(accentColor, gradientEnd, 0.55f);
        SDL_SetRenderDrawColor(renderer, gridColor.r, gridColor.g, gridColor.b, 38);
        for (int x = heroGrid.x; x <= heroGrid.x + heroGrid.w; x += Scale(56))
        {
            SDL_RenderDrawLine(renderer, x, heroGrid.y, x, heroGrid.y + heroGrid.h);
        }
        for (int y = heroGrid.y; y <= heroGrid.y + heroGrid.h; y += Scale(44))
        {
            SDL_RenderDrawLine(renderer, heroGrid.x, y, heroGrid.x + heroGrid.w, y);
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    const int heroContentWidth = std::max(0, heroRect.w - heroPadding * 2);
    const int heroContentX = heroRect.x + heroPadding;
    int heroCursorY = heroRect.y + heroPadding;
    const int heroTextWidth = heroRect.w >= Scale(900) ? heroContentWidth / 2 : heroContentWidth;

    RebuildHeroDescription(renderer, heroTextWidth, theme.heroBody);

    if (hero_.eyebrow.texture)
    {
        const int eyebrowPaddingX = Scale(18);
        const int eyebrowPaddingY = Scale(8);
        SDL_Rect eyebrowRect{
            heroContentX,
            heroCursorY,
            hero_.eyebrow.width + eyebrowPaddingX * 2,
            hero_.eyebrow.height + eyebrowPaddingY * 2};
        SDL_Color eyebrowFill = colony::color::Mix(accentColor, theme.heroTitle, 0.45f);
        SDL_Color eyebrowOutline = colony::color::Mix(accentColor, theme.heroTitle, 0.25f);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, eyebrowFill.r, eyebrowFill.g, eyebrowFill.b, 130);
        colony::drawing::RenderFilledRoundedRect(renderer, eyebrowRect, eyebrowRect.h / 2);
        SDL_SetRenderDrawColor(renderer, eyebrowOutline.r, eyebrowOutline.g, eyebrowOutline.b, 160);
        colony::drawing::RenderRoundedRect(renderer, eyebrowRect, eyebrowRect.h / 2);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_Rect eyebrowTextRect{
            eyebrowRect.x + eyebrowPaddingX,
            eyebrowRect.y + eyebrowPaddingY,
            hero_.eyebrow.width,
            hero_.eyebrow.height};
        colony::RenderTexture(renderer, hero_.eyebrow, eyebrowTextRect);
        heroCursorY += eyebrowRect.h + Scale(20);
    }

    if (hero_.headline.texture)
    {
        SDL_Rect accentRule{heroContentX, heroCursorY - Scale(18), Scale(64), Scale(6)};
        SDL_Color accentRuleColor = colony::color::Mix(accentColor, theme.heroTitle, 0.35f);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, accentRuleColor.r, accentRuleColor.g, accentRuleColor.b, 150);
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

    if (hero_.metaLine.texture)
    {
        heroCursorY += Scale(18);
        SDL_Rect metaRect{heroContentX, heroCursorY, hero_.metaLine.width, hero_.metaLine.height};
        colony::RenderTexture(renderer, hero_.metaLine, metaRect);
        heroCursorY += hero_.metaLine.height;
    }

    int chipCursorX = heroContentX;
    int chipCursorY = heroCursorY + Scale(22);
    int chipRowHeight = 0;
    const int chipPaddingX = Scale(18);
    const int chipPaddingY = Scale(10);
    const int chipSpacing = Scale(12);
    int chipsRendered = 0;
    const int maxChips = 4;
    for (std::size_t index = 0; index < branches_.size() && chipsRendered < maxChips; ++index)
    {
        const BranchChrome& branch = branches_[index];
        if (!branch.chipLabel.texture)
        {
            continue;
        }

        const int chipWidth = branch.chipLabel.width + chipPaddingX * 2;
        const int chipHeight = branch.chipLabel.height + chipPaddingY * 2;
        if (chipCursorX + chipWidth > heroContentX + heroTextWidth && chipCursorX > heroContentX)
        {
            chipCursorX = heroContentX;
            chipCursorY += chipRowHeight + chipSpacing;
            chipRowHeight = 0;
        }

        SDL_Rect chipRect{chipCursorX, chipCursorY, chipWidth, chipHeight};
        SDL_Color branchAccent = resolveAccent(branch);
        SDL_Color chipFill = MixWithBackground(branchAccent, theme.heroTitle, 0.35f);
        SDL_Color chipOutline = colony::color::Mix(branchAccent, theme.heroTitle, 0.45f);
        SDL_Color chipHighlight = colony::color::Mix(branchAccent, theme.heroTitle, 0.65f);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, chipFill.r, chipFill.g, chipFill.b, 200);
        colony::drawing::RenderFilledRoundedRect(renderer, chipRect, chipRect.h / 2);
        SDL_Rect chipHighlightRect{
            chipRect.x + chipRect.w / 2,
            chipRect.y,
            chipRect.w / 2,
            chipRect.h};
        SDL_SetRenderDrawColor(renderer, chipHighlight.r, chipHighlight.g, chipHighlight.b, 70);
        colony::drawing::RenderFilledRoundedRect(
            renderer,
            chipHighlightRect,
            chipRect.h / 2,
            colony::drawing::CornerTopRight | colony::drawing::CornerBottomRight);
        SDL_SetRenderDrawColor(renderer, chipOutline.r, chipOutline.g, chipOutline.b, 210);
        colony::drawing::RenderRoundedRect(renderer, chipRect, chipRect.h / 2);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_Rect chipTextRect{chipRect.x + chipPaddingX, chipRect.y + chipPaddingY, branch.chipLabel.width, branch.chipLabel.height};
        colony::RenderTexture(renderer, branch.chipLabel, chipTextRect);

        chipCursorX += chipRect.w + chipSpacing;
        chipRowHeight = std::max(chipRowHeight, chipRect.h);
        ++chipsRendered;
    }

    if (chipRowHeight > 0)
    {
        heroCursorY = chipCursorY + chipRowHeight;
    }

    heroCursorY += Scale(28);
    SDL_Rect heroBottomGlow{heroContentX, heroCursorY, heroTextWidth, Scale(6)};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    SDL_Color heroGlowColor = colony::color::Mix(accentColor, theme.heroTitle, 0.3f);
    SDL_SetRenderDrawColor(renderer, heroGlowColor.r, heroGlowColor.g, heroGlowColor.b, 70);
    SDL_RenderFillRect(renderer, &heroBottomGlow);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    const int gridTop = heroRect.y + heroRect.h + Scale(32);
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
    std::vector<int> columnOffsets(static_cast<std::size_t>(columns), gridTop);
    result.branchHitboxes.clear();
    result.branchHitboxes.reserve(branches_.size());

    for (std::size_t index = 0; index < branches_.size(); ++index)
    {
        const int columnIndex = columns > 0 ? static_cast<int>(index % static_cast<std::size_t>(columns)) : 0;
        int column = columnIndex;
        if (columns > 1)
        {
            column = 0;
            for (int c = 1; c < columns; ++c)
            {
                if (columnOffsets[static_cast<std::size_t>(c)] < columnOffsets[static_cast<std::size_t>(column)])
                {
                    column = c;
                }
            }
        }

        BranchChrome& branch = branches_[index];
        const int tilePadding = Scale(32);
        const int textWidth = std::max(0, tileWidth - tilePadding * 2);
        RebuildBranchDescription(renderer, branch, textWidth, theme.muted);

        int tileHeight = tilePadding * 2;
        if (branch.title.texture)
        {
            tileHeight += branch.title.height;
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
        tileHeight += Scale(44);
        tileHeight = std::max(tileHeight, Scale(240));

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

        SDL_Rect shadowRect = drawRect;
        shadowRect.x += Scale(4);
        shadowRect.y += Scale(8);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, 42);
        colony::drawing::RenderFilledRoundedRect(renderer, shadowRect, Scale(24));

        SDL_Color branchAccent = resolveAccent(branch);
        float emphasis = isActive ? 0.48f : isHovered ? 0.32f : 0.22f;
        SDL_Color baseFill = MixWithBackground(branchAccent, theme.libraryCard, emphasis);
        SDL_Color outline = colony::color::Mix(branchAccent, theme.heroTitle, isActive ? 0.5f : 0.28f);
        SDL_Color hoverGlow = colony::color::Mix(branchAccent, theme.heroTitle, 0.36f);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, baseFill.r, baseFill.g, baseFill.b, baseFill.a);
        colony::drawing::RenderFilledRoundedRect(renderer, drawRect, Scale(24));

        SDL_Rect headerRect = drawRect;
        headerRect.h = std::min(Scale(120), drawRect.h / 2);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        for (int layer = 0; layer < 3; ++layer)
        {
            SDL_Rect layerRect = headerRect;
            layerRect.y += layer * Scale(12);
            layerRect.h -= layer * Scale(10);
            if (layerRect.h <= 0)
            {
                continue;
            }
            SDL_Color layerColor = colony::color::Mix(branchAccent, theme.heroTitle, 0.22f + 0.16f * static_cast<float>(layer));
            const Uint8 layerAlpha = static_cast<Uint8>(96 - layer * 18);
            SDL_SetRenderDrawColor(renderer, layerColor.r, layerColor.g, layerColor.b, layerAlpha);
            colony::drawing::RenderFilledRoundedRect(
                renderer,
                layerRect,
                Scale(24),
                colony::drawing::CornerTopLeft | colony::drawing::CornerTopRight);
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_SetRenderDrawColor(renderer, outline.r, outline.g, outline.b, outline.a);
        colony::drawing::RenderRoundedRect(renderer, drawRect, Scale(24));

        SDL_Rect glowRect = drawRect;
        glowRect.h = drawRect.h / 2;
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
        const Uint8 glowAlpha = static_cast<Uint8>(isHovered ? 90 : 60);
        SDL_Color glowColor = colony::color::Mix(branchAccent, theme.heroTitle, 0.3f);
        SDL_SetRenderDrawColor(renderer, glowColor.r, glowColor.g, glowColor.b, glowAlpha);
        colony::drawing::RenderFilledRoundedRect(renderer, glowRect, Scale(24));
        if (isActive)
        {
            SDL_SetRenderDrawColor(renderer, hoverGlow.r, hoverGlow.g, hoverGlow.b, 110);
            colony::drawing::RenderRoundedRect(renderer, drawRect, Scale(26));
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_Rect badgeRect{drawRect.x + drawRect.w - Scale(44), drawRect.y + Scale(20), Scale(20), Scale(20)};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_Color badgeColor = colony::color::Mix(branchAccent, theme.heroTitle, isActive ? 0.5f : 0.35f);
        SDL_SetRenderDrawColor(renderer, badgeColor.r, badgeColor.g, badgeColor.b, 200);
        colony::drawing::RenderFilledRoundedRect(renderer, badgeRect, badgeRect.h / 2);
        SDL_Rect badgeInner{badgeRect.x + Scale(6), badgeRect.y + Scale(6), std::max(0, badgeRect.w - Scale(12)), std::max(0, badgeRect.h - Scale(12))};
        if (badgeInner.w > 0 && badgeInner.h > 0)
        {
            SDL_SetRenderDrawColor(renderer, theme.background.r, theme.background.g, theme.background.b, 220);
            colony::drawing::RenderFilledRoundedRect(renderer, badgeInner, badgeInner.h / 2);
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        const int textStartX = drawRect.x + tilePadding;
        int textCursorY = drawRect.y + tilePadding + Scale(6);

        SDL_Rect tileAccentBar{textStartX, textCursorY - Scale(16), Scale(64), Scale(6)};
        SDL_Color barColor = colony::color::Mix(branchAccent, theme.heroTitle, 0.45f);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, barColor.r, barColor.g, barColor.b, 170);
        colony::drawing::RenderFilledRoundedRect(renderer, tileAccentBar, tileAccentBar.h / 2);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        if (branch.title.texture)
        {
            SDL_Rect titleRect{textStartX, textCursorY, branch.title.width, branch.title.height};
            colony::RenderTexture(renderer, branch.title, titleRect);
            textCursorY += branch.title.height + Scale(14);
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

        const int arrowSize = Scale(26);
        SDL_Rect actionRect{drawRect.x + drawRect.w - arrowSize - tilePadding, drawRect.y + drawRect.h - arrowSize - tilePadding,
                            arrowSize, arrowSize};
        SDL_Rect actionBackdrop{
            actionRect.x - Scale(10), actionRect.y - Scale(10), actionRect.w + Scale(20), actionRect.h + Scale(20)};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_Color actionBackground = MixWithBackground(branchAccent, theme.heroTitle, isHovered ? 0.4f : 0.25f);
        SDL_SetRenderDrawColor(renderer, actionBackground.r, actionBackground.g, actionBackground.b, isHovered ? 150 : 110);
        colony::drawing::RenderFilledRoundedRect(renderer, actionBackdrop, actionBackdrop.h / 2);
        SDL_SetRenderDrawColor(renderer, hoverGlow.r, hoverGlow.g, hoverGlow.b, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(renderer, actionRect.x, actionRect.y + actionRect.h / 2, actionRect.x + actionRect.w, actionRect.y + actionRect.h / 2);
        SDL_RenderDrawLine(
            renderer,
            actionRect.x + actionRect.w / 2,
            actionRect.y,
            actionRect.x + actionRect.w,
            actionRect.y + actionRect.h / 2);
        SDL_RenderDrawLine(
            renderer,
            actionRect.x + actionRect.w / 2,
            actionRect.y + actionRect.h,
            actionRect.x + actionRect.w,
            actionRect.y + actionRect.h / 2);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        result.branchHitboxes.push_back(HubRenderResult::BranchHitbox{branch.id, drawRect});
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

} // namespace colony::ui

