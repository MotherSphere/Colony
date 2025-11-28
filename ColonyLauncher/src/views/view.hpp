#pragma once

#include "core/content.hpp"
#include "utils/text.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace colony
{

struct RenderContext
{
    SDL_Renderer* renderer{};
    TTF_Font* headingFont{};
    TTF_Font* paragraphFont{};
    TTF_Font* buttonFont{};
    SDL_Color primaryColor{};
    SDL_Color mutedColor{};
    SDL_Color accentColor{};
};

class View
{
  public:
    virtual ~View() = default;

    virtual std::string_view Id() const noexcept = 0;
    virtual void BindContent(const ViewContent& content) = 0;
    virtual void Activate(const RenderContext& context) = 0;
    virtual void Deactivate() = 0;
    virtual void Render(const RenderContext& context, const SDL_Rect& bounds) = 0;
    virtual void OnPrimaryAction(std::string& statusBuffer) = 0;
    [[nodiscard]] virtual std::optional<SDL_Rect> PrimaryActionRect() const = 0;
};

using ViewPtr = std::unique_ptr<View>;
using ViewCollection = std::vector<ViewPtr>;

} // namespace colony
