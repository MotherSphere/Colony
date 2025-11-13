#pragma once

#include "frontend/utils/icon_loader.hpp"
#include "ui/theme.hpp"
#include "utils/text.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <string>
#include <string_view>

namespace colony::frontend::components
{

class SidebarItem
{
  public:
    void Build(
        SDL_Renderer* renderer,
        TTF_Font* font,
        std::string_view id,
        std::string_view label,
        const colony::ui::ThemeColors& theme);

    SDL_Rect Render(
        SDL_Renderer* renderer,
        const colony::ui::ThemeColors& theme,
        const colony::ui::Typography& typography,
        const colony::ui::InteractionColors& interactions,
        const SDL_Rect& bounds,
        SDL_Color accent,
        bool active,
        bool hovered,
        double timeSeconds) const;

    [[nodiscard]] std::string_view Id() const noexcept { return id_; }

  private:
    std::string id_;
    colony::TextTexture labelTexture_;
};

} // namespace colony::frontend::components
