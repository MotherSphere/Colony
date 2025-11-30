#pragma once

#include "utils/sdl_wrappers.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

namespace colony::platform
{

struct RendererDimensions
{
    int width = 0;
    int height = 0;
};

class RendererHost
{
  public:
    RendererHost() = default;
    RendererHost(const RendererHost&) = delete;
    RendererHost& operator=(const RendererHost&) = delete;

    [[nodiscard]] bool Init(const char* windowTitle, int width, int height);
    void Shutdown();

    [[nodiscard]] SDL_Renderer* Renderer() const noexcept { return renderer_.get(); }
    [[nodiscard]] SDL_Window* Window() const noexcept { return window_.get(); }
    [[nodiscard]] RendererDimensions OutputSize() const noexcept;

  private:
    bool initialized_ = false;
    sdl::WindowHandle window_{};
    sdl::RendererHandle renderer_{};
};

} // namespace colony::platform
