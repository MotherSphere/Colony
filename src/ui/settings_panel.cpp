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

    const int themeCardSpacing = Scale(6);
    const int themeColumns = availableWidth >= Scale(720) ? 3 : (availableWidth >= Scale(480) ? 2 : 1);
    const int totalThemeSpacing = themeCardSpacing * std::max(0, themeColumns - 1);
    const int themeCardWidth = themeColumns == 1 ? availableWidth : (availableWidth - totalThemeSpacing) / themeColumns;
    const int themeCardHeight = Scale(72);

    for (std::size_t index = 0; index < themeOptions_.size(); ++index)
    {
        const int column = themeColumns == 1 ? 0 : static_cast<int>(index % themeColumns);
        const int row = themeColumns == 1 ? static_cast<int>(index) : static_cast<int>(index / themeColumns);
        const int cardX = bounds.x + horizontalPadding + column * (themeCardWidth + themeCardSpacing);
        const int cardY = cursorY + row * (themeCardHeight + themeCardSpacing);
        SDL_Rect logicalCardRect{cardX, cardY, themeCardWidth, themeCardHeight};
        SDL_Rect cardRect = offsetRect(logicalCardRect);

        const bool isActive = themeOptions_[index].id == activeSchemeId;
        const SDL_Color baseColor = isActive ? colony::color::Mix(theme.libraryCardActive, theme.heroTitle, 0.1f)
                                             : theme.libraryCard;
        const SDL_Color borderColor = isActive ? theme.heroTitle : theme.border;

        SDL_SetRenderDrawColor(renderer, baseColor.r, baseColor.g, baseColor.b, baseColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, cardRect, 18);
        SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
        colony::drawing::RenderRoundedRect(renderer, cardRect, 18);

        const auto& option = themeOptions_[index];
        SDL_Rect labelRect{
            logicalCardRect.x + Scale(12),
            logicalCardRect.y + Scale(10),
            option.label.width,
            option.label.height};
        colony::RenderTexture(renderer, option.label, offsetRect(labelRect));

        const int swatchInset = Scale(12);
        const int swatchHeight = Scale(18);
        const int swatchTrackWidth = logicalCardRect.w - 2 * swatchInset;
        const int swatchY = logicalCardRect.y + logicalCardRect.h - swatchHeight - swatchInset;
        const std::size_t swatchCount = option.swatch.size();
        if (swatchCount > 0 && swatchTrackWidth > 0)
        {
            SDL_Rect swatchTrackRect{
                logicalCardRect.x + swatchInset, swatchY, swatchTrackWidth, swatchHeight};
            const int baseSwatchWidth = swatchTrackWidth / static_cast<int>(swatchCount);
            int remainder = swatchTrackWidth % static_cast<int>(swatchCount);
            int swatchX = swatchTrackRect.x;
            for (std::size_t swatchIndex = 0; swatchIndex < swatchCount; ++swatchIndex)
            {
                const SDL_Color& swatchColor = option.swatch[swatchIndex];
                int swatchWidth = baseSwatchWidth;
                if (remainder > 0)
                {
                    ++swatchWidth;
                    --remainder;
                }
                if (swatchIndex == swatchCount - 1)
                {
                    swatchWidth = swatchTrackRect.x + swatchTrackRect.w - swatchX;
                }
                if (swatchWidth <= 0)
                {
                    continue;
                }
                SDL_Rect swatchRect{swatchX, swatchY, swatchWidth, swatchHeight};
                SDL_Rect swatchDrawRect = offsetRect(swatchRect);
                SDL_SetRenderDrawColor(renderer, swatchColor.r, swatchColor.g, swatchColor.b, swatchColor.a);
                const int radius = (swatchIndex == 0 || swatchIndex == swatchCount - 1) ? 8 : 0;
                colony::drawing::RenderFilledRoundedRect(renderer, swatchDrawRect, radius);
                swatchX += swatchWidth;
            }

            SDL_SetRenderDrawColor(
                renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
            colony::drawing::RenderRoundedRect(renderer, offsetRect(swatchTrackRect), 8);
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

    const int languageCardHeight = Scale(62);
    const int languageCardSpacing = Scale(14);
    for (const auto& language : languages_)
    {
        SDL_Rect logicalCardRect{bounds.x + horizontalPadding, cursorY, availableWidth, languageCardHeight};
        SDL_Rect cardRect = offsetRect(logicalCardRect);
        const bool isActive = language.id == activeLanguageId;
        const SDL_Color baseColor = isActive ? colony::color::Mix(theme.libraryCardActive, theme.heroTitle, 0.1f)
                                             : theme.libraryCard;
        const SDL_Color borderColor = isActive ? theme.heroTitle : theme.border;

        SDL_SetRenderDrawColor(renderer, baseColor.r, baseColor.g, baseColor.b, baseColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, cardRect, 14);
        SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
        colony::drawing::RenderRoundedRect(renderer, cardRect, 14);

        const int contentX = logicalCardRect.x + Scale(16);
        int contentY = logicalCardRect.y + Scale(14);
        if (language.name.texture)
        {
            SDL_Rect nameRect{contentX, contentY, language.name.width, language.name.height};
            colony::RenderTexture(renderer, language.name, offsetRect(nameRect));
            contentY += language.name.height + Scale(4);
        }

        if (language.nativeName.texture)
        {
            SDL_Rect nativeRect{contentX, contentY, language.nativeName.width, language.nativeName.height};
            colony::RenderTexture(renderer, language.nativeName, offsetRect(nativeRect));
        }

        const int radioSize = Scale(22);
        SDL_Rect radioRect{
            logicalCardRect.x + logicalCardRect.w - radioSize - Scale(16),
            logicalCardRect.y + (logicalCardRect.h - radioSize) / 2,
            radioSize,
            radioSize};
        SDL_Rect drawRadioRect = offsetRect(radioRect);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        colony::drawing::RenderRoundedRect(renderer, drawRadioRect, radioSize / 2);
        if (isActive)
        {
            SDL_Rect innerRect{radioRect.x + Scale(5), radioRect.y + Scale(5), radioRect.w - Scale(10), radioRect.h - Scale(10)};
            SDL_Rect drawInnerRect = offsetRect(innerRect);
            SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, theme.heroTitle.a);
            colony::drawing::RenderFilledRoundedRect(renderer, drawInnerRect, drawInnerRect.w / 2);
        }

        addInteractiveRegion(language.id, RenderResult::InteractionType::LanguageSelection, cardRect);

        cursorY += languageCardHeight + languageCardSpacing;
    }

    cursorY += Scale(10);

    drawSectionHeader(generalTitle_, generalSubtitle_);

    const int toggleCardHeight = Scale(78);
    const int toggleCardSpacing = Scale(14);
    for (const auto& toggle : toggles_)
    {
        SDL_Rect logicalCardRect{bounds.x + horizontalPadding, cursorY, availableWidth, toggleCardHeight};
        SDL_Rect cardRect = offsetRect(logicalCardRect);
        SDL_SetRenderDrawColor(renderer, theme.libraryCard.r, theme.libraryCard.g, theme.libraryCard.b, theme.libraryCard.a);
        colony::drawing::RenderFilledRoundedRect(renderer, cardRect, 14);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        colony::drawing::RenderRoundedRect(renderer, cardRect, 14);

        const int contentX = logicalCardRect.x + Scale(16);
        int contentY = logicalCardRect.y + Scale(14);
        if (toggle.label.texture)
        {
            SDL_Rect labelRect{contentX, contentY, toggle.label.width, toggle.label.height};
            colony::RenderTexture(renderer, toggle.label, offsetRect(labelRect));
            contentY += toggle.label.height + Scale(4);
        }

        if (toggle.description.texture)
        {
            SDL_Rect descriptionRect{contentX, contentY, toggle.description.width, toggle.description.height};
            colony::RenderTexture(renderer, toggle.description, offsetRect(descriptionRect));
        }

        const int switchWidth = Scale(54);
        const int switchHeight = Scale(26);
        SDL_Rect switchRect{
            logicalCardRect.x + logicalCardRect.w - switchWidth - Scale(18),
            logicalCardRect.y + (logicalCardRect.h - switchHeight) / 2,
            switchWidth,
            switchHeight};
        SDL_Rect drawSwitchRect = offsetRect(switchRect);

        const bool isEnabled = toggleStates.contains(toggle.id) ? toggleStates.at(toggle.id) : false;
        SDL_Color trackColor = isEnabled ? theme.heroTitle : theme.muted;
        SDL_SetRenderDrawColor(renderer, trackColor.r, trackColor.g, trackColor.b, trackColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, drawSwitchRect, switchHeight / 2);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        colony::drawing::RenderRoundedRect(renderer, drawSwitchRect, switchHeight / 2);

        const int handleSize = switchHeight - Scale(6);
        SDL_Rect handleRect{
            isEnabled ? switchRect.x + switchRect.w - handleSize - Scale(3) : switchRect.x + Scale(3),
            switchRect.y + Scale(3),
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
