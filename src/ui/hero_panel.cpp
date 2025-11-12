#include "ui/hero_panel.hpp"

#include "ui/layout.hpp"
#include "utils/color.hpp"
#include "utils/drawing.hpp"

#include <algorithm>
#include <cmath>

namespace colony::ui
{

void HeroPanelRenderer::Build(
    SDL_Renderer* renderer,
    TTF_Font* labelFont,
    const ThemeColors& theme,
    const std::function<std::string(std::string_view)>& localize)
{
    chrome_.capabilitiesLabel =
        colony::CreateTextTexture(renderer, labelFont, localize("hero.capabilities"), theme.muted);
    chrome_.updatesLabel = colony::CreateTextTexture(renderer, labelFont, localize("hero.patch_notes"), theme.muted);
}

HeroRenderResult HeroPanelRenderer::RenderHero(
    SDL_Renderer* renderer,
    const ThemeColors& theme,
    const SDL_Rect& heroRect,
    ProgramVisuals& visuals,
    TTF_Font* heroBodyFont,
    TTF_Font* patchTitleFont,
    TTF_Font* patchBodyFont,
    double timeSeconds,
    double deltaSeconds) const
{
    HeroRenderResult result;

    (void)deltaSeconds;

    const int heroPaddingX = Scale(46);
    const int heroPaddingY = Scale(48);
    const int heroContentX = heroRect.x + heroPaddingX;
    int heroCursorY = heroRect.y + heroPaddingY;
    const int heroContentWidth = heroRect.w - heroPaddingX * 2;
    const int heroColumnsGap = Scale(24);
    int patchPanelWidth = heroRect.w >= Scale(860) ? std::min(Scale(300), heroContentWidth / 2) : 0;
    int textColumnWidth = heroContentWidth - (patchPanelWidth > 0 ? (patchPanelWidth + heroColumnsGap) : 0);
    if (textColumnWidth < Scale(320))
    {
        patchPanelWidth = 0;
        textColumnWidth = heroContentWidth;
    }

    SDL_Rect textColumnClip{
        heroContentX,
        heroRect.y + heroPaddingY,
        textColumnWidth,
        heroRect.h - heroPaddingY * 2};
    const bool hasColumnClip = textColumnClip.w > 0 && textColumnClip.h > 0;
    if (hasColumnClip)
    {
        SDL_RenderSetClipRect(renderer, &textColumnClip);
    }

    const float highlightPulse = static_cast<float>(0.35 + 0.35 * std::sin(timeSeconds * 1.2));
    const SDL_Color highlightColor = colony::color::Mix(visuals.accent, theme.heroBody, 0.2f + highlightPulse * 0.3f);
    RebuildDescription(visuals, renderer, heroBodyFont, textColumnWidth, theme.heroBody);
    RebuildHighlights(visuals, renderer, heroBodyFont, textColumnWidth, highlightColor);
    if (patchPanelWidth > 0)
    {
        RebuildSections(
            visuals,
            renderer,
            patchTitleFont,
            patchBodyFont,
            patchPanelWidth - Scale(24),
            theme.heroTitle,
            theme.heroBody);
    }

    if (visuals.availability.texture)
    {
        const float chipPulse = static_cast<float>(0.5 + 0.5 * std::sin(timeSeconds * 2.4));
        SDL_Rect chipRect{
            heroContentX,
            heroCursorY,
            visuals.availability.width + Scale(22),
            visuals.availability.height + Scale(10)};
        SDL_Color chipColor = colony::color::Mix(visuals.accent, theme.statusBar, 0.15f + chipPulse * 0.2f);
        SDL_SetRenderDrawColor(renderer, chipColor.r, chipColor.g, chipColor.b, chipColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, chipRect, chipRect.h / 2);
        SDL_SetRenderDrawColor(renderer, visuals.accent.r, visuals.accent.g, visuals.accent.b, SDL_ALPHA_OPAQUE);
        colony::drawing::RenderRoundedRect(renderer, chipRect, chipRect.h / 2);
        SDL_Rect chipTextRect{
            chipRect.x + Scale(12),
            chipRect.y + (chipRect.h - visuals.availability.height) / 2,
            visuals.availability.width,
            visuals.availability.height};
        colony::RenderTexture(renderer, visuals.availability, chipTextRect);
        heroCursorY += chipRect.h + Scale(14);
    }

    if (visuals.heroTitle.texture)
    {
        SDL_Rect titleRect{heroContentX, heroCursorY, visuals.heroTitle.width, visuals.heroTitle.height};
        colony::RenderTexture(renderer, visuals.heroTitle, titleRect);
        heroCursorY += titleRect.h + Scale(14);
    }

    if (visuals.heroTagline.texture)
    {
        SDL_Rect taglineRect{heroContentX, heroCursorY, visuals.heroTagline.width, visuals.heroTagline.height};
        colony::RenderTexture(renderer, visuals.heroTagline, taglineRect);
        heroCursorY += taglineRect.h + Scale(18);
    }

    const int descriptionSpacing = Scale(14);
    const int baseLineSkip = heroBodyFont ? TTF_FontLineSkip(heroBodyFont) : 0;
    for (const auto& paragraphLines : visuals.descriptionLines)
    {
        for (std::size_t lineIndex = 0; lineIndex < paragraphLines.size(); ++lineIndex)
        {
            const auto& lineTexture = paragraphLines[lineIndex];
            SDL_Rect lineRect{heroContentX, heroCursorY, lineTexture.width, lineTexture.height};
            colony::RenderTexture(renderer, lineTexture, lineRect);
            heroCursorY += lineRect.h;
            if (lineIndex + 1 < paragraphLines.size())
            {
                const int spacing = baseLineSkip > 0 ? std::max(0, baseLineSkip - lineTexture.height) : Scale(4);
                heroCursorY += spacing;
            }
        }
        heroCursorY += descriptionSpacing;
    }

    if (!visuals.highlightLines.empty())
    {
        if (chrome_.capabilitiesLabel.texture)
        {
            SDL_Rect labelRect{heroContentX, heroCursorY, chrome_.capabilitiesLabel.width, chrome_.capabilitiesLabel.height};
            colony::RenderTexture(renderer, chrome_.capabilitiesLabel, labelRect);
            heroCursorY += labelRect.h + Scale(10);
        }

        const int bulletIndent = Scale(18);
        for (const auto& lines : visuals.highlightLines)
        {
            for (const auto& line : lines)
            {
                const int bulletX = heroContentX + (line.indent ? bulletIndent : 0);
                SDL_Rect lineRect{bulletX, heroCursorY, line.texture.width, line.texture.height};
                colony::RenderTexture(renderer, line.texture, lineRect);
                heroCursorY += lineRect.h + Scale(3);
            }
            heroCursorY += Scale(6);
        }
    }

    heroCursorY += Scale(12);

    const int buttonHeight = Scale(56);
    int buttonWidth = textColumnWidth > 0 ? std::min(textColumnWidth, Scale(200)) : Scale(200);
    const int iconBoxSize = std::min(Scale(20), buttonHeight - Scale(16));
    if (visuals.actionLabel.texture)
    {
        const int textRightPadding = Scale(18);
        const int textLeftPadding = Scale(18);
        const int iconLeftPadding = Scale(16);
        const int iconToTextSpacing = Scale(10);
        int labelOffset = textLeftPadding;
        if (iconBoxSize > 0)
        {
            labelOffset = iconLeftPadding + iconBoxSize + iconToTextSpacing;
        }
        const int requiredWidth = labelOffset + visuals.actionLabel.width + textRightPadding;
        if (textColumnWidth > 0)
        {
            buttonWidth = std::clamp(requiredWidth, buttonWidth, textColumnWidth);
        }
        else
        {
            buttonWidth = std::max(buttonWidth, requiredWidth);
        }
    }
    SDL_Rect buttonRect{heroContentX, heroCursorY, buttonWidth, buttonHeight};
    const float buttonPulse = static_cast<float>(0.5 + 0.5 * std::sin(timeSeconds * 3.2));
    SDL_Color buttonColor = colony::color::Mix(visuals.accent, theme.heroTitle, 0.2f + 0.25f * buttonPulse);
    SDL_Color buttonOutline = colony::color::Mix(visuals.accent, theme.heroTitle, 0.35f);

    SDL_Rect buttonShadow = buttonRect;
    buttonShadow.y += Scale(3);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, buttonOutline.r, buttonOutline.g, buttonOutline.b, 55);
    colony::drawing::RenderFilledRoundedRect(renderer, buttonShadow, 22);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    SDL_SetRenderDrawColor(renderer, buttonColor.r, buttonColor.g, buttonColor.b, buttonColor.a);
    colony::drawing::RenderFilledRoundedRect(renderer, buttonRect, 22);
    SDL_SetRenderDrawColor(renderer, buttonOutline.r, buttonOutline.g, buttonOutline.b, buttonOutline.a);
    colony::drawing::RenderRoundedRect(renderer, buttonRect, 22);

