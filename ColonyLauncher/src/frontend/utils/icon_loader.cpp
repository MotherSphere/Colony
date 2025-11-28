#include "frontend/utils/icon_loader.hpp"

#include "ui/layout.hpp"
#include "utils/color.hpp"
#include "utils/drawing.hpp"
#include "utils/asset_paths.hpp"

#include <SDL2/SDL.h>

#include <array>
#include <cmath>
#include <filesystem>
#include <numbers>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>

namespace colony::frontend::icons
{
namespace
{
using IconCache = std::unordered_map<std::string, IconTexture>;
constexpr std::string_view kIconDirectory = "assets/icons";

SDL_Color Mix(SDL_Color a, SDL_Color b, float factor)
{
    return colony::color::Mix(a, b, factor);
}

void PaintDashboardIcon(SDL_Renderer* renderer, SDL_Color accent, SDL_Color base, int size)
{
    SDL_Color frame = Mix(accent, base, 0.4f);
    SDL_SetRenderDrawColor(renderer, frame.r, frame.g, frame.b, 180);
    const int corner = std::max(4, size / 6);
    SDL_Rect rect{0, 0, size, size};
    colony::drawing::RenderRoundedRect(renderer, rect, corner);

    SDL_Color tile = accent;
    tile.a = 220;
    const int padding = std::max(2, size / 8);
    const int tileSize = (size - padding * 3) / 2;
    for (int row = 0; row < 2; ++row)
    {
        for (int col = 0; col < 2; ++col)
        {
            SDL_Rect tileRect{
                padding + col * (tileSize + padding),
                padding + row * (tileSize + padding),
                tileSize,
                tileSize};
            SDL_SetRenderDrawColor(renderer, tile.r, tile.g, tile.b, tile.a);
            colony::drawing::RenderFilledRoundedRect(renderer, tileRect, corner / 2);
        }
    }
}

void PaintBrandsIcon(SDL_Renderer* renderer, SDL_Color accent, SDL_Color base, int size)
{
    const int circleSize = size - std::max(4, size / 5);
    const int offset = (size - circleSize) / 2;
    SDL_Rect circleRect{offset, offset, circleSize, circleSize};

    SDL_Color outer = Mix(accent, base, 0.25f);
    SDL_SetRenderDrawColor(renderer, outer.r, outer.g, outer.b, 200);
    colony::drawing::RenderFilledRoundedRect(renderer, circleRect, circleSize / 2);

    SDL_Color inner = Mix(accent, base, 0.55f);
    SDL_Rect innerRect{circleRect.x + circleRect.w / 4, circleRect.y + circleRect.h / 4, circleRect.w / 2, circleRect.h / 2};
    colony::drawing::RenderFilledRoundedRect(renderer, innerRect, innerRect.w / 2);

    SDL_Color dot = accent;
    dot.a = 255;
    SDL_Rect dotRect{circleRect.x + circleRect.w / 2 - circleRect.w / 8, circleRect.y + circleRect.h / 2 - circleRect.h / 8, circleRect.w / 4, circleRect.h / 4};
    colony::drawing::RenderFilledRoundedRect(renderer, dotRect, dotRect.w / 2);
}

void PaintSalesIcon(SDL_Renderer* renderer, SDL_Color accent, SDL_Color base, int size)
{
    SDL_Color arrow = accent;
    arrow.a = 230;
    const int thickness = std::max(2, size / 6);
    SDL_SetRenderDrawColor(renderer, arrow.r, arrow.g, arrow.b, arrow.a);
    SDL_Point points[4];
    points[0] = SDL_Point{size / 6, size - size / 6};
    points[1] = SDL_Point{size / 2, size / 2};
    points[2] = SDL_Point{size - size / 6, size - size / 3};
    points[3] = SDL_Point{size - size / 6, size / 6};

    for (int i = 0; i < 3; ++i)
    {
        SDL_RenderDrawLine(renderer, points[i].x, points[i].y, points[i + 1].x, points[i + 1].y);
    }

    SDL_Rect stem{points[0].x - thickness / 2, points[0].y - thickness, thickness, size / 2};
    colony::drawing::RenderFilledRoundedRect(renderer, stem, thickness / 2);

    SDL_Point arrowHead[3];
    arrowHead[0] = SDL_Point{points[3].x, points[3].y};
    arrowHead[1] = SDL_Point{points[3].x - size / 6, size / 4};
    arrowHead[2] = SDL_Point{points[3].x + size / 6, size / 4};
    SDL_RenderDrawLine(renderer, arrowHead[0].x, arrowHead[0].y, arrowHead[1].x, arrowHead[1].y);
    SDL_RenderDrawLine(renderer, arrowHead[0].x, arrowHead[0].y, arrowHead[2].x, arrowHead[2].y);
}

void PaintSettingsIcon(SDL_Renderer* renderer, SDL_Color accent, SDL_Color base, int size)
{
    SDL_Color ring = Mix(accent, base, 0.4f);
    const int center = size / 2;
    const int outerRadius = size / 2 - 1;
    const int innerRadius = outerRadius - std::max(3, size / 6);

    SDL_SetRenderDrawColor(renderer, ring.r, ring.g, ring.b, 200);
    for (int radius = innerRadius; radius <= outerRadius; ++radius)
    {
        for (int angle = 0; angle < 360; ++angle)
        {
            const double radians = static_cast<double>(angle) * (std::numbers::pi_v<double> / 180.0);
            const int x = center + static_cast<int>(std::cos(radians) * static_cast<double>(radius));
            const int y = center + static_cast<int>(std::sin(radians) * static_cast<double>(radius));
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }

    SDL_Color hub = Mix(accent, base, 0.1f);
    SDL_Rect hubRect{center - innerRadius / 2, center - innerRadius / 2, innerRadius, innerRadius};
    colony::drawing::RenderFilledRoundedRect(renderer, hubRect, innerRadius / 2);
}

void PaintDefaultIcon(SDL_Renderer* renderer, SDL_Color accent, SDL_Color base, int size)
{
    SDL_Color fill = Mix(accent, base, 0.3f);
    SDL_Rect rect{0, 0, size, size};
    colony::drawing::RenderFilledRoundedRect(renderer, rect, size / 2);
    SDL_Color highlight = Mix(accent, base, 0.65f);
    SDL_Rect inner{size / 4, size / 4, size / 2, size / 2};
    colony::drawing::RenderFilledRoundedRect(renderer, inner, inner.w / 2);
}

using Painter = void (*)(SDL_Renderer*, SDL_Color, SDL_Color, int);

Painter ResolvePainter(std::string_view id)
{
    if (id == "dashboard")
    {
        return PaintDashboardIcon;
    }
    if (id == "brands")
    {
        return PaintBrandsIcon;
    }
    if (id == "sales" || id == "analytics")
    {
        return PaintSalesIcon;
    }
    if (id == "settings" || id == "preferences")
    {
        return PaintSettingsIcon;
    }
    return PaintDefaultIcon;
}

std::string MakeCacheKey(std::string_view id, SDL_Color accent)
{
    std::ostringstream stream;
    stream << id << '#' << static_cast<int>(accent.r) << '_' << static_cast<int>(accent.g) << '_' << static_cast<int>(accent.b);
    return stream.str();
}

std::optional<IconTexture> LoadBitmapIcon(SDL_Renderer* renderer, const std::filesystem::path& path)
{
    SDL_Surface* surface = SDL_LoadBMP(path.string().c_str());
    if (surface == nullptr)
    {
        return std::nullopt;
    }

    sdl::TextureHandle texture{SDL_CreateTextureFromSurface(renderer, surface)};
    if (!texture)
    {
        SDL_FreeSurface(surface);
        return std::nullopt;
    }

    SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND);
    IconTexture icon;
    icon.texture = std::move(texture);
    icon.width = surface->w;
    icon.height = surface->h;
    SDL_FreeSurface(surface);
    return icon;
}

std::optional<IconTexture> LoadIconAsset(SDL_Renderer* renderer, std::string_view id)
{
    namespace fs = std::filesystem;
    std::error_code error;
    const fs::path directory = colony::paths::ResolveAssetDirectory(kIconDirectory);
    if (!fs::exists(directory, error))
    {
        return std::nullopt;
    }

    static constexpr std::array<std::string_view, 1> kExtensions{".bmp"};
    for (std::string_view extension : kExtensions)
    {
        fs::path candidate = directory / (std::string{id} + std::string{extension});
        if (!fs::exists(candidate, error))
        {
            continue;
        }

        if (auto icon = LoadBitmapIcon(renderer, candidate))
        {
            return icon;
        }
    }

    return std::nullopt;
}

const IconTexture& CreateIcon(
    IconCache& cache,
    SDL_Renderer* renderer,
    std::string_view id,
    SDL_Color accent,
    const colony::ui::ThemeColors& theme)
{
    const std::string key = MakeCacheKey(id, accent);
    if (const auto it = cache.find(key); it != cache.end())
    {
        return it->second;
    }

    if (auto assetIcon = LoadIconAsset(renderer, id))
    {
        auto [it, _] = cache.emplace(key, std::move(*assetIcon));
        return it->second;
    }

    const int size = colony::ui::Scale(34);
    sdl::TextureHandle texture{
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, size, size)};
    if (!texture)
    {
        static IconTexture empty{};
        return empty;
    }

    SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND);
    SDL_Texture* previousTarget = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, texture.get());
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    Painter painter = ResolvePainter(id);
    painter(renderer, accent, theme.navText, size);

    SDL_SetRenderTarget(renderer, previousTarget);

    IconTexture icon;
    icon.texture = std::move(texture);
    icon.width = size;
    icon.height = size;
    auto [it, _] = cache.emplace(key, std::move(icon));
    return it->second;
}

} // namespace

const IconTexture& LoadSidebarIcon(
    SDL_Renderer* renderer,
    std::string_view id,
    SDL_Color accent,
    const colony::ui::ThemeColors& theme)
{
    static IconCache cache;
    return CreateIcon(cache, renderer, id, accent, theme);
}

} // namespace colony::frontend::icons
