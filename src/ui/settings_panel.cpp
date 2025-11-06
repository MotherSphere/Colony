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
    categories_.clear();
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

    const SDL_Color accentBase = colony::color::Mix(titleColor, bodyColor, 0.35f);
    const auto addCategory = [&](std::string id, std::string label, std::string subtitle, float accentMix) {
        Category category;
        category.id = std::move(id);
        category.label = colony::CreateTextTexture(renderer, titleFont, label, titleColor);
        category.subtitle = colony::CreateTextTexture(renderer, bodyFont, subtitle, secondaryColor);
        category.accent = colony::color::Mix(accentBase, titleColor, accentMix);
        categories_.emplace_back(std::move(category));
    };

    addCategory(std::string(kAppearanceCategoryId), "Appearance", "Themes & layout", 0.35f);
    addCategory(std::string(kLanguageCategoryId), "Language", "Translators & locales", 0.5f);
    addCategory(std::string(kGeneralCategoryId), "General", "Everyday options", 0.2f);

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
    int scrollOffset,
    const ThemeColors& theme,
    std::string_view activeCategoryId,
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
    const int horizontalPadding = 28;
    const int availableWidth = bounds.w - horizontalPadding * 2;

    std::string resolvedCategory = std::string(activeCategoryId);
    const bool hasCategories = !categories_.empty();
    if (hasCategories)
    {
        const bool isValid = std::any_of(categories_.begin(), categories_.end(), [&](const Category& category) {
            return category.id == resolvedCategory;
        });
        if (!isValid)
        {
            resolvedCategory = categories_.front().id;
        }
    }

    const auto drawCategoryGlyph = [&](const Category& category, const SDL_Rect& drawRect) {
        SDL_Color overlay = colony::color::Mix(category.accent, theme.background, 0.2f);
        SDL_SetRenderDrawColor(renderer, overlay.r, overlay.g, overlay.b, overlay.a);

        if (category.id == kAppearanceCategoryId)
        {
            SDL_Rect swatch1{drawRect.x + 6, drawRect.y + 6, 10, 10};
            SDL_Rect swatch2{drawRect.x + 18, drawRect.y + 12, 12, 12};
            SDL_Rect swatch3{drawRect.x + 8, drawRect.y + 22, 14, 14};
            colony::drawing::RenderFilledRoundedRect(renderer, swatch1, 4);
            colony::drawing::RenderFilledRoundedRect(renderer, swatch2, 5);
            colony::drawing::RenderFilledRoundedRect(renderer, swatch3, 6);
        }
        else if (category.id == kLanguageCategoryId)
        {
            SDL_Rect bubble{drawRect.x + 5, drawRect.y + 7, drawRect.w - 10, drawRect.h - 16};
            colony::drawing::RenderFilledRoundedRect(renderer, bubble, 6);
            SDL_RenderDrawLine(renderer, bubble.x + 10, bubble.y + bubble.h, bubble.x + 16, bubble.y + bubble.h + 6);
            SDL_RenderDrawLine(renderer, bubble.x + 16, bubble.y + bubble.h + 6, bubble.x + 20, bubble.y + bubble.h);
        }
        else
        {
            const int barWidth = 6;
            const int gap = 4;
            SDL_Rect bar1{drawRect.x + 6, drawRect.y + 10, barWidth, drawRect.h - 20};
            SDL_Rect bar2{bar1.x + barWidth + gap, bar1.y + 4, barWidth, bar1.h - 8};
            SDL_Rect bar3{bar2.x + barWidth + gap, bar1.y + 8, barWidth, bar1.h - 16};
            colony::drawing::RenderFilledRoundedRect(renderer, bar1, 3);
            colony::drawing::RenderFilledRoundedRect(renderer, bar2, 3);
            colony::drawing::RenderFilledRoundedRect(renderer, bar3, 3);
        }
    };

    if (hasCategories && availableWidth > 0)
    {
        const int categorySpacing = 16;
        const int categoryCount = static_cast<int>(categories_.size());
        const int categoryCardWidth = categoryCount > 0
                                          ? (availableWidth - categorySpacing * (categoryCount - 1)) / categoryCount
                                          : availableWidth;
        const int categoryCardHeight = 88;

        for (int index = 0; index < categoryCount; ++index)
        {
            const auto& category = categories_[index];
            SDL_Rect logicalCardRect{
                bounds.x + horizontalPadding + index * (categoryCardWidth + categorySpacing),
                cursorY,
                categoryCardWidth,
                categoryCardHeight};
            SDL_Rect cardRect = offsetRect(logicalCardRect);
            const bool isActive = category.id == resolvedCategory;

            const SDL_Color baseColor = isActive ? colony::color::Mix(theme.libraryCardActive, category.accent, 0.2f)
                                                  : theme.libraryCard;
            const SDL_Color borderColor = isActive ? category.accent : theme.border;

            SDL_SetRenderDrawColor(renderer, baseColor.r, baseColor.g, baseColor.b, baseColor.a);
            colony::drawing::RenderFilledRoundedRect(renderer, cardRect, 18);
            SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
            colony::drawing::RenderRoundedRect(renderer, cardRect, 18);

            const int iconSize = 44;
            SDL_Rect iconRect{
                logicalCardRect.x + 16,
                logicalCardRect.y + (logicalCardRect.h - iconSize) / 2,
                iconSize,
                iconSize};
            SDL_Rect drawIconRect = offsetRect(iconRect);
            SDL_Color iconFill = colony::color::Mix(category.accent, theme.libraryCard, 0.25f);
            SDL_SetRenderDrawColor(renderer, iconFill.r, iconFill.g, iconFill.b, iconFill.a);
            colony::drawing::RenderFilledRoundedRect(renderer, drawIconRect, 12);
            SDL_SetRenderDrawColor(renderer, category.accent.r, category.accent.g, category.accent.b, category.accent.a);
            colony::drawing::RenderRoundedRect(renderer, drawIconRect, 12);
            drawCategoryGlyph(category, drawIconRect);

            int textX = iconRect.x + iconRect.w + 14;
            int textY = logicalCardRect.y + 16;
            if (category.label.texture)
            {
                SDL_Rect labelRect{textX, textY, category.label.width, category.label.height};
                colony::RenderTexture(renderer, category.label, offsetRect(labelRect));
                textY += labelRect.h + 6;
            }
            if (category.subtitle.texture)
            {
                SDL_Rect subtitleRect{textX, textY, category.subtitle.width, category.subtitle.height};
                colony::RenderTexture(renderer, category.subtitle, offsetRect(subtitleRect));
            }

            addInteractiveRegion(category.id, RenderResult::InteractionType::CategorySelection, cardRect);
        }
        cursorY += categoryCardHeight + 24;
    }

    const auto drawSectionHeader = [&](const colony::TextTexture& title, const colony::TextTexture& subtitle) {
        if (title.texture)
        {
            SDL_Rect titleRect{bounds.x + horizontalPadding, cursorY, title.width, title.height};
            colony::RenderTexture(renderer, title, offsetRect(titleRect));
            cursorY += titleRect.h + 6;
        }

        if (subtitle.texture)
        {
            SDL_Rect subtitleRect{bounds.x + horizontalPadding, cursorY, subtitle.width, subtitle.height};
            colony::RenderTexture(renderer, subtitle, offsetRect(subtitleRect));
            cursorY += subtitleRect.h + 16;
        }
    };

    if (resolvedCategory == kAppearanceCategoryId)
    {
        drawSectionHeader(appearanceTitle_, appearanceSubtitle_);

        const int themeCardSpacing = 12;
        const int themeColumns = availableWidth >= 640 ? 2 : 1;
        const int themeCardWidth = themeColumns == 1 ? availableWidth : (availableWidth - themeCardSpacing) / 2;
        const int themeCardHeight = 104;

        for (std::size_t index = 0; index < themeOptions_.size(); ++index)
        {
            const int column = themeColumns == 1 ? 0 : static_cast<int>(index % themeColumns);
            const int row = themeColumns == 1 ? static_cast<int>(index) : static_cast<int>(index / themeColumns);
            const int cardX = bounds.x + horizontalPadding + column * (themeCardWidth + themeCardSpacing);
            const int cardY = cursorY + row * (themeCardHeight + themeCardSpacing);
            SDL_Rect logicalCardRect{cardX, cardY, themeCardWidth, themeCardHeight};
            SDL_Rect cardRect = offsetRect(logicalCardRect);

            const bool isActive = themeOptions_[index].id == activeSchemeId;
            const SDL_Color baseColor = isActive ? colony::color::Mix(theme.libraryCardActive, theme.heroTitle, 0.12f)
                                                 : theme.libraryCard;
            const SDL_Color borderColor = isActive ? theme.heroTitle : theme.border;

            SDL_SetRenderDrawColor(renderer, baseColor.r, baseColor.g, baseColor.b, baseColor.a);
            colony::drawing::RenderFilledRoundedRect(renderer, cardRect, 18);
            SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
            colony::drawing::RenderRoundedRect(renderer, cardRect, 18);

            const auto& option = themeOptions_[index];
            const int iconSize = 40;
            SDL_Rect iconRect{
                logicalCardRect.x + 18,
                logicalCardRect.y + 18,
                iconSize,
                iconSize};
            SDL_Rect drawIconRect = offsetRect(iconRect);
            SDL_Color iconFill = colony::color::Mix(option.swatch[0], theme.libraryCard, 0.3f);
            SDL_SetRenderDrawColor(renderer, iconFill.r, iconFill.g, iconFill.b, iconFill.a);
            colony::drawing::RenderFilledRoundedRect(renderer, drawIconRect, 12);
            SDL_SetRenderDrawColor(renderer, option.swatch[0].r, option.swatch[0].g, option.swatch[0].b, option.swatch[0].a);
            colony::drawing::RenderRoundedRect(renderer, drawIconRect, 12);

            const int glyphSize = 12;
            SDL_Rect glyph1{iconRect.x + 6, iconRect.y + 6, glyphSize, glyphSize};
            SDL_Rect glyph2{iconRect.x + iconRect.w - glyphSize - 6, iconRect.y + 8, glyphSize, glyphSize};
            SDL_Rect glyph3{iconRect.x + 10, iconRect.y + iconRect.h - glyphSize - 6, glyphSize, glyphSize};
            SDL_Rect drawGlyph1 = offsetRect(glyph1);
            SDL_Rect drawGlyph2 = offsetRect(glyph2);
            SDL_Rect drawGlyph3 = offsetRect(glyph3);
            SDL_SetRenderDrawColor(renderer, option.swatch[1].r, option.swatch[1].g, option.swatch[1].b, option.swatch[1].a);
            colony::drawing::RenderFilledRoundedRect(renderer, drawGlyph1, 4);
            SDL_SetRenderDrawColor(renderer, option.swatch[2].r, option.swatch[2].g, option.swatch[2].b, option.swatch[2].a);
            colony::drawing::RenderFilledRoundedRect(renderer, drawGlyph2, 4);
            SDL_SetRenderDrawColor(renderer, option.swatch[0].r, option.swatch[0].g, option.swatch[0].b, option.swatch[0].a);
            colony::drawing::RenderFilledRoundedRect(renderer, drawGlyph3, 4);

            SDL_Rect labelRect{
                iconRect.x + iconRect.w + 14,
                logicalCardRect.y + 20,
                option.label.width,
                option.label.height};
            colony::RenderTexture(renderer, option.label, offsetRect(labelRect));

            const int swatchWidth = (logicalCardRect.w - 36 - 12 * 2) / 3;
            const int swatchHeight = 26;
            int swatchX = logicalCardRect.x + 18;
            const int swatchY = logicalCardRect.y + logicalCardRect.h - swatchHeight - 18;
            for (const SDL_Color& swatchColor : option.swatch)
            {
                SDL_Rect swatchRect{swatchX, swatchY, swatchWidth, swatchHeight};
                SDL_Rect swatchDrawRect = offsetRect(swatchRect);
                SDL_SetRenderDrawColor(renderer, swatchColor.r, swatchColor.g, swatchColor.b, swatchColor.a);
                colony::drawing::RenderFilledRoundedRect(renderer, swatchDrawRect, 8);
                SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
                colony::drawing::RenderRoundedRect(renderer, swatchDrawRect, 8);
                swatchX += swatchWidth + 12;
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
        cursorY += 8;
    }
    else if (resolvedCategory == kLanguageCategoryId)
    {
        drawSectionHeader(languageTitle_, languageSubtitle_);

        const int languageCardHeight = 72;
        const int languageCardSpacing = 12;
        for (const auto& language : languages_)
        {
            SDL_Rect logicalCardRect{bounds.x + horizontalPadding, cursorY, availableWidth, languageCardHeight};
            SDL_Rect cardRect = offsetRect(logicalCardRect);
            const bool isActive = language.id == activeLanguageId;
            const SDL_Color baseColor = isActive ? colony::color::Mix(theme.libraryCardActive, theme.heroTitle, 0.12f)
                                                 : theme.libraryCard;
            const SDL_Color borderColor = isActive ? theme.heroTitle : theme.border;

            SDL_SetRenderDrawColor(renderer, baseColor.r, baseColor.g, baseColor.b, baseColor.a);
            colony::drawing::RenderFilledRoundedRect(renderer, cardRect, 16);
            SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
            colony::drawing::RenderRoundedRect(renderer, cardRect, 16);

            const int iconSize = 40;
            SDL_Rect iconRect{
                logicalCardRect.x + 18,
                logicalCardRect.y + (logicalCardRect.h - iconSize) / 2,
                iconSize,
                iconSize};
            SDL_Rect drawIconRect = offsetRect(iconRect);
            SDL_Color iconBase = colony::color::Mix(theme.libraryCardActive, theme.heroTitle, 0.25f);
            SDL_SetRenderDrawColor(renderer, iconBase.r, iconBase.g, iconBase.b, iconBase.a);
            colony::drawing::RenderFilledRoundedRect(renderer, drawIconRect, 12);
            SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
            colony::drawing::RenderRoundedRect(renderer, drawIconRect, 12);
            SDL_RenderDrawLine(
                renderer,
                drawIconRect.x + 10,
                drawIconRect.y + drawIconRect.h / 2,
                drawIconRect.x + drawIconRect.w - 10,
                drawIconRect.y + drawIconRect.h / 2);
            SDL_RenderDrawLine(
                renderer,
                drawIconRect.x + 14,
                drawIconRect.y + drawIconRect.h / 2 - 8,
                drawIconRect.x + drawIconRect.w - 12,
                drawIconRect.y + drawIconRect.h / 2 - 8);

            int contentX = iconRect.x + iconRect.w + 14;
            int contentY = logicalCardRect.y + 14;
            if (language.name.texture)
            {
                SDL_Rect nameRect{contentX, contentY, language.name.width, language.name.height};
                colony::RenderTexture(renderer, language.name, offsetRect(nameRect));
                contentY += language.name.height + 4;
            }

            if (language.nativeName.texture)
            {
                SDL_Rect nativeRect{contentX, contentY, language.nativeName.width, language.nativeName.height};
                colony::RenderTexture(renderer, language.nativeName, offsetRect(nativeRect));
            }

            const int radioSize = 24;
            SDL_Rect radioRect{
                logicalCardRect.x + logicalCardRect.w - radioSize - 20,
                logicalCardRect.y + (logicalCardRect.h - radioSize) / 2,
                radioSize,
                radioSize};
            SDL_Rect drawRadioRect = offsetRect(radioRect);
            SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
            colony::drawing::RenderRoundedRect(renderer, drawRadioRect, radioSize / 2);
            if (isActive)
            {
                SDL_Rect innerRect{radioRect.x + 5, radioRect.y + 5, radioRect.w - 10, radioRect.h - 10};
                SDL_Rect drawInnerRect = offsetRect(innerRect);
                SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, theme.heroTitle.a);
                colony::drawing::RenderFilledRoundedRect(renderer, drawInnerRect, drawInnerRect.w / 2);
            }

            addInteractiveRegion(language.id, RenderResult::InteractionType::LanguageSelection, cardRect);

            cursorY += languageCardHeight + languageCardSpacing;
        }
        cursorY += 8;
    }
    else if (resolvedCategory == kGeneralCategoryId)
    {
        drawSectionHeader(generalTitle_, generalSubtitle_);

        const int toggleCardHeight = 88;
        const int toggleCardSpacing = 12;
        for (const auto& toggle : toggles_)
        {
            SDL_Rect logicalCardRect{bounds.x + horizontalPadding, cursorY, availableWidth, toggleCardHeight};
            SDL_Rect cardRect = offsetRect(logicalCardRect);
            SDL_SetRenderDrawColor(renderer, theme.libraryCard.r, theme.libraryCard.g, theme.libraryCard.b, theme.libraryCard.a);
            colony::drawing::RenderFilledRoundedRect(renderer, cardRect, 16);
            SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
            colony::drawing::RenderRoundedRect(renderer, cardRect, 16);

            const int iconSize = 36;
            SDL_Rect iconRect{
                logicalCardRect.x + 20,
                logicalCardRect.y + (logicalCardRect.h - iconSize) / 2,
                iconSize,
                iconSize};
            SDL_Rect drawIconRect = offsetRect(iconRect);
            SDL_Color iconAccent = colony::color::Mix(theme.heroTitle, theme.libraryCard, 0.35f);
            SDL_SetRenderDrawColor(renderer, iconAccent.r, iconAccent.g, iconAccent.b, iconAccent.a);
            colony::drawing::RenderFilledRoundedRect(renderer, drawIconRect, 10);
            SDL_SetRenderDrawColor(renderer, theme.heroTitle.r, theme.heroTitle.g, theme.heroTitle.b, theme.heroTitle.a);
            colony::drawing::RenderRoundedRect(renderer, drawIconRect, 10);
            const int sliderY = drawIconRect.y + drawIconRect.h / 2;
            SDL_RenderDrawLine(renderer, drawIconRect.x + 6, sliderY - 6, drawIconRect.x + drawIconRect.w - 6, sliderY - 6);
            SDL_RenderDrawLine(renderer, drawIconRect.x + 6, sliderY + 4, drawIconRect.x + drawIconRect.w - 6, sliderY + 4);
            SDL_Rect sliderKnob{drawIconRect.x + 10, sliderY - 10, 8, 8};
            SDL_Rect sliderKnob2{drawIconRect.x + drawIconRect.w - 16, sliderY, 8, 8};
            colony::drawing::RenderFilledRoundedRect(renderer, sliderKnob, 3);
            colony::drawing::RenderFilledRoundedRect(renderer, sliderKnob2, 3);

            const int contentX = iconRect.x + iconRect.w + 16;
            int contentY = logicalCardRect.y + 18;
            if (toggle.label.texture)
            {
                SDL_Rect labelRect{contentX, contentY, toggle.label.width, toggle.label.height};
                colony::RenderTexture(renderer, toggle.label, offsetRect(labelRect));
                contentY += toggle.label.height + 4;
            }

            if (toggle.description.texture)
            {
                SDL_Rect descriptionRect{contentX, contentY, toggle.description.width, toggle.description.height};
                colony::RenderTexture(renderer, toggle.description, offsetRect(descriptionRect));
            }

            const int switchWidth = 62;
            const int switchHeight = 30;
            SDL_Rect switchRect{
                logicalCardRect.x + logicalCardRect.w - switchWidth - 22,
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

            const int handleSize = switchHeight - 8;
            SDL_Rect handleRect{
                isEnabled ? switchRect.x + switchRect.w - handleSize - 4 : switchRect.x + 4,
                switchRect.y + 4,
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
        cursorY += 8;
    }

    result.contentHeight = cursorY - bounds.y;
    return result;
}

} // namespace colony::ui
