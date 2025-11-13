#pragma once

#include "ui/theme.hpp"
#include "utils/text.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <string>
#include <string_view>
#include <vector>

namespace colony::frontend::components
{

class BrandCard
{
  public:
    struct Content
    {
        std::string id;
        std::string title;
        std::string subtitle;
        std::string category;
        std::string metric;
        std::string statusLabel;
        std::string primaryActionLabel;
        std::string secondaryActionLabel;
        std::string metricBadgeLabel;
        std::vector<std::string> highlights;
        SDL_Color accent{255, 255, 255, SDL_ALPHA_OPAQUE};
        bool ready = false;
    };

    void Build(
        SDL_Renderer* renderer,
        const Content& content,
        TTF_Font* titleFont,
        TTF_Font* subtitleFont,
        TTF_Font* labelFont,
        const colony::ui::ThemeColors& theme);

    SDL_Rect Render(
        SDL_Renderer* renderer,
        const colony::ui::ThemeColors& theme,
        const colony::ui::InteractionColors& interactions,
        const SDL_Rect& bounds,
        TTF_Font* buttonFont,
        TTF_Font* labelFont,
        bool hovered,
        bool active,
        double timeSeconds) const;

    [[nodiscard]] const std::string& Id() const noexcept { return content_.id; }

  private:
    struct HighlightChip
    {
        std::string label;
        colony::TextTexture texture;
    };

    Content content_{};
    colony::TextTexture titleTexture_;
    colony::TextTexture subtitleTexture_;
    colony::TextTexture categoryTexture_;
    colony::TextTexture metricTexture_;
    std::vector<HighlightChip> highlightChips_;
};

} // namespace colony::frontend::components
