#include "ui/settings_panel.hpp"

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
    const ThemeManager& themeManager)
{
    themeOptions_.clear();
    languages_.clear();
    toggles_.clear();

    appearanceTitle_ = colony::CreateTextTexture(renderer, titleFont, "Appearance", titleColor);
    appearanceSubtitle_ = colony::CreateTextTexture(renderer, bodyFont, "Choose a preset palette for Colony", bodyColor);

    languageTitle_ = colony::CreateTextTexture(renderer, titleFont, "Language", titleColor);
    languageSubtitle_ = colony::CreateTextTexture(renderer, bodyFont, "Switch the interface to your preferred language", bodyColor);

    generalTitle_ = colony::CreateTextTexture(renderer, titleFont, "General", titleColor);
    const SDL_Color secondaryColor = colony::color::Mix(bodyColor, titleColor, 0.25f);
    generalSubtitle_ = colony::CreateTextTexture(renderer, bodyFont, "Tune everyday preferences and comfort options", secondaryColor);

    for (const auto& scheme : themeManager.Schemes())
    {
        ThemeOption option;
        option.id = scheme.id;
        option.label = colony::CreateTextTexture(renderer, bodyFont, scheme.name, bodyColor);
        option.swatch = {scheme.colors.background, scheme.colors.libraryCard, scheme.colors.heroTitle};
        themeOptions_.emplace_back(std::move(option));
    }

    const auto makeLanguage = [&](std::string id, std::string name, std::string native) {
        LanguageOption option;
        option.id = std::move(id);
        option.name = colony::CreateTextTexture(renderer, bodyFont, name, titleColor);
        option.nativeName = colony::CreateTextTexture(renderer, bodyFont, native, secondaryColor);
        languages_.emplace_back(std::move(option));
    };

    makeLanguage("en", "English", "English");
    makeLanguage("fr", "French", "Français");
    makeLanguage("zh", "Chinese", "中文");
    makeLanguage("de", "German", "Deutsch");
    makeLanguage("ar", "Arabic", "العربية");
    makeLanguage("hi", "Hindi", "हिन्दी");

    const auto makeToggle = [&](std::string id, std::string label, std::string description) {
        ToggleOption option;
        option.id = std::move(id);
        option.label = colony::CreateTextTexture(renderer, bodyFont, label, titleColor);
        option.description = colony::CreateTextTexture(renderer, bodyFont, description, secondaryColor);
        toggles_.emplace_back(std::move(option));
    };

    makeToggle("notifications", "Enable notifications", "Show alerts when new colony drops arrive");
    makeToggle("sound", "Enable interface sounds", "Play a soft chime when navigating the UI");
    makeToggle("auto_updates", "Auto-update channels", "Refresh channel content whenever Colony launches");
    makeToggle("reduced_motion", "Reduce motion", "Limit hero animations for sensitive users");
}