    const bool hasButtonClip = buttonRect.w > 0 && buttonRect.h > 0;
    if (hasButtonClip)
    {
        SDL_RenderSetClipRect(renderer, &buttonRect);
    }
    int buttonLabelLeft = buttonRect.x + Scale(18);
    if (iconBoxSize > 0)
    {
        SDL_Rect iconRect{
            buttonRect.x + Scale(18),
            buttonRect.y + (buttonRect.h - iconBoxSize) / 2,
            iconBoxSize,
            iconBoxSize};
        SDL_Color iconFill = colony::color::Mix(visuals.accent, buttonColor, 0.4f + buttonPulse * 0.2f);
        SDL_SetRenderDrawColor(renderer, iconFill.r, iconFill.g, iconFill.b, iconFill.a);
        colony::drawing::RenderFilledRoundedRect(renderer, iconRect, iconRect.h / 2);
        SDL_SetRenderDrawColor(renderer, buttonOutline.r, buttonOutline.g, buttonOutline.b, SDL_ALPHA_OPAQUE);
        colony::drawing::RenderRoundedRect(renderer, iconRect, iconRect.h / 2);

        SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, theme.heroTitle.a);
        SDL_Point arrowPoints[4] = {
            {iconRect.x + iconRect.w / 2 - 3, iconRect.y + iconRect.h / 4},
            {iconRect.x + iconRect.w / 2 - 3, iconRect.y + iconRect.h - iconRect.h / 4},
            {iconRect.x + iconRect.w - iconRect.w / 4, iconRect.y + iconRect.h / 2},
            {iconRect.x + iconRect.w / 2 - 3, iconRect.y + iconRect.h / 4}};
        SDL_RenderDrawLines(renderer, arrowPoints, 4);

