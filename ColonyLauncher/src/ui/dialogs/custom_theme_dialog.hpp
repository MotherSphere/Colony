#pragma once

#include "ui/dialogs/dialog_base.hpp"

#include <SDL2/SDL.h>

namespace colony
{
class Application;
}

namespace colony::ui::dialogs
{

class CustomThemeDialog : public DialogBase
{
  public:
    explicit CustomThemeDialog(Application& application);

    void Show();
    void Hide();
    void Render(double timeSeconds);
    bool HandleMouseClick(int x, int y);
    bool HandleMouseWheel(const SDL_MouseWheelEvent& wheel);
    bool HandleKey(SDL_Keycode key);
    bool HandleTextInput(const SDL_TextInputEvent& text);
    bool Apply();
    void EnsureFieldVisible(int focusIndex);

  private:
    Application& app_;
};

} // namespace colony::ui::dialogs
