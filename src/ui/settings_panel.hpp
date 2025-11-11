#pragma once

#include "ui/theme.hpp"
#include "utils/text.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace colony::ui
{

class SettingsPanel
{
  public:
    void Build(
        SDL_Renderer* renderer,
        TTF_Font* titleFont,
        TTF_Font* bodyFont,
        SDL_Color titleColor,
        SDL_Color bodyColor,
        const ThemeManager& themeManager,
        const std::function<std::string(std::string_view)>& localize,
        const std::function<TTF_Font*(std::string_view)>& nativeFontResolver);

    struct RenderResult
    {
        enum class InteractionType
        {
            ThemeSelection,
            LanguageSelection,
            Toggle,
            Customization,
            SectionToggle
        };

        struct InteractiveRegion
        {
            std::string id;
            InteractionType type;
            SDL_Rect rect{0, 0, 0, 0};
        };

        std::vector<InteractiveRegion> interactiveRegions;
        SDL_Rect viewport{0, 0, 0, 0};
        int contentHeight = 0;
    };

    struct SectionStates
    {
        bool appearanceExpanded = true;
        bool languageExpanded = true;
        bool generalExpanded = true;
    };

    inline static constexpr std::string_view kAppearanceSectionId = "settings.section.appearance";
    inline static constexpr std::string_view kLanguageSectionId = "settings.section.language";
    inline static constexpr std::string_view kGeneralSectionId = "settings.section.general";

    RenderResult Render(
        SDL_Renderer* renderer,
        const SDL_Rect& bounds,
        int scrollOffset,
        const ThemeColors& theme,
        std::string_view activeSchemeId,
        std::string_view activeLanguageId,
        const SectionStates& sectionStates,
        const std::unordered_map<std::string, bool>& toggleStates,
        const std::unordered_map<std::string, float>& customizationValues) const;

  private:
    struct ThemeOption
    {
        std::string id;
        colony::TextTexture label;
        std::array<SDL_Color, 3> swatch{};
        mutable SDL_Rect rect{0, 0, 0, 0};
    };

    struct LanguageOption
    {
        std::string id;
        colony::TextTexture name;
        colony::TextTexture nativeName;
    };

    struct ToggleOption
    {
        std::string id;
        colony::TextTexture label;
        colony::TextTexture description;
    };

    struct CustomizationOption
    {
        std::string id;
        colony::TextTexture label;
        colony::TextTexture description;
    };

    colony::TextTexture appearanceTitle_;
    colony::TextTexture appearanceSubtitle_;
    colony::TextTexture languageTitle_;
    colony::TextTexture languageSubtitle_;
    colony::TextTexture generalTitle_;
    colony::TextTexture generalSubtitle_;
    colony::TextTexture customizationHint_;

    std::vector<ThemeOption> themeOptions_;
    std::vector<LanguageOption> languages_;
    std::vector<ToggleOption> toggles_;
    std::vector<CustomizationOption> appearanceCustomizations_;
};

} // namespace colony::ui