        buttonLabelLeft = iconRect.x + iconRect.w + Scale(10);
    }
    if (visuals.actionLabel.texture)
    {
        SDL_Rect buttonTextRect{
            buttonLabelLeft,
            buttonRect.y + (buttonRect.h - visuals.actionLabel.height) / 2,
            visuals.actionLabel.width,
            visuals.actionLabel.height};
        colony::RenderTexture(renderer, visuals.actionLabel, buttonTextRect);
    }
    if (hasButtonClip)
    {
        SDL_RenderSetClipRect(renderer, hasColumnClip ? &textColumnClip : nullptr);
    }
    result.actionButtonRect = buttonRect;

    heroCursorY += buttonRect.h + Scale(16);

    int chipCursorX = heroContentX;
    const int chipSpacing = Scale(10);
    int metaIndex = 0;
    auto drawMetaChip = [&](const colony::TextTexture& texture) {
        if (!texture.texture)
        {
            return;
        }
        const float chipGlow = static_cast<float>(0.25 + 0.25 * std::sin(timeSeconds * 1.8 + metaIndex * 1.3));
        SDL_Rect chipRect{chipCursorX, heroCursorY, texture.width + Scale(22), texture.height + Scale(10)};
        SDL_Color chipBase = colony::color::Mix(theme.statusBar, visuals.accent, 0.2f + chipGlow * 0.35f);
        SDL_Color chipOutline = colony::color::Mix(visuals.accent, theme.heroTitle, 0.3f);

        SDL_Rect chipShadow = chipRect;
        chipShadow.y += Scale(2);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, chipOutline.r, chipOutline.g, chipOutline.b, 45);
        colony::drawing::RenderFilledRoundedRect(renderer, chipShadow, chipShadow.h / 2 + Scale(2));
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_SetRenderDrawColor(renderer, chipBase.r, chipBase.g, chipBase.b, chipBase.a);
        colony::drawing::RenderFilledRoundedRect(renderer, chipRect, chipRect.h / 2);
        SDL_SetRenderDrawColor(renderer, chipOutline.r, chipOutline.g, chipOutline.b, chipOutline.a);
        colony::drawing::RenderRoundedRect(renderer, chipRect, chipRect.h / 2);
        SDL_Rect textRect{
            chipRect.x + Scale(10),
            chipRect.y + (chipRect.h - texture.height) / 2,
            texture.width,
            texture.height};
        colony::RenderTexture(renderer, texture, textRect);
        chipCursorX += chipRect.w + chipSpacing;
        ++metaIndex;
    };
    drawMetaChip(visuals.version);
    drawMetaChip(visuals.installState);
    drawMetaChip(visuals.lastLaunched);

    if (hasColumnClip)
    {
        SDL_RenderSetClipRect(renderer, nullptr);
    }

    if (patchPanelWidth > 0 && !visuals.sections.empty())
    {
        SDL_Rect patchRect{
            heroRect.x + heroRect.w - heroPaddingX - patchPanelWidth,
            heroRect.y + heroPaddingY,
            patchPanelWidth,
            heroRect.h - heroPaddingY * 2};
        const float panelGlow = static_cast<float>(0.2 + 0.2 * std::sin(timeSeconds * 0.9));
        SDL_Color patchBg = colony::color::Mix(theme.statusBar, visuals.accent, 0.12f + panelGlow * 0.2f);
        SDL_SetRenderDrawColor(renderer, patchBg.r, patchBg.g, patchBg.b, patchBg.a);
        colony::drawing::RenderFilledRoundedRect(renderer, patchRect, 20);
        SDL_SetRenderDrawColor(renderer, visuals.accent.r, visuals.accent.g, visuals.accent.b, SDL_ALPHA_OPAQUE);
        colony::drawing::RenderRoundedRect(renderer, patchRect, 20);

        const int contentPadding = Scale(20);
        const int viewportHeight = std::max(0, patchRect.h - contentPadding * 2);
        int contentHeight = 0;

        if (chrome_.updatesLabel.texture)
        {
            contentHeight += chrome_.updatesLabel.height;
            contentHeight += Scale(10);
        }

        for (const auto& section : visuals.sections)
        {
            if (section.title.texture)
            {
                contentHeight += section.title.height;
                contentHeight += Scale(10);
            }

            for (const auto& optionLines : section.lines)
            {
                for (const auto& line : optionLines)
                {
                    contentHeight += line.texture.height;
                    contentHeight += Scale(3);
                }
                contentHeight += Scale(8);
            }

            contentHeight += Scale(10);
        }

        const int maxScroll = std::max(0, contentHeight - viewportHeight);
        visuals.sectionsContentHeight = contentHeight;
        visuals.sectionsViewportContentHeight = viewportHeight;
        visuals.sectionsViewport = patchRect;
        visuals.sectionsScrollOffset = std::clamp(visuals.sectionsScrollOffset, 0, maxScroll);

        const bool hasPatchClip = patchRect.w > 0 && patchRect.h > 0;
        if (hasPatchClip)
        {
            SDL_RenderSetClipRect(renderer, &patchRect);
        }

        int patchCursorX = patchRect.x + contentPadding;
        int patchCursorY = patchRect.y + contentPadding - visuals.sectionsScrollOffset;
        if (chrome_.updatesLabel.texture)
        {
            SDL_Rect labelRect{patchCursorX, patchCursorY, chrome_.updatesLabel.width, chrome_.updatesLabel.height};
            colony::RenderTexture(renderer, chrome_.updatesLabel, labelRect);
            patchCursorY += labelRect.h + Scale(10);
        }

        const int bulletIndent = Scale(16);
        for (const auto& section : visuals.sections)
        {
            if (section.title.texture)
            {
                SDL_Rect titleRect{patchCursorX, patchCursorY, section.title.width, section.title.height};
                colony::RenderTexture(renderer, section.title, titleRect);
                patchCursorY += titleRect.h + Scale(10);
            }

            for (const auto& optionLines : section.lines)
            {
                for (const auto& line : optionLines)
                {
                    SDL_Rect lineRect{
                        patchCursorX + (line.indent ? bulletIndent : 0),
                        patchCursorY,
                        line.texture.width,
                        line.texture.height};
                    colony::RenderTexture(renderer, line.texture, lineRect);
                    patchCursorY += line.texture.height + Scale(3);
                }
                patchCursorY += Scale(8);
            }

            patchCursorY += Scale(10);
        }

        if (hasPatchClip)
        {
            SDL_RenderSetClipRect(renderer, nullptr);
        }

        int mouseX = 0;
        int mouseY = 0;
        SDL_GetMouseState(&mouseX, &mouseY);
        auto pointInRect = [](const SDL_Rect& rect, int x, int y) {
            if (rect.w <= 0 || rect.h <= 0)
            {
                return false;
            }
            const int maxX = rect.x + rect.w;
            const int maxY = rect.y + rect.h;
            return x >= rect.x && x < maxX && y >= rect.y && y < maxY;
        };

        const bool hovered = pointInRect(patchRect, mouseX, mouseY);
        if (hovered && maxScroll > 0)
        {
            const int trackMargin = Scale(14);
            const int trackWidth = Scale(4);
            const int trackHeight = std::max(0, patchRect.h - trackMargin * 2);
            if (trackHeight > 0)
            {
                const int trackX = patchRect.x + patchRect.w - trackMargin - trackWidth;
                const int trackY = patchRect.y + trackMargin;

                SDL_Rect trackRect{trackX, trackY, trackWidth, trackHeight};
                SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
                SDL_RenderFillRect(renderer, &trackRect);

                const int thumbMinHeight = Scale(20);
                const int rawThumbHeight = static_cast<int>(std::round(
                    static_cast<float>(trackHeight) * viewportHeight / static_cast<float>(contentHeight)));
                const int thumbHeight = std::clamp(rawThumbHeight, thumbMinHeight, trackHeight);
                const float scrollRatio = static_cast<float>(visuals.sectionsScrollOffset) / static_cast<float>(maxScroll);
                const int thumbTravel = std::max(0, trackHeight - thumbHeight);
                const int thumbOffset = static_cast<int>(std::round(scrollRatio * thumbTravel));
                SDL_Rect thumbRect{trackX, trackY + thumbOffset, trackWidth, thumbHeight};
                SDL_Color thumbColor = colony::color::Mix(visuals.accent, theme.heroTitle, 0.25f);
                SDL_SetRenderDrawColor(renderer, thumbColor.r, thumbColor.g, thumbColor.b, SDL_ALPHA_OPAQUE);
                SDL_RenderFillRect(renderer, &thumbRect);
            }
        }
    }
    else
    {
        visuals.sectionsViewport = SDL_Rect{0, 0, 0, 0};
        visuals.sectionsViewportContentHeight = 0;
        visuals.sectionsContentHeight = 0;
        visuals.sectionsScrollOffset = 0;
    }

    return result;
}

