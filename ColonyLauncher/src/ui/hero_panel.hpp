#pragma once

#include "ui/program_visuals.hpp"
#include "ui/settings_panel.hpp"
#include "ui/theme.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <optional>
#include <functional>
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
    void Build(
        SDL_Renderer* renderer,
        TTF_Font* labelFont,
        const ThemeColors& theme,
        const std::function<std::string(std::string_view)>& localize);

    HeroRenderResult RenderHero(
        SDL_Renderer* renderer,
        const ThemeColors& theme,
        const SDL_Rect& heroRect,
        ProgramVisuals& visuals,
        TTF_Font* heroBodyFont,
        TTF_Font* patchTitleFont,
        TTF_Font* patchBodyFont,
        double timeSeconds,
        double deltaSeconds) const;

    void RenderSettings(
        SDL_Renderer* renderer,
        const ThemeColors& theme,
        const SDL_Rect& heroRect,
        const SettingsPanel& panel,
        int scrollOffset,
        std::string_view activeSchemeId,
        std::string_view activeLanguageId,
        const SettingsPanel::SectionStates& sectionStates,
        const std::unordered_map<std::string, float>& customizationValues,
        const std::unordered_map<std::string, bool>& toggleStates,
        SettingsPanel::RenderResult& outResult,
        double timeSeconds) const;

    void RenderStatusBar(
        SDL_Renderer* renderer,
        const ThemeColors& theme,
        const SDL_Rect& heroRect,
        int statusBarHeight,
        const ProgramVisuals* visuals,
        double timeSeconds) const;

  private:
    struct HeroChrome
    {
        colony::TextTexture capabilitiesLabel;
        colony::TextTexture updatesLabel;
    };

    HeroChrome chrome_;
};

} // namespace colony::ui
