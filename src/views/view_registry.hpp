#pragma once

#include "core/content.hpp"
#include "views/view.hpp"

#include <optional>
#include <string>
#include <unordered_map>

namespace colony
{

class ViewRegistry
{
  public:
    void Register(ViewPtr view);
    void BindContent(const AppContent& content);

    View* Activate(std::string_view id, const RenderContext& context);
    void DeactivateActive();
    [[nodiscard]] View* ActiveView() const noexcept { return active_; }

    void RenderActive(const RenderContext& context, const SDL_Rect& bounds) const;
    void TriggerPrimaryAction(std::string& statusBuffer) const;
    [[nodiscard]] std::optional<SDL_Rect> PrimaryActionRect() const;

  private:
    std::unordered_map<std::string, ViewPtr> views_;
    View* active_ = nullptr;
};

} // namespace colony
