#include "ui/settings_panel.hpp"

#include "ui/layout.hpp"
#include "utils/color.hpp"
#include "utils/drawing.hpp"

#include <algorithm>

namespace colony::ui
{

void SettingsPanel::Build(
    SDL_Renderer* renderer,
    TTF_Font* titleFont,
    TTF_Font* bodyFont,
    SDL_Color titleColor,
    SDL_Color bodyColor,
    const ThemeManager& themeManager,
    const std::function<std::string(std::string_view)>& localize,
    const std::function<TTF_Font*(std::string_view)>& nativeFontResolver)
{
    themeOptions_.clear();
    languages_.clear();
    toggles_.clear();

    appearanceTitle_ = colony::CreateTextTexture(renderer, titleFont, localize("settings.appearance.title"), titleColor);
    appearanceSubtitle_ = colony::CreateTextTexture(renderer, bodyFont, localize("settings.appearance.subtitle"), bodyColor);

    languageTitle_ = colony::CreateTextTexture(renderer, titleFont, localize("settings.language.title"), titleColor);
    languageSubtitle_ =
        colony::CreateTextTexture(renderer, bodyFont, localize("settings.language.subtitle"), bodyColor);

    generalTitle_ = colony::CreateTextTexture(renderer, titleFont, localize("settings.general.title"), titleColor);
    const SDL_Color secondaryColor = colony::color::Mix(bodyColor, titleColor, 0.25f);
    generalSubtitle_ = colony::CreateTextTexture(
        renderer,
        bodyFont,
        localize("settings.general.subtitle"),
        secondaryColor);

    for (const auto& scheme : themeManager.Schemes())
    {
        ThemeOption option;
        option.id = scheme.id;
        option.label = colony::CreateTextTexture(renderer, bodyFont, scheme.name, bodyColor);
        option.swatch = {scheme.colors.background, scheme.colors.libraryCard, scheme.colors.heroTitle};
        themeOptions_.emplace_back(std::move(option));
    }

    const auto makeLanguage = [&](std::string_view id) {
        LanguageOption option;
        option.id = std::string{id};
        std::string prefix = "settings.language.options.";
        prefix.append(option.id);
        const std::string nameKey = prefix + ".name";
        const std::string nativeKey = prefix + ".native";
        option.name = colony::CreateTextTexture(renderer, bodyFont, localize(nameKey), titleColor);
        TTF_Font* nativeFont = bodyFont;
        if (nativeFontResolver)
        {
            if (TTF_Font* providedFont = nativeFontResolver(option.id); providedFont != nullptr)
            {
                nativeFont = providedFont;
            }
        }
        option.nativeName = colony::CreateTextTexture(renderer, nativeFont, localize(nativeKey), secondaryColor);
        languages_.emplace_back(std::move(option));
    };

    makeLanguage("en");
    makeLanguage("fr");
    makeLanguage("zh");
    makeLanguage("de");
    makeLanguage("ar");
    makeLanguage("hi");

    const auto makeToggle = [&](std::string_view id) {
        ToggleOption option;
        option.id = std::string{id};
        std::string prefix = "settings.toggles.";
        prefix.append(option.id);
        const std::string labelKey = prefix + ".label";
        const std::string descriptionKey = prefix + ".description";
        option.label = colony::CreateTextTexture(renderer, bodyFont, localize(labelKey), titleColor);
        option.description = colony::CreateTextTexture(renderer, bodyFont, localize(descriptionKey), secondaryColor);
        toggles_.emplace_back(std::move(option));
    };

    makeToggle("notifications");
    makeToggle("sound");
    makeToggle("auto_updates");
    makeToggle("reduced_motion");
}

SettingsPanel::RenderResult SettingsPanel::Render(
    SDL_Renderer* renderer,
    const SDL_Rect& bounds,
    int scrollOffset,
    const ThemeColors& theme,
    std::string_view activeSchemeId,
    std::string_view activeLanguageId,
    const std::unordered_map<std::string, bool>& toggleStates) const
{
    SettingsPanel::RenderResult result;
    result.viewport = bounds;

    const int clampedScroll = std::max(0, scrollOffset);
    const auto offsetRect = [&](SDL_Rect rect) {
        rect.y -= clampedScroll;
        return rect;
    };
    const auto addInteractiveRegion = [&](
                                             const std::string& id,
                                             SettingsPanel::RenderResult::InteractionType type,
                                             const SDL_Rect& rect) {
        SDL_Rect clippedRect{};
        if (rect.w > 0 && rect.h > 0 && SDL_IntersectRect(&rect, &bounds, &clippedRect) == SDL_TRUE)
        {
            result.interactiveRegions.push_back({id, type, clippedRect});
        }
    };

    int cursorY = bounds.y;
    const int horizontalPadding = Scale(26);
    const int availableWidth = bounds.w - horizontalPadding * 2;

    const auto drawSectionHeader = [&](const colony::TextTexture& title, const colony::TextTexture& subtitle) {
        if (title.texture)
        {
            SDL_Rect titleRect{bounds.x + horizontalPadding, cursorY, title.width, title.height};
            colony::RenderTexture(renderer, title, offsetRect(titleRect));
            cursorY += titleRect.h + Scale(8);
        }

        if (subtitle.texture)
        {
            SDL_Rect subtitleRect{bounds.x + horizontalPadding, cursorY, subtitle.width, subtitle.height};
            colony::RenderTexture(renderer, subtitle, offsetRect(subtitleRect));
            cursorY += subtitleRect.h + Scale(18);
        }
    };

    drawSectionHeader(appearanceTitle_, appearanceSubtitle_);

    const int themeCardSpacing = Scale(14);
    const int themeColumns = availableWidth >= Scale(720) ? 3 : (availableWidth >= Scale(520) ? 2 : 1);
    const int totalThemeSpacing = themeCardSpacing * std::max(0, themeColumns - 1);
    const int themeCardWidth = themeColumns == 1 ? availableWidth : (availableWidth - totalThemeSpacing) / themeColumns;
    const int themeCardHeight = Scale(152);

    for (std::size_t index = 0; index < themeOptions_.size(); ++index)
    {
        const int column = themeColumns == 1 ? 0 : static_cast<int>(index % themeColumns);
        const int row = themeColumns == 1 ? static_cast<int>(index) : static_cast<int>(index / themeColumns);
        const int cardX = bounds.x + horizontalPadding + column * (themeCardWidth + themeCardSpacing);
        const int cardY = cursorY + row * (themeCardHeight + themeCardSpacing);
        SDL_Rect logicalCardRect{cardX, cardY, themeCardWidth, themeCardHeight};
        SDL_Rect cardRect = offsetRect(logicalCardRect);

        const bool isActive = themeOptions_[index].id == activeSchemeId;
        const SDL_Color baseColor = isActive
            ? colony::color::Mix(theme.libraryCardActive, theme.heroTitle, 0.08f)
            : colony::color::Mix(theme.libraryCard, theme.background, 0.35f);
        const SDL_Color borderColor = isActive ? theme.heroTitle : colony::color::Mix(theme.border, theme.libraryCard, 0.5f);

        SDL_SetRenderDrawColor(renderer, baseColor.r, baseColor.g, baseColor.b, baseColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, cardRect, 20);
        SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
        colony::drawing::RenderRoundedRect(renderer, cardRect, 20);

        const int cardPadding = Scale(18);
        const auto& option = themeOptions_[index];

        SDL_Rect previewRect{
            logicalCardRect.x + cardPadding,
            logicalCardRect.y + cardPadding,
            logicalCardRect.w - cardPadding * 2,
            Scale(72)};
        SDL_Rect drawPreviewRect = offsetRect(previewRect);

        const SDL_Color previewBackground = option.swatch[0];
        SDL_SetRenderDrawColor(renderer, previewBackground.r, previewBackground.g, previewBackground.b, previewBackground.a);
        colony::drawing::RenderFilledRoundedRect(renderer, drawPreviewRect, 16);

        const SDL_Color previewCardColor = option.swatch[1];
        SDL_Rect heroPreviewRect{
            previewRect.x + Scale(14),
            previewRect.y + Scale(12),
            previewRect.w - Scale(28),
            Scale(26)};
        SDL_Rect drawHeroPreviewRect = offsetRect(heroPreviewRect);
        SDL_SetRenderDrawColor(renderer, previewCardColor.r, previewCardColor.g, previewCardColor.b, previewCardColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, drawHeroPreviewRect, 10);

        SDL_Rect accentCircle{
            heroPreviewRect.x + heroPreviewRect.w - Scale(36),
            heroPreviewRect.y - Scale(10),
            Scale(36),
            Scale(36)};
        SDL_Rect drawAccentCircle = offsetRect(accentCircle);
        const SDL_Color accentColor = option.swatch[2];
        SDL_SetRenderDrawColor(renderer, accentColor.r, accentColor.g, accentColor.b, accentColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, drawAccentCircle, accentCircle.w / 2);

        SDL_Rect labelRect{
            logicalCardRect.x + cardPadding,
            previewRect.y + previewRect.h + Scale(12),
            option.label.width,
            option.label.height};
        colony::RenderTexture(renderer, option.label, offsetRect(labelRect));

        int dotX = labelRect.x;
        const int dotSize = Scale(12);
        const int dotSpacing = Scale(8);
        const int dotY = labelRect.y + labelRect.h + Scale(10);
        for (const SDL_Color& swatchColor : option.swatch)
        {
            SDL_Rect dotRect{dotX, dotY, dotSize, dotSize};
            SDL_Rect drawDotRect = offsetRect(dotRect);
            SDL_SetRenderDrawColor(renderer, swatchColor.r, swatchColor.g, swatchColor.b, swatchColor.a);
            colony::drawing::RenderFilledRoundedRect(renderer, drawDotRect, dotSize / 2);
            SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
            colony::drawing::RenderRoundedRect(renderer, drawDotRect, dotSize / 2);
            dotX += dotSize + dotSpacing;
        }

        if (isActive)
        {
            const int badgeSize = Scale(30);
            SDL_Rect badgeRect{
                logicalCardRect.x + logicalCardRect.w - badgeSize - cardPadding + Scale(4),
                logicalCardRect.y + cardPadding - Scale(8),
                badgeSize,
                badgeSize};
            SDL_Rect drawBadgeRect = offsetRect(badgeRect);
            SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, theme.heroTitle.a);
            colony::drawing::RenderFilledRoundedRect(renderer, drawBadgeRect, badgeSize / 2);

            SDL_SetRenderDrawColor(renderer, theme.background.r, theme.background.g, theme.background.b, SDL_ALPHA_OPAQUE);
            const int checkStartX = drawBadgeRect.x + Scale(8);
            const int checkStartY = drawBadgeRect.y + badgeRect.h / 2;
            const int checkMidX = checkStartX + Scale(5);
            const int checkMidY = checkStartY + Scale(5);
            const int checkEndX = drawBadgeRect.x + badgeRect.w - Scale(6);
            const int checkEndY = drawBadgeRect.y + Scale(10);
            SDL_RenderDrawLine(renderer, checkStartX, checkStartY, checkMidX, checkMidY);
            SDL_RenderDrawLine(renderer, checkMidX, checkMidY, checkEndX, checkEndY);
        }

        addInteractiveRegion(option.id, RenderResult::InteractionType::ThemeSelection, cardRect);

        if (themeColumns == 1)
        {
            cursorY += themeCardHeight + themeCardSpacing;
        }
        else if (column + 1 == themeColumns)
        {
            cursorY = cardY + themeCardHeight + themeCardSpacing;
        }
    }

