#include "views/simple_text_view.hpp"

#include <algorithm>

namespace colony
{

namespace
{
constexpr int kParagraphSpacing = 16;
constexpr int kHeadingBottomSpacing = 32;
constexpr int kActionTopSpacing = 48;
constexpr int kActionWidth = 220;
constexpr int kActionHeight = 60;
}

SimpleTextView::SimpleTextView(std::string id) : id_(std::move(id)) {}

std::string_view SimpleTextView::Id() const noexcept
{
    return id_;
}

void SimpleTextView::BindContent(const ViewContent& content)
{
    content_ = content;
}

void SimpleTextView::Activate(const RenderContext& context)
{
    headingTexture_ = CreateTextTexture(context.renderer, context.headingFont, content_.heading, context.primaryColor);
    paragraphTextures_.clear();
    paragraphTextures_.reserve(content_.paragraphs.size());
    for (const auto& paragraph : content_.paragraphs)
    {
        paragraphTextures_.emplace_back(CreateTextTexture(context.renderer, context.paragraphFont, paragraph, context.mutedColor));
    }

    actionTexture_ = CreateTextTexture(
        context.renderer,
        context.buttonFont,
        content_.primaryActionLabel,
        context.accentColor);
}

void SimpleTextView::Deactivate()
{
    headingTexture_ = {};
    paragraphTextures_.clear();
    actionTexture_ = {};
    lastActionRect_.reset();
}

void SimpleTextView::Render(const RenderContext& context, const SDL_Rect& bounds)
{
    int cursorY = bounds.y;

    if (headingTexture_.texture)
    {
        SDL_Rect headingRect{bounds.x, cursorY, headingTexture_.width, headingTexture_.height};
        RenderTexture(context.renderer, headingTexture_, headingRect);
        cursorY += headingRect.h + kHeadingBottomSpacing;
    }

    for (const auto& paragraph : paragraphTextures_)
    {
        SDL_Rect paragraphRect{bounds.x, cursorY, paragraph.width, paragraph.height};
        RenderTexture(context.renderer, paragraph, paragraphRect);
        cursorY += paragraphRect.h + kParagraphSpacing;
    }

    cursorY += kActionTopSpacing;

    SDL_Rect buttonRect{bounds.x, cursorY, kActionWidth, kActionHeight};
    SDL_SetRenderDrawColor(context.renderer, 245, 245, 245, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(context.renderer, &buttonRect);
    SDL_SetRenderDrawColor(context.renderer, context.accentColor.r, context.accentColor.g, context.accentColor.b, context.accentColor.a);
    SDL_RenderDrawRect(context.renderer, &buttonRect);

    if (actionTexture_.texture)
    {
        SDL_Rect textRect{
            buttonRect.x + (buttonRect.w - actionTexture_.width) / 2,
            buttonRect.y + (buttonRect.h - actionTexture_.height) / 2,
            actionTexture_.width,
            actionTexture_.height};
        RenderTexture(context.renderer, actionTexture_, textRect);
    }

    lastActionRect_ = buttonRect;
}

void SimpleTextView::OnPrimaryAction(std::string& statusBuffer)
{
    statusBuffer = content_.statusMessage;
}

std::optional<SDL_Rect> SimpleTextView::PrimaryActionRect() const
{
    return lastActionRect_;
}

} // namespace colony
