#include "frontend/views/dashboard_page.hpp"

#include "ui/layout.hpp"

#include <algorithm>

namespace colony::frontend::views
{

DashboardLayout DashboardPage::Compute(const SDL_Rect& bounds, int detailWidth, int topBarHeight, int gutter) const
{
    DashboardLayout layout;
    layout.topBar = SDL_Rect{bounds.x, bounds.y, bounds.w, topBarHeight};
    layout.body = SDL_Rect{bounds.x, bounds.y + topBarHeight + gutter, bounds.w, bounds.h - topBarHeight - gutter};
    const int clampedDetailWidth = std::clamp(detailWidth, colony::ui::Scale(240), std::max(colony::ui::Scale(320), bounds.w / 2));
    layout.detailArea = SDL_Rect{
        bounds.x + bounds.w - clampedDetailWidth,
        layout.body.y,
        clampedDetailWidth,
        layout.body.h};
    layout.libraryArea = SDL_Rect{
        bounds.x,
        layout.body.y,
        bounds.w - clampedDetailWidth - gutter,
        layout.body.h};
    return layout;
}

} // namespace colony::frontend::views
