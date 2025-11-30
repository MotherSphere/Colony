#pragma once

#include <SDL2/SDL.h>

#include "input/input_router.h"

namespace colony
{
class Application;
}

namespace colony::input
{

class HubInputHandler
{
  public:
    explicit HubInputHandler(Application& app);
    void Register(InputRouter& router);

  private:
    bool HandleMouseButtonDown(const SDL_Event& event, bool& running);
    bool HandleMouseMotion(const SDL_Event& event, bool& running);
    bool HandleMouseWheel(const SDL_Event& event, bool& running);
    bool HandleKeyDown(const SDL_Event& event, bool& running);
    bool HandleTextInput(const SDL_Event& event, bool& running);

    Application& app_;
};

class DialogInputHandler
{
  public:
    explicit DialogInputHandler(Application& app);
    void Register(InputRouter& router);

  private:
    bool HandleMouseButtonDown(const SDL_Event& event, bool& running);
    bool HandleMouseWheel(const SDL_Event& event, bool& running);
    bool HandleKeyDown(const SDL_Event& event, bool& running);
    bool HandleTextInput(const SDL_Event& event, bool& running);

    Application& app_;
};

class NavigationInputHandler
{
  public:
    explicit NavigationInputHandler(Application& app);
    void Register(InputRouter& router);

  private:
    bool HandleMouseButtonDown(const SDL_Event& event, bool& running);
    bool HandleMouseButtonUp(const SDL_Event& event, bool& running);
    bool HandleMouseMotion(const SDL_Event& event, bool& running);
    bool HandleKeyDown(const SDL_Event& event, bool& running);
    bool HandleQuit(const SDL_Event& event, bool& running);

    Application& app_;
};

class LibraryInputHandler
{
  public:
    explicit LibraryInputHandler(Application& app);
    void Register(InputRouter& router);

  private:
    bool HandleMouseButtonDown(const SDL_Event& event, bool& running);
    bool HandleMouseButtonUp(const SDL_Event& event, bool& running);
    bool HandleMouseMotion(const SDL_Event& event, bool& running);
    bool HandleMouseWheel(const SDL_Event& event, bool& running);
    bool HandleMouseRightClick(const SDL_Event& event, bool& running);
    bool HandleKeyDown(const SDL_Event& event, bool& running);
    bool HandleTextInput(const SDL_Event& event, bool& running);

    Application& app_;
};

} // namespace colony::input
