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
        const std::function<std::string(std::string_view)>& localize);

    struct RenderResult
    {
        enum class InteractionType
        {
            ThemeSelection,
            LanguageSelection,
            Toggle
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

    RenderResult Render(
        SDL_Renderer* renderer,
        const SDL_Rect& bounds,
        int scrollOffset,
        const ThemeColors& theme,
        std::string_view activeSchemeId,
        std::string_view activeLanguageId,
        const std::unordered_map<std::string, bool>& toggleStates) const;

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

    colony::TextTexture appearanceTitle_;
    colony::TextTexture appearanceSubtitle_;
    colony::TextTexture languageTitle_;
    colony::TextTexture languageSubtitle_;
    colony::TextTexture generalTitle_;
    colony::TextTexture generalSubtitle_;

    std::vector<ThemeOption> themeOptions_;
    std::vector<LanguageOption> languages_;
    std::vector<ToggleOption> toggles_;
};

} // namespace colony::ui
