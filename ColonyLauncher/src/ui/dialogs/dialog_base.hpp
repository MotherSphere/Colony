#pragma once

#include <functional>

namespace colony::ui::dialogs
{

class DialogBase
{
  public:
    using Callback = std::function<void()>;
    virtual ~DialogBase() = default;

    void Open();
    void Close();
    [[nodiscard]] bool IsOpen() const { return open_; }

    void SetOnSubmit(Callback callback) { onSubmit_ = std::move(callback); }
    void SetOnCancel(Callback callback) { onCancel_ = std::move(callback); }

  protected:
    [[nodiscard]] bool Validate() const { return true; }
    virtual void OnOpen() {}
    virtual void OnClose() {}

    void TriggerSubmit();
    void TriggerCancel();

    bool open_ = false;
    int focusedIndex_ = -1;

  private:
    Callback onSubmit_{};
    Callback onCancel_{};
};

} // namespace colony::ui::dialogs