void HeroPanelRenderer::RenderSettings(
    SDL_Renderer* renderer,
    const ThemeColors& theme,
    const SDL_Rect& heroRect,
    const SettingsPanel& panel,
    int scrollOffset,
    std::string_view activeSchemeId,
    std::string_view activeLanguageId,
    const SettingsPanel::SectionStates& sectionStates,
    const std::unordered_map<std::string, float>& customizationValues,
    const std::unordered_map<std::string, bool>& toggleStates,
    SettingsPanel::RenderResult& outResult,
    double timeSeconds) const
{
    SDL_Rect contentRect{
        heroRect.x + Scale(46),
        heroRect.y + Scale(48),
        heroRect.w - Scale(92),
        heroRect.h - Scale(96)};
    const bool hasClip = contentRect.w > 0 && contentRect.h > 0;
    if (hasClip)
    {
        SDL_Rect glowRect = contentRect;
        glowRect.x -= Scale(12);
        glowRect.y -= Scale(12);
        glowRect.w += Scale(24);
        glowRect.h += Scale(24);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
        const float glow = static_cast<float>(0.35 + 0.35 * std::sin(timeSeconds));
        SDL_Color glowColor = colony::color::Mix(theme.statusBar, theme.heroTitle, glow);
        SDL_SetRenderDrawColor(renderer, glowColor.r, glowColor.g, glowColor.b, 36);
        colony::drawing::RenderFilledRoundedRect(renderer, glowRect, 26);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
    if (hasClip)
    {
        SDL_RenderSetClipRect(renderer, &contentRect);
    }
    outResult = panel.Render(
        renderer,
        contentRect,
        scrollOffset,
        theme,
        activeSchemeId,
        activeLanguageId,
        sectionStates,
        toggleStates,
        customizationValues);
    if (hasClip)
    {
        SDL_RenderSetClipRect(renderer, nullptr);
    }
}

void HeroPanelRenderer::RenderStatusBar(
    SDL_Renderer* renderer,
    const ThemeColors& theme,
    const SDL_Rect& heroRect,
    int statusBarHeight,
    const ProgramVisuals* visuals,
    double timeSeconds) const
{
    SDL_Rect statusRect{heroRect.x, heroRect.y + heroRect.h - statusBarHeight, heroRect.w, statusBarHeight};
    SDL_SetRenderDrawColor(renderer, theme.statusBar.r, theme.statusBar.g, theme.statusBar.b, theme.statusBar.a);
    colony::drawing::RenderFilledRoundedRect(renderer, statusRect, 12);
    SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
    SDL_RenderDrawLine(renderer, statusRect.x, statusRect.y, statusRect.x + statusRect.w, statusRect.y);

    const int sweepWidth = Scale(160);
    const double sweepPhase = std::fmod(timeSeconds * 0.35, 1.0);
    SDL_Rect sweepRect{
        statusRect.x + static_cast<int>(std::round((statusRect.w + sweepWidth) * sweepPhase)) - sweepWidth,
        statusRect.y,
        sweepWidth,
        statusRect.h};
    SDL_Rect clippedSweep{};
    if (SDL_IntersectRect(&statusRect, &sweepRect, &clippedSweep) == SDL_TRUE)
    {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
        SDL_Color glow = colony::color::Mix(theme.statusBar, theme.heroTitle, 0.6f);
        SDL_SetRenderDrawColor(renderer, glow.r, glow.g, glow.b, 56);
        SDL_RenderFillRect(renderer, &clippedSweep);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    if (visuals != nullptr && visuals->statusBar.texture)
    {
        SDL_Rect textRect{
            statusRect.x + Scale(18),
            statusRect.y + (statusRect.h - visuals->statusBar.height) / 2,
            visuals->statusBar.width,
            visuals->statusBar.height};
        colony::RenderTexture(renderer, visuals->statusBar, textRect);
    }
}

} // namespace colony::ui
