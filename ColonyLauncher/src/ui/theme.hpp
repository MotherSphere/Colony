#pragma once

#include <SDL2/SDL.h>

#include <string>
#include <string_view>
#include <vector>

namespace colony::ui
{

struct ThemeColors
{
    SDL_Color background{};
    SDL_Color navRail{};
    SDL_Color surface{};
    SDL_Color surfaceAlt{};
    SDL_Color elevatedSurface{};
    SDL_Color card{};
    SDL_Color cardHover{};
    SDL_Color cardActive{};
    SDL_Color divider{};
    SDL_Color navText{};
    SDL_Color navTextMuted{};
    SDL_Color heroTitle{};
    SDL_Color heroBody{};
    SDL_Color muted{};
    SDL_Color border{};
    SDL_Color statusBar{};
    SDL_Color statusBarText{};
    SDL_Color channelBadge{};
    SDL_Color chipBackground{};
    SDL_Color inputBackground{};
    SDL_Color inputBorder{};
    SDL_Color inputPlaceholder{};
    SDL_Color buttonPrimary{};
    SDL_Color buttonPrimaryHover{};
    SDL_Color buttonPrimaryActive{};
    SDL_Color buttonGhost{};
    SDL_Color buttonGhostHover{};
    SDL_Color focusRing{};
    SDL_Color success{};
    SDL_Color warning{};
    SDL_Color info{};
    SDL_Color overlay{};
    SDL_Color libraryBackground{};
    SDL_Color libraryCard{};
    SDL_Color libraryCardHover{};
    SDL_Color libraryCardActive{};
    SDL_Color heroGradientFallbackStart{};
    SDL_Color heroGradientFallbackEnd{};
};

struct InteractionColors
{
    SDL_Color hover{};
    SDL_Color active{};
    SDL_Color focus{};
    SDL_Color subtleGlow{};
};

struct TypographyStyle
{
    int size = 0;
    float lineHeight = 1.0f;
    SDL_Color color{};
};

struct Typography
{
    TypographyStyle display;
    TypographyStyle headline;
    TypographyStyle title;
    TypographyStyle subtitle;
    TypographyStyle body;
    TypographyStyle label;
    TypographyStyle caption;
};

struct MotionTimings
{
    float fast = 0.12f;
    float standard = 0.24f;
    float slow = 0.36f;
    float hoverLift = 6.0f;
    float focusScale = 1.03f;
};

struct ColorScheme
{
    std::string id;
    std::string name;
    ThemeColors colors;
    InteractionColors interactions;
    Typography typography;
    MotionTimings motion;
    bool isCustom = false;
};

class ThemeManager
{
  public:
    ThemeManager();

    [[nodiscard]] const ColorScheme& ActiveScheme() const noexcept { return *active_; }
    [[nodiscard]] const std::vector<ColorScheme>& Schemes() const noexcept { return schemes_; }

    const ColorScheme& AddCustomScheme(ColorScheme scheme, bool makeActive = false);
    bool SetActiveScheme(std::string_view id);

  private:
    std::vector<ColorScheme> schemes_;
    const ColorScheme* active_ = nullptr;
};

} // namespace colony::ui
