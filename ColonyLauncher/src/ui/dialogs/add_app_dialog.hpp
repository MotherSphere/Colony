#pragma once

#include "ui/dialogs/dialog_base.hpp"

#include <SDL2/SDL.h>

namespace colony
{
class Application;
}

namespace colony::ui::dialogs
{

class AddAppDialog : public DialogBase
{
  public:
    explicit AddAppDialog(Application& application);

    void Show();
    void Hide();
    void RefreshEntries();
    void Render(double timeSeconds);
    bool HandleMouseClick(int x, int y);
    bool HandleMouseWheel(const SDL_MouseWheelEvent& wheel);
    bool HandleKey(SDL_Keycode key);

  private:
    Application& app_;
};

} // namespace colony::ui::dialogs
