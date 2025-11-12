#pragma once

#include "ui/theme.hpp"

#include "utils/text.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <string>
#include <string_view>
#include <vector>

namespace colony::ui
{

struct HubBranchContent
{
    std::string id;
    std::string title;
    std::string description;
    SDL_Color accent{0, 0, 0, SDL_ALPHA_OPAQUE};
};

struct HubContent
{
    std::string headline;
    std::string description;
    std::vector<HubBranchContent> branches;
};

struct HubRenderResult
{
    struct BranchHitbox
    {
        std::string id;
        SDL_Rect rect{0, 0, 0, 0};
    };

    SDL_Rect heroRect{0, 0, 0, 0};
    std::vector<BranchHitbox> branchHitboxes;
};

class HubPanelRenderer
{
  public:
    void Build(
        SDL_Renderer* renderer,
        const HubContent& content,
        TTF_Font* headlineFont,
        TTF_Font* heroBodyFont,
        TTF_Font* tileTitleFont,
        TTF_Font* tileBodyFont,
        const ThemeColors& theme);

    [[nodiscard]] HubRenderResult Render(
        SDL_Renderer* renderer,
        const ThemeColors& theme,
        const SDL_Rect& bounds,
        double timeSeconds,
        int hoveredBranchIndex,
        int activeBranchIndex) const;

  private:
    struct HeroChrome
    {
        colony::TextTexture headline;
        std::string description;
        mutable int descriptionWidth = 0;
        mutable std::vector<colony::TextTexture> descriptionLines;
    };

    struct BranchChrome
    {
        std::string id;
        colony::TextTexture title;
        std::string description;
        SDL_Color accent{0, 0, 0, SDL_ALPHA_OPAQUE};
        mutable int descriptionWidth = 0;
        mutable std::vector<colony::TextTexture> bodyLines;
    };

    void RebuildHeroDescription(SDL_Renderer* renderer, int maxWidth, SDL_Color color) const;
    void RebuildBranchDescription(SDL_Renderer* renderer, BranchChrome& branch, int maxWidth, SDL_Color color) const;

    mutable HeroChrome hero_;
    mutable std::vector<BranchChrome> branches_;

    TTF_Font* heroBodyFont_ = nullptr;
    TTF_Font* tileBodyFont_ = nullptr;
};

} // namespace colony::ui

