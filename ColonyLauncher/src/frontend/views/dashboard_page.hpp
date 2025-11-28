#pragma once

#include <SDL2/SDL.h>

namespace colony::frontend::views
{

struct DashboardLayout
{
    SDL_Rect topBar{0, 0, 0, 0};
    SDL_Rect body{0, 0, 0, 0};
    SDL_Rect libraryArea{0, 0, 0, 0};
    SDL_Rect detailArea{0, 0, 0, 0};
};

class DashboardPage
{
  public:
    DashboardLayout Compute(const SDL_Rect& bounds, int detailWidth, int topBarHeight, int gutter) const;
};

} // namespace colony::frontend::views