    cursorY += Scale(6);

    drawSectionHeader(languageTitle_, languageSubtitle_);

    const int languageCardHeight = Scale(86);
    const int languageCardSpacing = Scale(16);
    for (const auto& language : languages_)
    {
        SDL_Rect logicalCardRect{bounds.x + horizontalPadding, cursorY, availableWidth, languageCardHeight};
        SDL_Rect cardRect = offsetRect(logicalCardRect);
        const bool isActive = language.id == activeLanguageId;
        const SDL_Color baseColor = isActive
            ? colony::color::Mix(theme.libraryCardActive, theme.heroTitle, 0.08f)
            : colony::color::Mix(theme.libraryCard, theme.background, 0.4f);
        const SDL_Color borderColor = isActive ? theme.heroTitle : colony::color::Mix(theme.border, theme.libraryCard, 0.4f);

        SDL_SetRenderDrawColor(renderer, baseColor.r, baseColor.g, baseColor.b, baseColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, cardRect, 18);
        SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
        colony::drawing::RenderRoundedRect(renderer, cardRect, 18);

        const int accentWidth = Scale(6);
        SDL_Rect accentRect{logicalCardRect.x, logicalCardRect.y, accentWidth, logicalCardRect.h};
        SDL_Rect drawAccentRect = offsetRect(accentRect);
        const SDL_Color accentColor = isActive ? theme.heroTitle : colony::color::Mix(theme.border, theme.libraryCard, 0.5f);
        SDL_SetRenderDrawColor(renderer, accentColor.r, accentColor.g, accentColor.b, accentColor.a);
        SDL_RenderFillRect(renderer, &drawAccentRect);

        const int contentX = logicalCardRect.x + Scale(22);
        int contentY = logicalCardRect.y + Scale(18);
        if (language.name.texture)
        {
            SDL_Rect nameRect{contentX, contentY, language.name.width, language.name.height};
            colony::RenderTexture(renderer, language.name, offsetRect(nameRect));
            contentY += language.name.height + Scale(6);
        }

        if (language.nativeName.texture)
        {
            SDL_Rect nativeRect{contentX, contentY, language.nativeName.width, language.nativeName.height};
            colony::RenderTexture(renderer, language.nativeName, offsetRect(nativeRect));
        }

        const int radioSize = Scale(28);
        SDL_Rect radioRect{
            logicalCardRect.x + logicalCardRect.w - radioSize - Scale(24),
            logicalCardRect.y + (logicalCardRect.h - radioSize) / 2,
            radioSize,
            radioSize};
        SDL_Rect drawRadioRect = offsetRect(radioRect);
        const SDL_Color radioBorder = isActive ? theme.heroTitle : colony::color::Mix(theme.border, theme.libraryCard, 0.5f);
        SDL_SetRenderDrawColor(renderer, radioBorder.r, radioBorder.g, radioBorder.b, radioBorder.a);
        colony::drawing::RenderRoundedRect(renderer, drawRadioRect, radioSize / 2);
        if (isActive)
        {
            SDL_Rect innerRect{
                radioRect.x + Scale(6),
                radioRect.y + Scale(6),
                radioRect.w - Scale(12),
                radioRect.h - Scale(12)};
            SDL_Rect drawInnerRect = offsetRect(innerRect);
            SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, theme.heroTitle.a);
            colony::drawing::RenderFilledRoundedRect(renderer, drawInnerRect, drawInnerRect.w / 2);
        }

