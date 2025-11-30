#include "ui/dialogs/custom_theme_dialog.hpp"

#include "app/application.h"

namespace colony::ui::dialogs
{

CustomThemeDialog::CustomThemeDialog(Application& application)
    : app_(application)
{}

void CustomThemeDialog::Show()
{
    app_.ShowCustomThemeDialog();
}

void CustomThemeDialog::Hide()
{
    app_.HideCustomThemeDialog();
}

void CustomThemeDialog::Render(double timeSeconds)
{
    app_.RenderCustomThemeDialog(timeSeconds);
}

bool CustomThemeDialog::HandleMouseClick(int x, int y)
{
    return app_.HandleCustomThemeDialogMouseClick(x, y);
}

bool CustomThemeDialog::HandleMouseWheel(const SDL_MouseWheelEvent& wheel)
{
    return app_.HandleCustomThemeDialogMouseWheel(wheel);
}

bool CustomThemeDialog::HandleKey(SDL_Keycode key)
{
    return app_.HandleCustomThemeDialogKey(key);
}

bool CustomThemeDialog::HandleTextInput(const SDL_TextInputEvent& text)
{
    return app_.HandleCustomThemeDialogText(text);
}

bool CustomThemeDialog::Apply()
{
    return app_.ApplyCustomThemeDialog();
}

void CustomThemeDialog::EnsureFieldVisible(int focusIndex)
{
    app_.EnsureCustomThemeFieldVisible(focusIndex);
}

} // namespace colony::ui::dialogs
