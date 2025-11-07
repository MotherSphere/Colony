#pragma once

#include "views/view.hpp"

#include <vector>

namespace colony
{

class ChartView : public View
{
  public:
    explicit ChartView(std::string id);

    std::string_view Id() const noexcept override;
    void BindContent(const ViewContent& content) override;
    void Activate(const RenderContext& context) override;
    void Deactivate() override;
    void Render(const RenderContext& context, const SDL_Rect& bounds) override;
    void OnPrimaryAction(std::string& statusBuffer) override;
    [[nodiscard]] std::optional<SDL_Rect> PrimaryActionRect() const override;

  private:
    struct BarEntry
    {
        TextTexture label;
        TextTexture valueText;
        double value = 0.0;
        double normalized = 0.0;
    };

    std::string id_;
    ViewContent content_;
    TextTexture headingTexture_;
    TextTexture actionTexture_;
    std::vector<BarEntry> entries_;
    mutable std::optional<SDL_Rect> lastActionRect_;
};

} // namespace colony
