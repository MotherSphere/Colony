#include "orbital_arcade/arcade_main.hpp"

#include <SDL2/SDL.h>

#include <iostream>

namespace orbital_arcade
{
ArcadeResult LaunchStandalone()
{
    ArcadeResult result{};

    SDL_Window* arcadeWindow = SDL_CreateWindow(
        "Orbital Arcade",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (arcadeWindow == nullptr)
    {
        std::cerr << "Unable to create Orbital Arcade window: " << SDL_GetError() << '\n';
        return result;
    }

    SDL_Renderer* arcadeRenderer = SDL_CreateRenderer(
        arcadeWindow,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (arcadeRenderer == nullptr)
    {
        std::cerr << "Unable to create Orbital Arcade renderer: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(arcadeWindow);
        return result;
    }

    const Uint32 arcadeWindowId = SDL_GetWindowID(arcadeWindow);
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
                if (event.window.windowID == arcadeWindowId && event.window.event == SDL_WINDOWEVENT_CLOSE)
                {
                    running = false;
                }
                break;
            default:
                break;
            }
        }

        SDL_SetRenderDrawColor(arcadeRenderer, 6, 10, 26, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(arcadeRenderer);
        SDL_RenderPresent(arcadeRenderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(arcadeRenderer);
    SDL_DestroyWindow(arcadeWindow);

    return result;
}
} // namespace orbital_arcade

