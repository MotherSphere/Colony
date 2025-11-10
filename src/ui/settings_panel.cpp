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
    appearanceCustomizations_.clear();

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
    customizationHint_ =
        colony::CreateTextTexture(renderer, bodyFont, localize("settings.appearance.customization.hint"), secondaryColor);

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

    const auto makeCustomization = [&](std::string_view id) {
        CustomizationOption option;
        option.id = std::string{id};
        std::string prefix = "settings.appearance.customization.";
        prefix.append(option.id);
        const std::string labelKey = prefix + ".label";
        const std::string descriptionKey = prefix + ".description";
        option.label = colony::CreateTextTexture(renderer, bodyFont, localize(labelKey), titleColor);
        option.description = colony::CreateTextTexture(renderer, bodyFont, localize(descriptionKey), secondaryColor);
        appearanceCustomizations_.emplace_back(std::move(option));
    };

    makeCustomization("accent_intensity");
    makeCustomization("background_depth");
    makeCustomization("interface_density");
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

    const auto activeThemeIt = std::find_if(
        themeOptions_.begin(),
        themeOptions_.end(),
        [&](const ThemeOption& option) { return option.id == activeSchemeId; });

    const int dropdownHeight = Scale(74);
    SDL_Rect dropdownRect{bounds.x + horizontalPadding, cursorY, availableWidth, dropdownHeight};
    SDL_Rect drawDropdownRect = offsetRect(dropdownRect);
    SDL_Color dropdownBase = colony::color::Mix(theme.libraryCardActive, theme.background, 0.12f);
    SDL_Color dropdownBorder = colony::color::Mix(theme.border, theme.libraryCardActive, 0.45f);
    SDL_SetRenderDrawColor(renderer, dropdownBase.r, dropdownBase.g, dropdownBase.b, dropdownBase.a);
    colony::drawing::RenderFilledRoundedRect(renderer, drawDropdownRect, 20);
    SDL_SetRenderDrawColor(renderer, dropdownBorder.r, dropdownBorder.g, dropdownBorder.b, dropdownBorder.a);
    colony::drawing::RenderRoundedRect(renderer, drawDropdownRect, 20);

    const int dropdownPadding = Scale(22);
    int dropdownContentX = dropdownRect.x + dropdownPadding;
    const int dropdownContentY = dropdownRect.y + Scale(20);
    if (activeThemeIt != themeOptions_.end() && activeThemeIt->label.texture)
    {
        SDL_Rect activeLabelRect{dropdownContentX, dropdownContentY, activeThemeIt->label.width, activeThemeIt->label.height};
        colony::RenderTexture(renderer, activeThemeIt->label, offsetRect(activeLabelRect));
        dropdownContentX += activeLabelRect.w + Scale(16);
    }

    if (activeThemeIt != themeOptions_.end())
    {
        const int swatchWidth = Scale(24);
        const int swatchSpacing = Scale(6);
        int swatchX = dropdownRect.x + dropdownRect.w - dropdownPadding - (swatchWidth * 3 + swatchSpacing * 2);
        const int swatchY = dropdownRect.y + (dropdownHeight - swatchWidth) / 2;
        for (const SDL_Color& color : activeThemeIt->swatch)
        {
            SDL_Rect swatchRect{swatchX, swatchY, swatchWidth, swatchWidth};
            SDL_Rect drawSwatchRect = offsetRect(swatchRect);
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            colony::drawing::RenderFilledRoundedRect(renderer, drawSwatchRect, swatchWidth / 2);
            SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
            colony::drawing::RenderRoundedRect(renderer, drawSwatchRect, swatchWidth / 2);
            swatchX += swatchWidth + swatchSpacing;
        }
    }

    const int chevronSize = Scale(12);
    SDL_Point chevronCenter{dropdownRect.x + dropdownRect.w - dropdownPadding, dropdownRect.y + dropdownHeight / 2};
    SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, theme.heroTitle.a);
    SDL_RenderDrawLine(renderer, chevronCenter.x - chevronSize, chevronCenter.y - chevronSize / 2, chevronCenter.x, chevronCenter.y + chevronSize / 2);
    SDL_RenderDrawLine(renderer, chevronCenter.x, chevronCenter.y + chevronSize / 2, chevronCenter.x + chevronSize, chevronCenter.y - chevronSize / 2);

    cursorY += dropdownHeight + Scale(14);

    const int listPadding = Scale(18);
    const int themeRowHeight = Scale(82);
    const int themeRowSpacing = Scale(10);
    const int rowContainerHeight = themeOptions_.empty()
        ? 0
        : static_cast<int>(themeOptions_.size()) * themeRowHeight + std::max<int>(0, (static_cast<int>(themeOptions_.size()) - 1)) * themeRowSpacing +
            listPadding * 2;

    SDL_Rect menuRect{bounds.x + horizontalPadding, cursorY, availableWidth, rowContainerHeight};
    if (rowContainerHeight > 0)
    {
        SDL_Rect drawMenuRect = offsetRect(menuRect);
        SDL_Color menuBase = colony::color::Mix(theme.libraryCard, theme.background, 0.3f);
        SDL_SetRenderDrawColor(renderer, menuBase.r, menuBase.g, menuBase.b, menuBase.a);
        colony::drawing::RenderFilledRoundedRect(renderer, drawMenuRect, 18);
        SDL_SetRenderDrawColor(renderer, dropdownBorder.r, dropdownBorder.g, dropdownBorder.b, dropdownBorder.a);
        colony::drawing::RenderRoundedRect(renderer, drawMenuRect, 18);
    }

    int optionY = menuRect.y + listPadding;
    for (std::size_t index = 0; index < themeOptions_.size(); ++index)
    {
        ThemeOption& option = themeOptions_[index];
        SDL_Rect optionRect{menuRect.x + listPadding, optionY, menuRect.w - listPadding * 2, themeRowHeight};
        option.rect = optionRect;
        SDL_Rect drawOptionRect = offsetRect(optionRect);
        const bool isActive = option.id == activeSchemeId;
        SDL_Color optionBase = isActive ? colony::color::Mix(theme.libraryCardActive, theme.heroTitle, 0.12f)
                                        : colony::color::Mix(theme.libraryCard, theme.background, 0.4f);
        SDL_Color optionBorder = isActive ? theme.heroTitle : colony::color::Mix(theme.border, theme.libraryCard, 0.45f);
        SDL_SetRenderDrawColor(renderer, optionBase.r, optionBase.g, optionBase.b, optionBase.a);
        colony::drawing::RenderFilledRoundedRect(renderer, drawOptionRect, 14);
        SDL_SetRenderDrawColor(renderer, optionBorder.r, optionBorder.g, optionBorder.b, optionBorder.a);
        colony::drawing::RenderRoundedRect(renderer, drawOptionRect, 14);

        const int indicatorWidth = Scale(6);
        SDL_Rect indicatorRect{optionRect.x, optionRect.y, indicatorWidth, optionRect.h};
        SDL_Rect drawIndicatorRect = offsetRect(indicatorRect);
        SDL_Color indicatorColor = isActive ? theme.heroTitle : colony::color::Mix(theme.border, theme.libraryCard, 0.5f);
        SDL_SetRenderDrawColor(renderer, indicatorColor.r, indicatorColor.g, indicatorColor.b, indicatorColor.a);
        SDL_RenderFillRect(renderer, &drawIndicatorRect);

        const int optionPadding = Scale(20);
        int optionContentX = optionRect.x + optionPadding + indicatorWidth;
        int optionContentY = optionRect.y + optionPadding - Scale(4);
        if (option.label.texture)
        {
            SDL_Rect labelRect{optionContentX, optionContentY, option.label.width, option.label.height};
            colony::RenderTexture(renderer, option.label, offsetRect(labelRect));
            optionContentY += option.label.height + Scale(10);
        }

        int swatchX = optionRect.x + optionPadding + indicatorWidth;
        const int swatchHeight = Scale(14);
        const int swatchSpacing = Scale(8);
        for (const SDL_Color& swatchColor : option.swatch)
        {
            SDL_Rect swatchRect{swatchX, optionContentY, Scale(34), swatchHeight};
            SDL_Rect drawSwatchRect = offsetRect(swatchRect);
            SDL_SetRenderDrawColor(renderer, swatchColor.r, swatchColor.g, swatchColor.b, swatchColor.a);
            colony::drawing::RenderFilledRoundedRect(renderer, drawSwatchRect, swatchHeight / 2);
            SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
            colony::drawing::RenderRoundedRect(renderer, drawSwatchRect, swatchHeight / 2);
            swatchX += Scale(34) + swatchSpacing;
        }

        if (isActive)
        {
            const int badgeSize = Scale(26);
            SDL_Rect badgeRect{
                optionRect.x + optionRect.w - badgeSize - optionPadding,
                optionRect.y + (optionRect.h - badgeSize) / 2,
                badgeSize,
                badgeSize};
            SDL_Rect drawBadgeRect = offsetRect(badgeRect);
            SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, theme.heroTitle.a);
            colony::drawing::RenderFilledRoundedRect(renderer, drawBadgeRect, badgeSize / 2);

            SDL_SetRenderDrawColor(renderer, theme.background.r, theme.background.g, theme.background.b, SDL_ALPHA_OPAQUE);
            const int checkStartX = drawBadgeRect.x + Scale(7);
            const int checkStartY = drawBadgeRect.y + badgeRect.h / 2;
            const int checkMidX = checkStartX + Scale(5);
            const int checkMidY = checkStartY + Scale(4);
            const int checkEndX = drawBadgeRect.x + badgeRect.w - Scale(5);
            const int checkEndY = drawBadgeRect.y + Scale(8);
            SDL_RenderDrawLine(renderer, checkStartX, checkStartY, checkMidX, checkMidY);
            SDL_RenderDrawLine(renderer, checkMidX, checkMidY, checkEndX, checkEndY);
        }

        addInteractiveRegion(option.id, RenderResult::InteractionType::ThemeSelection, offsetRect(optionRect));

        optionY += themeRowHeight + themeRowSpacing;
    }

    cursorY = menuRect.y + menuRect.h + Scale(18);

    if (customizationHint_.texture)
    {
        SDL_Rect hintRect{bounds.x + horizontalPadding, cursorY, customizationHint_.width, customizationHint_.height};
        colony::RenderTexture(renderer, customizationHint_, offsetRect(hintRect));
        cursorY += hintRect.h + Scale(16);
    }

    const int customizationCardHeight = Scale(112);
    const int customizationSpacing = Scale(16);
    for (std::size_t index = 0; index < appearanceCustomizations_.size(); ++index)
    {
        const auto& customization = appearanceCustomizations_[index];
        SDL_Rect cardRect{bounds.x + horizontalPadding, cursorY, availableWidth, customizationCardHeight};
        SDL_Rect drawCardRect = offsetRect(cardRect);
        SDL_Color cardBase = colony::color::Mix(theme.libraryCard, theme.background, 0.35f);
        SDL_SetRenderDrawColor(renderer, cardBase.r, cardBase.g, cardBase.b, cardBase.a);
        colony::drawing::RenderFilledRoundedRect(renderer, drawCardRect, 18);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        colony::drawing::RenderRoundedRect(renderer, drawCardRect, 18);

        const int accentWidth = Scale(5);
        SDL_Rect accentRect{cardRect.x, cardRect.y, accentWidth, cardRect.h};
        SDL_Rect drawAccentRect = offsetRect(accentRect);
        SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, theme.heroTitle.a);
        SDL_RenderFillRect(renderer, &drawAccentRect);

        const int contentX = cardRect.x + Scale(20) + accentWidth;
        int contentY = cardRect.y + Scale(20);
        if (customization.label.texture)
        {
            SDL_Rect labelRect{contentX, contentY, customization.label.width, customization.label.height};
            colony::RenderTexture(renderer, customization.label, offsetRect(labelRect));
            contentY += labelRect.h + Scale(10);
        }

        if (customization.description.texture)
        {
            SDL_Rect descriptionRect{contentX, contentY, customization.description.width, customization.description.height};
            colony::RenderTexture(renderer, customization.description, offsetRect(descriptionRect));
            contentY += descriptionRect.h + Scale(16);
        }

        const int sliderWidth = std::max(Scale(120), cardRect.w - Scale(120));
        const int sliderHeight = Scale(12);
        SDL_Rect sliderRect{contentX, cardRect.y + cardRect.h - Scale(40), sliderWidth, sliderHeight};
        SDL_Rect drawSliderRect = offsetRect(sliderRect);
        SDL_Color trackColor = colony::color::Mix(theme.muted, theme.libraryCard, 0.5f);
        SDL_SetRenderDrawColor(renderer, trackColor.r, trackColor.g, trackColor.b, trackColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, drawSliderRect, sliderHeight / 2);

        const float knobRatio = 0.35f + static_cast<float>(index) * 0.25f;
        const int knobSize = Scale(28);
        const int knobTravel = std::max(0, sliderRect.w - knobSize);
        SDL_Rect knobRect{
            sliderRect.x + static_cast<int>(std::clamp(knobRatio, 0.0f, 1.0f) * knobTravel),
            sliderRect.y - (knobSize - sliderHeight) / 2,
            knobSize,
            knobSize};
        SDL_Rect drawKnobRect = offsetRect(knobRect);
        SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, theme.heroTitle.a);
        colony::drawing::RenderFilledRoundedRect(renderer, drawKnobRect, knobSize / 2);
        SDL_SetRenderDrawColor(renderer, theme.background.r, theme.background.g, theme.background.b, SDL_ALPHA_OPAQUE);
        colony::drawing::RenderRoundedRect(renderer, drawKnobRect, knobSize / 2);

        addInteractiveRegion(
            customization.id,
            RenderResult::InteractionType::Customization,
            offsetRect(knobRect));

        cursorY += customizationCardHeight + customizationSpacing;
    }

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
