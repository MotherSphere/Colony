#include "platform/renderer_host.hpp"

#include <iostream>

namespace colony::platform
{
namespace
{
bool SDLCallSucceeded(int result)
{
    if (result < 0)
    {
        std::cerr << "SDL error: " << SDL_GetError() << '\n';
        return false;
    }
    return true;
}
} // namespace

bool RendererHost::Init(const char* windowTitle, int width, int height)
{
    if (initialized_)
    {
        Shutdown();
    }

    if (!SDLCallSucceeded(SDL_Init(SDL_INIT_VIDEO)))
    {
        return false;
    }

    if (TTF_Init() == -1)
    {
        std::cerr << "Failed to initialize SDL_ttf: " << TTF_GetError() << '\n';
        SDL_Quit();
        return false;
    }

    window_ = sdl::WindowHandle{SDL_CreateWindow(
        windowTitle,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE)};
    if (!window_)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << '\n';
        Shutdown();
        return false;
    }

    renderer_ = sdl::RendererHandle{SDL_CreateRenderer(
        window_.get(),
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE)};
    if (!renderer_)
    {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << '\n';
        Shutdown();
        return false;
    }

    initialized_ = true;
    return true;
}

void RendererHost::Shutdown()
{
    renderer_.reset();
    window_.reset();

    if (initialized_)
    {
        TTF_Quit();
        SDL_Quit();
    }

    initialized_ = false;
}

RendererDimensions RendererHost::OutputSize() const noexcept
{
    RendererDimensions dimensions{};
    if (renderer_)
    {
        SDL_GetRendererOutputSize(renderer_.get(), &dimensions.width, &dimensions.height);
    }
    return dimensions;
}

} // namespace colony::platform
