#pragma once

#include "views/view.hpp"

#include <vector>

namespace colony
{

class MediaView : public View
{
  public:
    explicit MediaView(std::string id);

    std::string_view Id() const noexcept override;
    void BindContent(const ViewContent& content) override;
    void Activate(const RenderContext& context) override;
    void Deactivate() override;
    void Render(const RenderContext& context, const SDL_Rect& bounds) override;
    void OnPrimaryAction(std::string& statusBuffer) override;
    [[nodiscard]] std::optional<SDL_Rect> PrimaryActionRect() const override;

  private:
    struct MediaEntry
    {
        TextTexture title;
        TextTexture description;
    };

    std::string id_;
    ViewContent content_;
    TextTexture headingTexture_;
    TextTexture taglineTexture_;
    TextTexture actionTexture_;
    std::vector<MediaEntry> entries_;
    mutable std::optional<SDL_Rect> lastActionRect_;
};

} // namespace colony
