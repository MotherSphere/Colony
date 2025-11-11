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
    SDL_Color libraryBackground{};
    SDL_Color libraryCard{};
    SDL_Color libraryCardHover{};
    SDL_Color libraryCardActive{};
    SDL_Color navText{};
    SDL_Color heroTitle{};
    SDL_Color heroBody{};
    SDL_Color muted{};
    SDL_Color border{};
    SDL_Color statusBar{};
    SDL_Color statusBarText{};
    SDL_Color channelBadge{};
    SDL_Color heroGradientFallbackStart{};
    SDL_Color heroGradientFallbackEnd{};
};

struct ColorScheme
{
    std::string id;
    std::string name;
    ThemeColors colors;
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