SettingsPanel::RenderResult SettingsPanel::Render(
    SDL_Renderer* renderer,
    const SDL_Rect& bounds,
    const ThemeColors& theme,
    std::string_view activeSchemeId,
    std::string_view activeLanguageId,
    const std::unordered_map<std::string, bool>& toggleStates) const
{
    SettingsPanel::RenderResult result;

    int cursorY = bounds.y;
    const int horizontalPadding = 32;
    const int availableWidth = bounds.w - horizontalPadding * 2;

    const auto drawSectionHeader = [&](const colony::TextTexture& title, const colony::TextTexture& subtitle) {
        if (title.texture)
        {
            SDL_Rect titleRect{bounds.x + horizontalPadding, cursorY, title.width, title.height};
            colony::RenderTexture(renderer, title, titleRect);
            cursorY += titleRect.h + 10;
        }

        if (subtitle.texture)
        {
            SDL_Rect subtitleRect{bounds.x + horizontalPadding, cursorY, subtitle.width, subtitle.height};
            colony::RenderTexture(renderer, subtitle, subtitleRect);
            cursorY += subtitleRect.h + 24;
        }
    };

    drawSectionHeader(appearanceTitle_, appearanceSubtitle_);

    const int themeCardSpacing = 20;
    const int themeColumns = availableWidth >= 640 ? 2 : 1;
    const int themeCardWidth = themeColumns == 1 ? availableWidth : (availableWidth - themeCardSpacing) / 2;
    const int themeCardHeight = 108;

    for (std::size_t index = 0; index < themeOptions_.size(); ++index)
    {
        const int column = themeColumns == 1 ? 0 : static_cast<int>(index % themeColumns);
        const int row = themeColumns == 1 ? static_cast<int>(index) : static_cast<int>(index / themeColumns);
        const int cardX = bounds.x + horizontalPadding + column * (themeCardWidth + themeCardSpacing);
        const int cardY = cursorY + row * (themeCardHeight + themeCardSpacing);
        SDL_Rect cardRect{cardX, cardY, themeCardWidth, themeCardHeight};

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
            cardRect.x + 20,
            cardRect.y + 18,
            option.label.width,
            option.label.height};
        colony::RenderTexture(renderer, option.label, labelRect);

        const int swatchWidth = (cardRect.w - 40 - 12 * 2) / 3;
        const int swatchHeight = 28;
        int swatchX = cardRect.x + 20;
        const int swatchY = cardRect.y + cardRect.h - swatchHeight - 20;
        for (const SDL_Color& swatchColor : option.swatch)
        {
            SDL_Rect swatchRect{swatchX, swatchY, swatchWidth, swatchHeight};
            SDL_SetRenderDrawColor(renderer, swatchColor.r, swatchColor.g, swatchColor.b, swatchColor.a);
            colony::drawing::RenderFilledRoundedRect(renderer, swatchRect, 8);
            SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
            colony::drawing::RenderRoundedRect(renderer, swatchRect, 8);
            swatchX += swatchWidth + 12;
        }

        result.interactiveRegions.push_back({
            option.id,
            RenderResult::InteractionType::ThemeSelection,
            cardRect});

        if (themeColumns == 1)
        {
            cursorY += themeCardHeight + themeCardSpacing;
        }
        else if (column + 1 == themeColumns)
        {
            cursorY = cardY + themeCardHeight + themeCardSpacing;
        }
    }

    cursorY += 12;

    drawSectionHeader(languageTitle_, languageSubtitle_);

    const int languageCardHeight = 74;
    const int languageCardSpacing = 16;
    for (const auto& language : languages_)
    {
        SDL_Rect cardRect{bounds.x + horizontalPadding, cursorY, availableWidth, languageCardHeight};
        const bool isActive = language.id == activeLanguageId;
        const SDL_Color baseColor = isActive ? colony::color::Mix(theme.libraryCardActive, theme.heroTitle, 0.1f)
                                             : theme.libraryCard;
        const SDL_Color borderColor = isActive ? theme.heroTitle : theme.border;

        SDL_SetRenderDrawColor(renderer, baseColor.r, baseColor.g, baseColor.b, baseColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, cardRect, 14);
        SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
        colony::drawing::RenderRoundedRect(renderer, cardRect, 14);

        const int contentX = cardRect.x + 20;
        int contentY = cardRect.y + 16;
        if (language.name.texture)
        {
            SDL_Rect nameRect{contentX, contentY, language.name.width, language.name.height};
            colony::RenderTexture(renderer, language.name, nameRect);
            contentY += language.name.height + 4;
        }

        if (language.nativeName.texture)
        {
            SDL_Rect nativeRect{contentX, contentY, language.nativeName.width, language.nativeName.height};
            colony::RenderTexture(renderer, language.nativeName, nativeRect);
        }

        const int radioSize = 24;
        SDL_Rect radioRect{
            cardRect.x + cardRect.w - radioSize - 20,
            cardRect.y + (cardRect.h - radioSize) / 2,
            radioSize,
            radioSize};
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        colony::drawing::RenderRoundedRect(renderer, radioRect, radioSize / 2);
        if (isActive)
        {
            SDL_Rect innerRect{radioRect.x + 6, radioRect.y + 6, radioRect.w - 12, radioRect.h - 12};
            SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, theme.heroTitle.a);
            colony::drawing::RenderFilledRoundedRect(renderer, innerRect, innerRect.w / 2);
        }

        result.interactiveRegions.push_back({
            language.id,
            RenderResult::InteractionType::LanguageSelection,
            cardRect});

        cursorY += languageCardHeight + languageCardSpacing;
    }

    cursorY += 12;

    drawSectionHeader(generalTitle_, generalSubtitle_);

    const int toggleCardHeight = 92;
    const int toggleCardSpacing = 16;
    for (const auto& toggle : toggles_)
    {
        SDL_Rect cardRect{bounds.x + horizontalPadding, cursorY, availableWidth, toggleCardHeight};
        SDL_SetRenderDrawColor(renderer, theme.libraryCard.r, theme.libraryCard.g, theme.libraryCard.b, theme.libraryCard.a);
        colony::drawing::RenderFilledRoundedRect(renderer, cardRect, 14);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        colony::drawing::RenderRoundedRect(renderer, cardRect, 14);

        const int contentX = cardRect.x + 20;
        int contentY = cardRect.y + 18;
        if (toggle.label.texture)
        {
            SDL_Rect labelRect{contentX, contentY, toggle.label.width, toggle.label.height};
            colony::RenderTexture(renderer, toggle.label, labelRect);
            contentY += toggle.label.height + 6;
        }

        if (toggle.description.texture)
        {
            SDL_Rect descriptionRect{contentX, contentY, toggle.description.width, toggle.description.height};
            colony::RenderTexture(renderer, toggle.description, descriptionRect);
        }

        const int switchWidth = 64;
        const int switchHeight = 32;
        SDL_Rect switchRect{
            cardRect.x + cardRect.w - switchWidth - 24,
            cardRect.y + (cardRect.h - switchHeight) / 2,
            switchWidth,
            switchHeight};

        const bool isEnabled = toggleStates.contains(toggle.id) ? toggleStates.at(toggle.id) : false;
        SDL_Color trackColor = isEnabled ? theme.heroTitle : theme.muted;
        SDL_SetRenderDrawColor(renderer, trackColor.r, trackColor.g, trackColor.b, trackColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, switchRect, switchHeight / 2);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        colony::drawing::RenderRoundedRect(renderer, switchRect, switchHeight / 2);

        const int handleSize = switchHeight - 8;
        SDL_Rect handleRect{
            isEnabled ? switchRect.x + switchRect.w - handleSize - 4 : switchRect.x + 4,
            switchRect.y + 4,
            handleSize,
            handleSize};
        SDL_SetRenderDrawColor(renderer, theme.background.r, theme.background.g, theme.background.b, SDL_ALPHA_OPAQUE);
        colony::drawing::RenderFilledRoundedRect(renderer, handleRect, handleSize / 2);
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        colony::drawing::RenderRoundedRect(renderer, handleRect, handleSize / 2);

        result.interactiveRegions.push_back({
            toggle.id,
            RenderResult::InteractionType::Toggle,
            switchRect});

        cursorY += toggleCardHeight + toggleCardSpacing;
    }

    result.contentHeight = cursorY - bounds.y;
    return result;
}

} // namespace colony::ui
