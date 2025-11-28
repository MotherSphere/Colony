#pragma once

#include "ui/theme.hpp"
#include "utils/text.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <string>
#include <string_view>

namespace colony::frontend::components
{

class SearchField
{
  public:
    void Build(SDL_Renderer* renderer, TTF_Font* font, std::string_view placeholder, const colony::ui::ThemeColors& theme);

    struct RenderResult
    {
        SDL_Rect inputRect{0, 0, 0, 0};
    };

    RenderResult Render(
        SDL_Renderer* renderer,
        const colony::ui::ThemeColors& theme,
        const colony::ui::InteractionColors& interactions,
        const SDL_Rect& bounds,
        std::string_view value,
        bool focused,
        double timeSeconds) const;

  private:
    TTF_Font* font_ = nullptr;
    colony::TextTexture placeholder_;
    mutable colony::TextTexture cachedValueTexture_;
    mutable std::string cachedValue_;
    mutable SDL_Color cachedTextColor_{0, 0, 0, 0};
};

} // namespace colony::frontend::components
