#include "views/view_registry.hpp"

#include <stdexcept>

namespace colony
{

void ViewRegistry::Register(ViewPtr view)
{
    if (!view)
    {
        throw std::invalid_argument("Cannot register null view");
    }
    const std::string id{view->Id()};
    views_[id] = std::move(view);
}

void ViewRegistry::BindContent(const AppContent& content)
{
    for (const auto& [id, viewContent] : content.views)
    {
        auto it = views_.find(id);
        if (it == views_.end())
        {
            continue;
        }
        it->second->BindContent(viewContent);
    }
}

View* ViewRegistry::Activate(std::string_view id, const RenderContext& context)
{
    DeactivateActive();

    const std::string key{id};
    auto it = views_.find(key);
    if (it == views_.end())
    {
        active_ = nullptr;
        return nullptr;
    }

    it->second->Activate(context);
    active_ = it->second.get();
    return active_;
}

void ViewRegistry::DeactivateActive()
{
    if (active_ != nullptr)
    {
        active_->Deactivate();
        active_ = nullptr;
    }
}

void ViewRegistry::RenderActive(const RenderContext& context, const SDL_Rect& bounds) const
{
    if (active_ != nullptr)
    {
        active_->Render(context, bounds);
    }
}

void ViewRegistry::TriggerPrimaryAction(std::string& statusBuffer) const
{
    if (active_ != nullptr)
    {
        active_->OnPrimaryAction(statusBuffer);
    }
}

std::optional<SDL_Rect> ViewRegistry::PrimaryActionRect() const
{
    if (active_ != nullptr)
    {
        return active_->PrimaryActionRect();
    }
    return std::nullopt;
}

} // namespace colony
