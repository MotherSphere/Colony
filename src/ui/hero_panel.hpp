#pragma once

#include "ui/program_visuals.hpp"
#include "ui/settings_panel.hpp"
#include "ui/theme.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <optional>

namespace colony::ui
{

struct HeroChrome
{
    colony::TextTexture capabilitiesLabel;
    colony::TextTexture updatesLabel;
};

HeroChrome BuildHeroChrome(SDL_Renderer* renderer, TTF_Font* labelFont, const ThemeColors& theme);

struct HeroRenderResult
{
    std::optional<SDL_Rect> actionButtonRect;
};

HeroRenderResult RenderHeroPanel(
    SDL_Renderer* renderer,
    const ThemeColors& theme,
    const SDL_Rect& heroRect,
    ProgramVisuals& visuals,
    const HeroChrome& chrome,
    TTF_Font* heroBodyFont,
    TTF_Font* patchTitleFont,
    TTF_Font* patchBodyFont);

void RenderSettingsPanel(
    SDL_Renderer* renderer,
    const ThemeColors& theme,
    const SDL_Rect& heroRect,
    const SettingsPanel& panel,
    std::string_view activeSchemeId,
    SettingsPanel::RenderResult& outResult);

void RenderStatusBar(
    SDL_Renderer* renderer,
    const ThemeColors& theme,
    const SDL_Rect& heroRect,
    int statusBarHeight,
    const ProgramVisuals* visuals);

} // namespace colony::ui
