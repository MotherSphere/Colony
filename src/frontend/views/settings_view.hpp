#pragma once

#include "views/view.hpp"

#include <optional>
#include <string>
#include <vector>

namespace colony::frontend::views
{

class SettingsView : public colony::View
{
  public:
    explicit SettingsView(std::string id);

    std::string_view Id() const noexcept override;
    void BindContent(const colony::ViewContent& content) override;
    void Activate(const colony::RenderContext& context) override;
    void Deactivate() override;
    void Render(const colony::RenderContext& context, const SDL_Rect& bounds) override;
    void OnPrimaryAction(std::string& statusBuffer) override;
    [[nodiscard]] std::optional<SDL_Rect> PrimaryActionRect() const override;

  private:
    struct NavLink
    {
        std::string id;
        colony::TextTexture label;
        SDL_Rect rect{0, 0, 0, 0};
        int targetOffset = 0;
    };

    struct SectionLabel
    {
        std::string id;
        colony::TextTexture label;
    };

    struct SectionAnchor
    {
        std::string id;
        int offsetY = 0;
    };

    struct ToggleRow
    {
        std::string id;
        colony::TextTexture label;
        colony::TextTexture description;
        bool prominent = false;
        SDL_Rect rect{0, 0, 0, 0};
    };

    std::string id_;
    colony::ViewContent content_;
    colony::TextTexture headingTexture_;
    colony::TextTexture taglineTexture_;
    std::vector<NavLink> navLinks_;
    std::vector<SectionLabel> sectionLabels_;
    std::vector<SectionAnchor> anchors_;
    std::vector<ToggleRow> toggles_;
    std::optional<SDL_Rect> primaryActionRect_;
    int lastLayoutWidth_ = 0;
};

} // namespace colony::frontend::views
