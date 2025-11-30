#include "ui/dialogs/edit_user_app_dialog.hpp"

#include "app/application.h"

namespace colony::ui::dialogs
{

EditUserAppDialog::EditUserAppDialog(Application& application)
    : app_(application)
{}

void EditUserAppDialog::Show(const std::string& programId)
{
    app_.ShowEditUserAppDialog(programId);
}

void EditUserAppDialog::Hide()
{
    app_.HideEditUserAppDialog();
}

void EditUserAppDialog::Render(double timeSeconds)
{
    app_.RenderEditUserAppDialog(timeSeconds);
}

bool EditUserAppDialog::HandleMouseClick(int x, int y)
{
    return app_.HandleEditUserAppDialogMouseClick(x, y);
}

bool EditUserAppDialog::HandleKey(SDL_Keycode key)
{
    return app_.HandleEditUserAppDialogKey(key);
}

bool EditUserAppDialog::HandleTextInput(const SDL_TextInputEvent& text)
{
    return app_.HandleEditUserAppDialogText(text);
}

bool EditUserAppDialog::ApplyChanges()
{
    return app_.ApplyEditUserAppChanges();
}

} // namespace colony::ui::dialogs
