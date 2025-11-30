#pragma once

#include "ui/theme.hpp"

#include "utils/text.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <string>
#include <string_view>
#include <vector>

namespace colony::ui::panels
{

struct HubBranchContent
{
    std::string id;
    std::string title;
    std::string description;
    SDL_Color accent{0, 0, 0, SDL_ALPHA_OPAQUE};
    std::vector<std::string> tags;
    std::string actionLabel;
    std::string metrics;
    std::string channelLabel;
    std::string programLabel;
    std::vector<std::string> detailBullets;
};

struct HubWidgetContent
{
    std::string id;
    std::string title;
    std::string description;
    std::vector<std::string> items;
    SDL_Color accent{0, 0, 0, SDL_ALPHA_OPAQUE};
};

struct HubContent
{
    std::string headline;
    std::string description;
    std::string searchPlaceholder;
    std::vector<HubBranchContent> branches;
    std::vector<std::string> highlights;
    std::string primaryActionLabel;
    std::string primaryActionDescription;
    std::vector<HubWidgetContent> widgets;
};

struct HubRenderResult
{
    struct BranchHitbox
    {
        std::string id;
        SDL_Rect rect{0, 0, 0, 0};
        int branchIndex = -1;
    };

    struct WidgetPagerHitbox
    {
        enum class Type
        {
            Previous,
            Next,
            Page
        } type{Type::Page};

        SDL_Rect rect{0, 0, 0, 0};
        int pageIndex = 0;
        bool enabled = true;
    };

    SDL_Rect heroRect{0, 0, 0, 0};
    SDL_Rect heroToggleRect{0, 0, 0, 0};
    SDL_Rect searchInputRect{0, 0, 0, 0};
    SDL_Rect searchClearRect{0, 0, 0, 0};
    SDL_Rect detailPanelRect{0, 0, 0, 0};
    SDL_Rect detailActionRect{0, 0, 0, 0};
    SDL_Rect scrollViewport{0, 0, 0, 0};
    std::vector<BranchHitbox> branchHitboxes;
    std::vector<WidgetPagerHitbox> widgetPagerHitboxes;
    int scrollableContentHeight = 0;
    int visibleContentHeight = 0;
    int widgetPageCount = 0;
};

class HubPanel
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
        int activeBranchIndex,
        int detailBranchIndex,
        int scrollOffset,
        bool heroCollapsed,
        std::string_view searchQuery,
        bool searchFocused,
        int widgetPage,
        int widgetsPerPage) const;

    bool OnClick(int /*x*/, int /*y*/) const { return false; }
    bool OnWheel(const SDL_MouseWheelEvent& /*wheel*/) const { return false; }
    bool OnKey(SDL_Keycode /*key*/) const { return false; }

  private:
    struct HeroChrome
    {
        colony::TextTexture headline;
        std::string description;
        mutable int descriptionWidth = 0;
        mutable std::vector<colony::TextTexture> descriptionLines;
        std::vector<colony::TextTexture> highlightChips;
        colony::TextTexture primaryActionLabel;
        std::string primaryActionDescription;
        mutable int actionDescriptionWidth = 0;
        mutable std::vector<colony::TextTexture> actionDescriptionLines;
    };

    struct BranchChrome
    {
        std::string id;
        colony::TextTexture title;
        std::string description;
        SDL_Color accent{0, 0, 0, SDL_ALPHA_OPAQUE};
        mutable int descriptionWidth = 0;
        mutable std::vector<colony::TextTexture> bodyLines;
        std::vector<colony::TextTexture> tagChips;
        colony::TextTexture actionLabel;
        colony::TextTexture metricsLabel;
        colony::TextTexture iconGlyph;
        std::string channelLabelText;
        std::string programLabelText;
        std::vector<std::string> detailBullets;
        colony::TextTexture channelLabel;
        colony::TextTexture programLabel;
        std::vector<std::vector<colony::TextTexture>> detailBulletLines;
        mutable int detailBodyWidth = 0;
        mutable std::vector<colony::TextTexture> detailBodyLines;
        mutable int detailBulletWidth = 0;
    };

    struct WidgetChrome
    {
        std::string id;
        colony::TextTexture title;
        std::string description;
        mutable int descriptionWidth = 0;
        mutable std::vector<colony::TextTexture> descriptionLines;
        std::vector<std::string> items;
        mutable int itemsWidth = 0;
        mutable std::vector<std::vector<colony::TextTexture>> itemLines;
        SDL_Color accent{0, 0, 0, SDL_ALPHA_OPAQUE};
    };

    struct SearchChrome
    {
        std::string placeholder;
        mutable colony::TextTexture placeholderTexture;
        mutable std::string lastQuery;
        mutable colony::TextTexture queryTexture;
    };

    void RebuildHeroDescription(SDL_Renderer* renderer, int maxWidth, SDL_Color color) const;
    void RebuildHeroActionDescription(SDL_Renderer* renderer, int maxWidth, SDL_Color color) const;
    void RebuildBranchDescription(SDL_Renderer* renderer, BranchChrome& branch, int maxWidth, SDL_Color color) const;
    void RebuildBranchDetailDescription(SDL_Renderer* renderer, BranchChrome& branch, int maxWidth, SDL_Color color) const;
    void RebuildWidgetDescription(SDL_Renderer* renderer, WidgetChrome& widget, int maxWidth, SDL_Color color) const;
    void RebuildWidgetItems(SDL_Renderer* renderer, WidgetChrome& widget, int maxWidth, SDL_Color color) const;

    mutable HeroChrome hero_;
    mutable std::vector<BranchChrome> branches_;
    mutable std::vector<WidgetChrome> widgets_;
    mutable SearchChrome search_{};

    TTF_Font* heroBodyFont_ = nullptr;
    TTF_Font* tileBodyFont_ = nullptr;
};

} // namespace colony::ui::panels

