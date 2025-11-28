#pragma once

#include <algorithm>
#include <cmath>

#include "frontend/components/search_field.hpp"
#include "ui/theme.hpp"
#include "utils/text.hpp"

#include <string>

namespace colony::ui
{

inline float& UiScaleStorage()
{
    static float scale = 0.82f;
    return scale;
}

inline void SetUiScale(float scale)
{
    constexpr float kMinScale = 0.6f;
    constexpr float kMaxScale = 1.1f;
    UiScaleStorage() = std::clamp(scale, kMinScale, kMaxScale);
}

inline float GetUiScale() noexcept
{
    return UiScaleStorage();
}

inline int Scale(int value)
{
    if (value <= 0)
    {
        return value;
    }

    const float scale = GetUiScale();
    const int scaled = static_cast<int>(static_cast<float>(value) * scale + 0.5f);
    return scaled < 1 ? 1 : scaled;
}

inline float Scale(float value)
{
    return value * GetUiScale();
}

inline int ScaleDynamic(int value)
{
    if (value <= 0)
    {
        return value;
    }

    const float scale = GetUiScale();
    const int scaled = static_cast<int>(std::lround(static_cast<double>(value) * scale));
    return scaled < 1 ? 1 : scaled;
}

inline float ScaleDynamic(float value)
{
    return static_cast<float>(value * GetUiScale());
}

class TopBar
{
  public:
    void Build(
        SDL_Renderer* renderer,
        TTF_Font* titleFont,
        TTF_Font* bodyFont,
        const ThemeColors& theme,
        const Typography& typography,
        std::string_view searchPlaceholder,
        std::string_view titleText);

    void UpdateTitle(SDL_Renderer* renderer, std::string_view titleText, SDL_Color titleColor);

    struct RenderResult
    {
        SDL_Rect searchFieldRect{0, 0, 0, 0};
        SDL_Rect profileButtonRect{0, 0, 0, 0};
    };

    RenderResult Render(
        SDL_Renderer* renderer,
        const ThemeColors& theme,
        const Typography& typography,
        const InteractionColors& interactions,
        const SDL_Rect& bounds,
        std::string_view searchValue,
        bool searchFocused,
        double timeSeconds) const;

  private:
    TTF_Font* titleFont_ = nullptr;
    SDL_Color titleColor_{255, 255, 255, SDL_ALPHA_OPAQUE};
    std::string currentTitle_;
    colony::TextTexture titleTexture_;
    colony::frontend::components::SearchField searchField_;
};

} // namespace colony::ui
