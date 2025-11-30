#include "ui/dialogs/add_app_dialog.hpp"

#include "app/application.hpp"

namespace colony::ui::dialogs
{

AddAppDialog::AddAppDialog(Application& application)
    : app_(application)
{}

void AddAppDialog::Show()
{
    app_.ShowAddAppDialog();
}

void AddAppDialog::Hide()
{
    app_.HideAddAppDialog();
}

void AddAppDialog::RefreshEntries()
{
    app_.RefreshAddAppDialogEntries();
}

void AddAppDialog::Render(double timeSeconds)
{
    app_.RenderAddAppDialog(timeSeconds);
}

bool AddAppDialog::HandleMouseClick(int x, int y)
{
    return app_.HandleAddAppDialogMouseClick(x, y);
}

bool AddAppDialog::HandleMouseWheel(const SDL_MouseWheelEvent& wheel)
{
    return app_.HandleAddAppDialogMouseWheel(wheel);
}

bool AddAppDialog::HandleKey(SDL_Keycode key)
{
    return app_.HandleAddAppDialogKey(key);
}

} // namespace colony::ui::dialogs
