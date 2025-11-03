#pragma once

#include "core/content.hpp"
#include "utils/text.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <vector>

namespace colony::ui
{

struct WrappedLine
{
    colony::TextTexture texture;
    bool indent = false;
};

struct ProgramVisuals
{
    const colony::ViewContent* content{};

    colony::TextTexture heroTitle;
    colony::TextTexture heroTagline;
    colony::TextTexture availability;
    colony::TextTexture version;
    colony::TextTexture installState;
    colony::TextTexture lastLaunched;
    colony::TextTexture actionLabel;
    colony::TextTexture statusBar;

    colony::TextTexture tileTitle;
    colony::TextTexture tileSubtitle;
    colony::TextTexture tileMeta;

    SDL_Color accent{};
    SDL_Color gradientStart{};
    SDL_Color gradientEnd{};

    int descriptionWidth = 0;
    std::vector<std::vector<colony::TextTexture>> descriptionLines;

    int highlightsWidth = 0;
    std::vector<std::vector<WrappedLine>> highlightLines;

    struct PatchSection
    {
        colony::TextTexture title;
        int width = 0;
        std::vector<std::vector<WrappedLine>> lines;
    };
    std::vector<PatchSection> sections;
};

ProgramVisuals BuildProgramVisuals(
    const colony::ViewContent& content,
    SDL_Renderer* renderer,
    TTF_Font* heroTitleFont,
    TTF_Font* heroSubtitleFont,
    TTF_Font* heroBodyFont,
    TTF_Font* buttonFont,
    TTF_Font* tileTitleFont,
    TTF_Font* tileSubtitleFont,
    TTF_Font* tileMetaFont,
    TTF_Font* patchTitleFont,
    TTF_Font* patchBodyFont,
    TTF_Font* statusFont,
    SDL_Color heroTitleColor,
    SDL_Color heroBodyColor,
    SDL_Color heroSubtitleColor,
    SDL_Color mutedColor,
    SDL_Color statusBarTextColor,
    SDL_Color gradientFallbackStart,
    SDL_Color gradientFallbackEnd);

void RebuildDescription(
    ProgramVisuals& visuals,
    SDL_Renderer* renderer,
    TTF_Font* font,
    int maxWidth,
    SDL_Color bodyColor);

void RebuildHighlights(
    ProgramVisuals& visuals,
    SDL_Renderer* renderer,
    TTF_Font* font,
    int maxWidth,
    SDL_Color textColor);

void RebuildSections(
    ProgramVisuals& visuals,
    SDL_Renderer* renderer,
    TTF_Font* titleFont,
    TTF_Font* bodyFont,
    int maxWidth,
    SDL_Color titleColor,
    SDL_Color bodyColor);

} // namespace colony::ui
