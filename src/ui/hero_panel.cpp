#include "ui/hero_panel.hpp"

#include "utils/color.hpp"
#include "utils/drawing.hpp"

#include <algorithm>
#include <cmath>

namespace colony::ui
{

void HeroPanelRenderer::Build(SDL_Renderer* renderer, TTF_Font* labelFont, const ThemeColors& theme)
{
    chrome_.capabilitiesLabel = colony::CreateTextTexture(renderer, labelFont, "CAPABILITIES", theme.muted);
    chrome_.updatesLabel = colony::CreateTextTexture(renderer, labelFont, "PATCH NOTES", theme.muted);
}

HeroRenderResult HeroPanelRenderer::RenderHero(
    SDL_Renderer* renderer,
    const ThemeColors& theme,
    const SDL_Rect& heroRect,
    ProgramVisuals& visuals,
    TTF_Font* heroBodyFont,
    TTF_Font* patchTitleFont,
    TTF_Font* patchBodyFont) const
{
    HeroRenderResult result;

    const int heroPaddingX = 56;
    const int heroPaddingY = 58;
    const int heroContentX = heroRect.x + heroPaddingX;
    int heroCursorY = heroRect.y + heroPaddingY;
    const int heroContentWidth = heroRect.w - heroPaddingX * 2;
    const int heroColumnsGap = 32;
    int patchPanelWidth = heroRect.w >= 960 ? std::min(340, heroContentWidth / 2) : 0;
    int textColumnWidth = heroContentWidth - (patchPanelWidth > 0 ? (patchPanelWidth + heroColumnsGap) : 0);
    if (textColumnWidth < 360)
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

    const SDL_Color highlightColor = colony::color::Mix(visuals.accent, theme.heroBody, 0.25f);
    RebuildDescription(visuals, renderer, heroBodyFont, textColumnWidth, theme.heroBody);
    RebuildHighlights(visuals, renderer, heroBodyFont, textColumnWidth, highlightColor);
    if (patchPanelWidth > 0)
    {
        RebuildSections(visuals, renderer, patchTitleFont, patchBodyFont, patchPanelWidth - 32, theme.heroTitle, theme.heroBody);
    }

    if (visuals.availability.texture)
    {
        SDL_Rect chipRect{
            heroContentX,
            heroCursorY,
            visuals.availability.width + 28,
            visuals.availability.height + 12};
        SDL_Color chipColor = colony::color::Mix(visuals.accent, theme.statusBar, 0.2f);
        SDL_SetRenderDrawColor(renderer, chipColor.r, chipColor.g, chipColor.b, chipColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, chipRect, chipRect.h / 2);
        SDL_SetRenderDrawColor(renderer, visuals.accent.r, visuals.accent.g, visuals.accent.b, SDL_ALPHA_OPAQUE);
        colony::drawing::RenderRoundedRect(renderer, chipRect, chipRect.h / 2);
        SDL_Rect chipTextRect{
            chipRect.x + 14,
            chipRect.y + (chipRect.h - visuals.availability.height) / 2,
            visuals.availability.width,
            visuals.availability.height};
        colony::RenderTexture(renderer, visuals.availability, chipTextRect);
        heroCursorY += chipRect.h + 18;
    }

    if (visuals.heroTitle.texture)
    {
        SDL_Rect titleRect{heroContentX, heroCursorY, visuals.heroTitle.width, visuals.heroTitle.height};
        colony::RenderTexture(renderer, visuals.heroTitle, titleRect);
        heroCursorY += titleRect.h + 18;
    }

    if (visuals.heroTagline.texture)
    {
        SDL_Rect taglineRect{heroContentX, heroCursorY, visuals.heroTagline.width, visuals.heroTagline.height};
        colony::RenderTexture(renderer, visuals.heroTagline, taglineRect);
        heroCursorY += taglineRect.h + 24;
    }

    const int descriptionSpacing = 18;
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
                const int spacing = baseLineSkip > 0 ? std::max(0, baseLineSkip - lineTexture.height) : 6;
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
            heroCursorY += labelRect.h + 12;
        }

