#pragma once

#include "ui/theme.hpp"
#include "utils/text.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <array>
#include <optional>
#include <string>
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
        const ThemeManager& themeManager);

    struct RenderResult
    {
        std::vector<std::pair<std::string, SDL_Rect>> optionRects;
        int contentHeight = 0;
    };

    RenderResult Render(
        SDL_Renderer* renderer,
        const SDL_Rect& bounds,
        const ThemeColors& theme,
        std::string_view activeSchemeId) const;

  private:
    struct Option
    {
        std::string id;
        colony::TextTexture label;
        std::array<SDL_Color, 3> swatch{};
        mutable SDL_Rect rect{0, 0, 0, 0};
    };

    colony::TextTexture title_;
    colony::TextTexture subtitle_;
    std::vector<Option> options_;
};

} // namespace colony::ui
