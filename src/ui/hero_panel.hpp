#pragma once

#include "ui/program_visuals.hpp"
#include "ui/settings_panel.hpp"
#include "ui/theme.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <optional>
#include <unordered_map>

namespace colony::ui
{

struct HeroRenderResult
{
    std::optional<SDL_Rect> actionButtonRect;
};

class HeroPanelRenderer
{
  public:
    void Build(SDL_Renderer* renderer, TTF_Font* labelFont, const ThemeColors& theme);

    HeroRenderResult RenderHero(
        SDL_Renderer* renderer,
        const ThemeColors& theme,
        const SDL_Rect& heroRect,
        ProgramVisuals& visuals,
        TTF_Font* heroBodyFont,
        TTF_Font* patchTitleFont,
        TTF_Font* patchBodyFont) const;

    void RenderSettings(
        SDL_Renderer* renderer,
        const ThemeColors& theme,
        const SDL_Rect& heroRect,
        const SettingsPanel& panel,
        std::string_view activeSchemeId,
        std::string_view activeLanguageId,
        const std::unordered_map<std::string, bool>& toggleStates,
        SettingsPanel::RenderResult& outResult) const;

    void RenderStatusBar(
        SDL_Renderer* renderer,
        const ThemeColors& theme,
        const SDL_Rect& heroRect,
        int statusBarHeight,
        const ProgramVisuals* visuals) const;

  private:
    struct HeroChrome
    {
        colony::TextTexture capabilitiesLabel;
        colony::TextTexture updatesLabel;
    };

    HeroChrome chrome_;
};

} // namespace colony::ui
