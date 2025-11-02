#include "views/simple_text_view.hpp"

#include "utils/text_wrapping.hpp"

#include <algorithm>

namespace colony
{

namespace
{
constexpr int kParagraphSpacing = 16;
constexpr int kLineSpacingFallback = 6;
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
    paragraphLines_.clear();
    lastLayoutWidth_ = 0;

    actionTexture_ = CreateTextTexture(
        context.renderer,
        context.buttonFont,
        content_.primaryActionLabel,
        context.accentColor);
}

void SimpleTextView::Deactivate()
{
    headingTexture_ = {};
    paragraphLines_.clear();
    actionTexture_ = {};
    lastActionRect_.reset();
    lastLayoutWidth_ = 0;
}

void SimpleTextView::Render(const RenderContext& context, const SDL_Rect& bounds)
{
    if (bounds.w > 0 && bounds.w != lastLayoutWidth_)
    {
        RebuildParagraphTextures(context, bounds.w);
    }

    int cursorY = bounds.y;

    if (headingTexture_.texture)
    {
        SDL_Rect headingRect{bounds.x, cursorY, headingTexture_.width, headingTexture_.height};
        RenderTexture(context.renderer, headingTexture_, headingRect);
        cursorY += headingRect.h + kHeadingBottomSpacing;
    }

    const int baseLineSkip = context.paragraphFont != nullptr ? TTF_FontLineSkip(context.paragraphFont) : 0;

    for (std::size_t paragraphIndex = 0; paragraphIndex < paragraphLines_.size(); ++paragraphIndex)
    {
        const auto& lines = paragraphLines_[paragraphIndex];
        for (std::size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
        {
            const auto& lineTexture = lines[lineIndex];
            SDL_Rect paragraphRect{bounds.x, cursorY, lineTexture.width, lineTexture.height};
            RenderTexture(context.renderer, lineTexture, paragraphRect);
            cursorY += paragraphRect.h;

            if (lineIndex + 1 < lines.size())
            {
                const int spacing = baseLineSkip > 0 ? std::max(0, baseLineSkip - lineTexture.height) : kLineSpacingFallback;
                cursorY += spacing;
            }
        }

        if (!lines.empty() && paragraphIndex + 1 < paragraphLines_.size())
        {
            cursorY += kParagraphSpacing;
        }
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

void SimpleTextView::RebuildParagraphTextures(const RenderContext& context, int maxWidth)
{
    paragraphLines_.clear();
    lastLayoutWidth_ = maxWidth;

    if (context.paragraphFont == nullptr || context.renderer == nullptr || maxWidth <= 0)
    {
        return;
    }

    const int lineSkip = TTF_FontLineSkip(context.paragraphFont);
    paragraphLines_.reserve(content_.paragraphs.size());
    for (const auto& paragraph : content_.paragraphs)
    {
        std::vector<TextTexture> lineTextures;
        const auto wrappedLines = WrapTextToWidth(context.paragraphFont, paragraph, maxWidth);
        lineTextures.reserve(wrappedLines.size());
        for (const auto& line : wrappedLines)
        {
            if (line.empty())
            {
                TextTexture placeholder{};
                placeholder.width = 0;
                placeholder.height = std::max(lineSkip, 0);
                lineTextures.emplace_back(std::move(placeholder));
                continue;
            }
            lineTextures.emplace_back(CreateTextTexture(context.renderer, context.paragraphFont, line, context.mutedColor));
        }
        paragraphLines_.emplace_back(std::move(lineTextures));
    }
}

} // namespace colony
