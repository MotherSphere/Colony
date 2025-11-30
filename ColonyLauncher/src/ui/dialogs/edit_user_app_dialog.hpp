#pragma once

#include "ui/dialogs/dialog_base.hpp"

#include <SDL2/SDL.h>

namespace colony
{
class Application;
}

namespace colony::ui::dialogs
{

class EditUserAppDialog : public DialogBase
{
  public:
    explicit EditUserAppDialog(Application& application);

    void Show(const std::string& programId);
    void Hide();
    void Render(double timeSeconds);
    bool HandleMouseClick(int x, int y);
    bool HandleKey(SDL_Keycode key);
    bool HandleTextInput(const SDL_TextInputEvent& text);
    bool ApplyChanges();

  private:
    Application& app_;
};

} // namespace colony::ui::dialogs
