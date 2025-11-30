#include "nexus/nexus_main.hpp"

#include <SDL2/SDL.h>

#include <cstdlib>
#include <iostream>
#include <string>

namespace nexus
{
namespace
{
std::string ResolveModuleName()
{
    if (const char* envName = std::getenv("COLONY_MODULE_NAME"))
    {
        return envName;
    }

    return "Nexus";
}
} // namespace

NexusResult LaunchStandalone()
{
    NexusResult result{};

    const std::string moduleName = ResolveModuleName();

    SDL_Window* moduleWindow = SDL_CreateWindow(
        moduleName.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (moduleWindow == nullptr)
    {
        std::cerr << "Unable to create module window: " << SDL_GetError() << '\n';
        return result;
    }

    SDL_Renderer* moduleRenderer = SDL_CreateRenderer(
        moduleWindow,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (moduleRenderer == nullptr)
    {
        std::cerr << "Unable to create module renderer: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(moduleWindow);
        return result;
    }

    const Uint32 moduleWindowId = SDL_GetWindowID(moduleWindow);
    bool running = true;

    while (running)
    {
        SDL_Event event{};
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                result.propagateQuit = true;
                running = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    running = false;
                }
                break;
            case SDL_WINDOWEVENT:
                if (event.window.windowID == moduleWindowId && event.window.event == SDL_WINDOWEVENT_CLOSE)
                {
                    running = false;
                }
                break;
            default:
                break;
            }
        }

        SDL_SetRenderDrawColor(moduleRenderer, 6, 10, 26, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(moduleRenderer);
        SDL_RenderPresent(moduleRenderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(moduleRenderer);
    SDL_DestroyWindow(moduleWindow);

    return result;
}
} // namespace nexus

