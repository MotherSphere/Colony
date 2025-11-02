#pragma once

#include "views/view.hpp"

namespace colony
{

class SimpleTextView : public View
{
  public:
    explicit SimpleTextView(std::string id);

    std::string_view Id() const noexcept override;
    void BindContent(const ViewContent& content) override;
    void Activate(const RenderContext& context) override;
    void Deactivate() override;
    void Render(const RenderContext& context, const SDL_Rect& bounds) override;
    void OnPrimaryAction(std::string& statusBuffer) override;
    [[nodiscard]] std::optional<SDL_Rect> PrimaryActionRect() const override;

  private:
    std::string id_;
    ViewContent content_;
    void RebuildParagraphTextures(const RenderContext& context, int maxWidth);

    TextTexture headingTexture_;
    std::vector<std::vector<TextTexture>> paragraphLines_;
    TextTexture actionTexture_;
    mutable std::optional<SDL_Rect> lastActionRect_;
    mutable int lastLayoutWidth_ = 0;
};

} // namespace colony
