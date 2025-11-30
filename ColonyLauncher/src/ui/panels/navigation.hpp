#pragma once

#include "core/content.hpp"
#include "frontend/components/sidebar_item.hpp"
#include "ui/program_visuals.hpp"
#include "ui/theme.hpp"
#include "utils/text.hpp"

#include <SDL2/SDL.h>

#include <optional>
#include <unordered_map>
#include <vector>

namespace colony::ui::panels
{

struct NavigationRenderResult
{
    std::vector<SDL_Rect> channelButtonRects;
    std::optional<SDL_Rect> hubButtonRect;
};

class NavigationRailPanel
{
  public:
    void Build(
        SDL_Renderer* renderer,
        TTF_Font* brandFont,
        TTF_Font* navFont,
        TTF_Font* metaFont,
        const colony::AppContent& content,
        const ThemeColors& theme,
        const Typography& typography);

    NavigationRenderResult Render(
        SDL_Renderer* renderer,
        const ThemeColors& theme,
        const Typography& typography,
        const InteractionColors& interactions,
        const SDL_Rect& navRailRect,
        int statusBarHeight,
        const colony::AppContent& content,
        const std::vector<int>& channelSelections,
        int activeChannelIndex,
        const std::unordered_map<std::string, ProgramVisuals>& programVisuals,
        double timeSeconds) const;

    bool OnClick(int /*x*/, int /*y*/) const { return false; }
    bool OnWheel(const SDL_MouseWheelEvent& /*wheel*/) const { return false; }
    bool OnKey(SDL_Keycode /*key*/) const { return false; }

  private:
    struct NavigationChrome
    {
        colony::TextTexture brand;
        std::vector<frontend::components::SidebarItem> items;
    };

    NavigationChrome chrome_;
    NavigationRenderResult lastRender_{};
};

} // namespace colony::ui::panels