        addInteractiveRegion(language.id, RenderResult::InteractionType::LanguageSelection, cardRect);

        cursorY += languageCardHeight + languageCardSpacing;
    }

    cursorY += Scale(10);

    drawSectionHeader(generalTitle_, generalSubtitle_);

    const int toggleCardHeight = Scale(92);
    const int toggleCardSpacing = Scale(18);
    for (const auto& toggle : toggles_)
    {
        SDL_Rect logicalCardRect{bounds.x + horizontalPadding, cursorY, availableWidth, toggleCardHeight};
        SDL_Rect cardRect = offsetRect(logicalCardRect);
        SDL_Color toggleBase = colony::color::Mix(theme.libraryCard, theme.background, 0.4f);
        SDL_SetRenderDrawColor(renderer, toggleBase.r, toggleBase.g, toggleBase.b, toggleBase.a);
        colony::drawing::RenderFilledRoundedRect(renderer, cardRect, 20);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        colony::drawing::RenderRoundedRect(renderer, cardRect, 20);

        const int contentX = logicalCardRect.x + Scale(22);
        int contentY = logicalCardRect.y + Scale(18);
        if (toggle.label.texture)
        {
            SDL_Rect labelRect{contentX, contentY, toggle.label.width, toggle.label.height};
            colony::RenderTexture(renderer, toggle.label, offsetRect(labelRect));
            contentY += toggle.label.height + Scale(6);
        }

        if (toggle.description.texture)
        {
            SDL_Rect descriptionRect{contentX, contentY, toggle.description.width, toggle.description.height};
            colony::RenderTexture(renderer, toggle.description, offsetRect(descriptionRect));
        }

        const int switchWidth = Scale(64);
        const int switchHeight = Scale(30);
        SDL_Rect switchRect{
            logicalCardRect.x + logicalCardRect.w - switchWidth - Scale(28),
            logicalCardRect.y + (logicalCardRect.h - switchHeight) / 2,
            switchWidth,
            switchHeight};
        SDL_Rect drawSwitchRect = offsetRect(switchRect);

        const bool isEnabled = toggleStates.contains(toggle.id) ? toggleStates.at(toggle.id) : false;
        SDL_Color trackColor = isEnabled ? theme.heroTitle : colony::color::Mix(theme.muted, theme.libraryCard, 0.55f);
        SDL_SetRenderDrawColor(renderer, trackColor.r, trackColor.g, trackColor.b, trackColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, drawSwitchRect, switchHeight / 2);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        colony::drawing::RenderRoundedRect(renderer, drawSwitchRect, switchHeight / 2);

        const int handleSize = switchHeight - Scale(8);
        SDL_Rect handleRect{
            isEnabled ? switchRect.x + switchRect.w - handleSize - Scale(4) : switchRect.x + Scale(4),
            switchRect.y + Scale(4),
            handleSize,
            handleSize};
        SDL_Rect drawHandleRect = offsetRect(handleRect);
        SDL_SetRenderDrawColor(renderer, theme.background.r, theme.background.g, theme.background.b, SDL_ALPHA_OPAQUE);
        colony::drawing::RenderFilledRoundedRect(renderer, drawHandleRect, handleSize / 2);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        colony::drawing::RenderRoundedRect(renderer, drawHandleRect, handleSize / 2);

        addInteractiveRegion(toggle.id, RenderResult::InteractionType::Toggle, drawSwitchRect);

        cursorY += toggleCardHeight + toggleCardSpacing;
    }

    result.contentHeight = cursorY - bounds.y;
    return result;
}

} // namespace colony::ui
