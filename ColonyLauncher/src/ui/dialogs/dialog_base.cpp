#include "ui/dialogs/dialog_base.hpp"

namespace colony::ui::dialogs
{

void DialogBase::Open()
{
    if (open_)
    {
        return;
    }
    open_ = true;
    OnOpen();
}

void DialogBase::Close()
{
    if (!open_)
    {
        return;
    }
    open_ = false;
    OnClose();
}

void DialogBase::TriggerSubmit()
{
    if (onSubmit_)
    {
        onSubmit_();
    }
}

void DialogBase::TriggerCancel()
{
    if (onCancel_)
    {
        onCancel_();
    }
}

} // namespace colony::ui::dialogs