        const int bulletIndent = 24;
        for (const auto& lines : visuals.highlightLines)
        {
            for (const auto& line : lines)
            {
                const int bulletX = heroContentX + (line.indent ? bulletIndent : 0);
                SDL_Rect lineRect{bulletX, heroCursorY, line.texture.width, line.texture.height};
                colony::RenderTexture(renderer, line.texture, lineRect);
                heroCursorY += lineRect.h + 4;
            }
            heroCursorY += 8;
        }
    }

    heroCursorY += 16;

    const int buttonWidth = textColumnWidth > 0 ? std::min(textColumnWidth, 240) : 240;
    const int buttonHeight = 64;
    SDL_Rect buttonRect{heroContentX, heroCursorY, buttonWidth, buttonHeight};
    SDL_Color buttonColor = colony::color::Mix(visuals.accent, theme.heroTitle, 0.15f);
    SDL_SetRenderDrawColor(renderer, buttonColor.r, buttonColor.g, buttonColor.b, buttonColor.a);
    colony::drawing::RenderFilledRoundedRect(renderer, buttonRect, 18);
    SDL_SetRenderDrawColor(renderer, visuals.accent.r, visuals.accent.g, visuals.accent.b, SDL_ALPHA_OPAQUE);
    colony::drawing::RenderRoundedRect(renderer, buttonRect, 18);

    const bool hasButtonClip = buttonRect.w > 0 && buttonRect.h > 0;
    if (hasButtonClip)
    {
        SDL_RenderSetClipRect(renderer, &buttonRect);
    }
    const int iconBoxSize = std::min(26, buttonHeight - 20);
    int buttonLabelLeft = buttonRect.x + 24;
    if (iconBoxSize > 0)
    {
        SDL_Rect iconRect{
            buttonRect.x + 18,
            buttonRect.y + (buttonRect.h - iconBoxSize) / 2,
            iconBoxSize,
            iconBoxSize};
        SDL_Color iconFill = colony::color::Mix(visuals.accent, theme.heroTitle, 0.35f);
        SDL_SetRenderDrawColor(renderer, iconFill.r, iconFill.g, iconFill.b, iconFill.a);
        colony::drawing::RenderFilledRoundedRect(renderer, iconRect, iconRect.h / 2);
        SDL_SetRenderDrawColor(renderer, visuals.accent.r, visuals.accent.g, visuals.accent.b, SDL_ALPHA_OPAQUE);
        colony::drawing::RenderRoundedRect(renderer, iconRect, iconRect.h / 2);

        SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, theme.heroTitle.a);
        SDL_Point arrowPoints[4] = {
            {iconRect.x + iconRect.w / 2 - 3, iconRect.y + iconRect.h / 4},
            {iconRect.x + iconRect.w / 2 - 3, iconRect.y + iconRect.h - iconRect.h / 4},
            {iconRect.x + iconRect.w - iconRect.w / 4, iconRect.y + iconRect.h / 2},
            {iconRect.x + iconRect.w / 2 - 3, iconRect.y + iconRect.h / 4}};
        SDL_RenderDrawLines(renderer, arrowPoints, 4);

        buttonLabelLeft = iconRect.x + iconRect.w + 12;
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

    heroCursorY += buttonRect.h + 22;

    int chipCursorX = heroContentX;
    const int chipSpacing = 12;
    auto drawMetaChip = [&](const colony::TextTexture& texture) {
        if (!texture.texture)
        {
            return;
        }
        SDL_Rect chipRect{chipCursorX, heroCursorY, texture.width + 26, texture.height + 12};
        SDL_SetRenderDrawColor(renderer, theme.statusBar.r, theme.statusBar.g, theme.statusBar.b, theme.statusBar.a);
        colony::drawing::RenderFilledRoundedRect(renderer, chipRect, chipRect.h / 2);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        colony::drawing::RenderRoundedRect(renderer, chipRect, chipRect.h / 2);
        SDL_Rect textRect{
            chipRect.x + 13,
            chipRect.y + (chipRect.h - texture.height) / 2,
            texture.width,
            texture.height};
        colony::RenderTexture(renderer, texture, textRect);
        chipCursorX += chipRect.w + chipSpacing;
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
        SDL_Color patchBg = colony::color::Mix(theme.statusBar, visuals.accent, 0.12f);
        SDL_SetRenderDrawColor(renderer, patchBg.r, patchBg.g, patchBg.b, patchBg.a);
        colony::drawing::RenderFilledRoundedRect(renderer, patchRect, 20);
        SDL_SetRenderDrawColor(renderer, visuals.accent.r, visuals.accent.g, visuals.accent.b, SDL_ALPHA_OPAQUE);
        colony::drawing::RenderRoundedRect(renderer, patchRect, 20);

        const int contentPadding = 24;
        const int viewportHeight = std::max(0, patchRect.h - contentPadding * 2);
        int contentHeight = 0;

        if (chrome_.updatesLabel.texture)
        {
            contentHeight += chrome_.updatesLabel.height;
            contentHeight += 12;
        }

        for (const auto& section : visuals.sections)
        {
            if (section.title.texture)
            {
                contentHeight += section.title.height;
                contentHeight += 12;
            }

            for (const auto& optionLines : section.lines)
            {
                for (const auto& line : optionLines)
                {
                    contentHeight += line.texture.height;
                    contentHeight += 4;
                }
                contentHeight += 10;
            }

            contentHeight += 12;
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
            patchCursorY += labelRect.h + 12;
        }

        const int bulletIndent = 20;
        for (const auto& section : visuals.sections)
        {
            if (section.title.texture)
            {
                SDL_Rect titleRect{patchCursorX, patchCursorY, section.title.width, section.title.height};
                colony::RenderTexture(renderer, section.title, titleRect);
                patchCursorY += titleRect.h + 12;
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
                    patchCursorY += line.texture.height + 4;
                }
                patchCursorY += 10;
            }

            patchCursorY += 12;
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
            const int trackMargin = 16;
            const int trackWidth = 6;
            const int trackHeight = std::max(0, patchRect.h - trackMargin * 2);
            if (trackHeight > 0)
            {
                const int trackX = patchRect.x + patchRect.w - trackMargin - trackWidth;
                const int trackY = patchRect.y + trackMargin;

                SDL_Rect trackRect{trackX, trackY, trackWidth, trackHeight};
                SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
                SDL_RenderFillRect(renderer, &trackRect);

                const int thumbMinHeight = 24;
                const int rawThumbHeight = static_cast<int>(std::round(
                    static_cast<float>(trackHeight) * viewportHeight / static_cast<float>(contentHeight)));
                const int thumbHeight = std::clamp(rawThumbHeight, thumbMinHeight, trackHeight);
                const float scrollRatio = static_cast<float>(visuals.sectionsScrollOffset) / static_cast<float>(maxScroll);
                const int thumbTravel = std::max(0, trackHeight - thumbHeight);
                const int thumbOffset = static_cast<int>(std::round(scrollRatio * thumbTravel));
                SDL_Rect thumbRect{trackX, trackY + thumbOffset, trackWidth, thumbHeight};
                SDL_SetRenderDrawColor(renderer, visuals.accent.r, visuals.accent.g, visuals.accent.b, SDL_ALPHA_OPAQUE);
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
    std::string_view activeSchemeId,
    std::string_view activeLanguageId,
    const std::unordered_map<std::string, bool>& toggleStates,
    SettingsPanel::RenderResult& outResult) const
{
    SDL_Rect contentRect{heroRect.x + 56, heroRect.y + 58, heroRect.w - 112, heroRect.h - 116};
    outResult = panel.Render(renderer, contentRect, theme, activeSchemeId, activeLanguageId, toggleStates);
}

void HeroPanelRenderer::RenderStatusBar(
    SDL_Renderer* renderer,
    const ThemeColors& theme,
    const SDL_Rect& heroRect,
    int statusBarHeight,
    const ProgramVisuals* visuals) const
{
    SDL_Rect statusRect{heroRect.x, heroRect.y + heroRect.h - statusBarHeight, heroRect.w, statusBarHeight};
    SDL_SetRenderDrawColor(renderer, theme.statusBar.r, theme.statusBar.g, theme.statusBar.b, theme.statusBar.a);
    colony::drawing::RenderFilledRoundedRect(renderer, statusRect, 12);
    SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
    SDL_RenderDrawLine(renderer, statusRect.x, statusRect.y, statusRect.x + statusRect.w, statusRect.y);

    if (visuals != nullptr && visuals->statusBar.texture)
    {
        SDL_Rect textRect{
            statusRect.x + 24,
            statusRect.y + (statusRect.h - visuals->statusBar.height) / 2,
            visuals->statusBar.width,
            visuals->statusBar.height};
        colony::RenderTexture(renderer, visuals->statusBar, textRect);
    }
}

} // namespace colony::ui
