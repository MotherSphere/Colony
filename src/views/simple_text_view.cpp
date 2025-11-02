#include "views/simple_text_view.hpp"

#include "utils/text_wrapping.hpp"

#include <algorithm>
#include <utility>

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
constexpr int kSectionTopSpacing = 40;
constexpr int kSectionPadding = 20;
constexpr int kSectionSpacing = 28;
constexpr int kSectionTitleSpacing = 12;
constexpr int kOptionSpacing = 12;
constexpr int kBulletIndent = 28;
constexpr int kOptionLineSpacingFallback = 4;
constexpr SDL_Color kCardFillColor{250, 250, 250, SDL_ALPHA_OPAQUE};
constexpr SDL_Color kCardBorderColor{222, 222, 222, SDL_ALPHA_OPAQUE};
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
    sectionRenderData_.clear();
    lastSectionLayoutWidth_ = 0;

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
    sectionRenderData_.clear();
    actionTexture_ = {};
    lastActionRect_.reset();
    lastLayoutWidth_ = 0;
    lastSectionLayoutWidth_ = 0;
}

void SimpleTextView::Render(const RenderContext& context, const SDL_Rect& bounds)
{
    if (bounds.w > 0 && bounds.w != lastLayoutWidth_)
    {
        RebuildParagraphTextures(context, bounds.w);
    }

    if (!content_.sections.empty() && bounds.w > 0 && bounds.w != lastSectionLayoutWidth_)
    {
        RebuildSectionTextures(context, bounds.w);
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

    if (!sectionRenderData_.empty())
    {
        cursorY += kSectionTopSpacing;

        const int optionLineSkip = context.paragraphFont != nullptr ? TTF_FontLineSkip(context.paragraphFont) : 0;

        for (std::size_t sectionIndex = 0; sectionIndex < sectionRenderData_.size(); ++sectionIndex)
        {
            const auto& section = sectionRenderData_[sectionIndex];

            const int cardWidth = bounds.w;
            int cardHeight = 2 * kSectionPadding;

            const bool hasTitle = section.titleTexture.texture != nullptr;
            const bool hasOptions = !section.optionLines.empty();

            if (hasTitle)
            {
                cardHeight += section.titleTexture.height;
            }

            if (hasTitle && hasOptions)
            {
                cardHeight += kSectionTitleSpacing;
            }

            for (std::size_t optionIndex = 0; optionIndex < section.optionLines.size(); ++optionIndex)
            {
                const auto& optionLines = section.optionLines[optionIndex];
                for (std::size_t lineIndex = 0; lineIndex < optionLines.size(); ++lineIndex)
                {
                    const auto& line = optionLines[lineIndex];
                    cardHeight += line.texture.height;
                    if (lineIndex + 1 < optionLines.size())
                    {
                        const int spacing = optionLineSkip > 0 ? std::max(0, optionLineSkip - line.texture.height) : kOptionLineSpacingFallback;
                        cardHeight += spacing;
                    }
                }

                if (optionIndex + 1 < section.optionLines.size())
                {
                    cardHeight += kOptionSpacing;
                }
            }

            SDL_Rect cardRect{bounds.x, cursorY, cardWidth, cardHeight};
            SDL_SetRenderDrawColor(context.renderer, kCardFillColor.r, kCardFillColor.g, kCardFillColor.b, kCardFillColor.a);
            SDL_RenderFillRect(context.renderer, &cardRect);
            SDL_SetRenderDrawColor(context.renderer, kCardBorderColor.r, kCardBorderColor.g, kCardBorderColor.b, kCardBorderColor.a);
            SDL_RenderDrawRect(context.renderer, &cardRect);

            int contentX = cardRect.x + kSectionPadding;
            int contentY = cardRect.y + kSectionPadding;

            if (hasTitle)
            {
                SDL_Rect titleRect{contentX, contentY, section.titleTexture.width, section.titleTexture.height};
                RenderTexture(context.renderer, section.titleTexture, titleRect);
                contentY += section.titleTexture.height;
                if (hasOptions)
                {
                    contentY += kSectionTitleSpacing;
                }
            }

            for (std::size_t optionIndex = 0; optionIndex < section.optionLines.size(); ++optionIndex)
            {
                const auto& optionLines = section.optionLines[optionIndex];
                for (std::size_t lineIndex = 0; lineIndex < optionLines.size(); ++lineIndex)
                {
                    const auto& line = optionLines[lineIndex];
                    const int lineX = contentX + (line.indent ? kBulletIndent : 0);
                    SDL_Rect lineRect{lineX, contentY, line.texture.width, line.texture.height};
                    RenderTexture(context.renderer, line.texture, lineRect);
                    contentY += line.texture.height;
                    if (lineIndex + 1 < optionLines.size())
                    {
                        const int spacing = optionLineSkip > 0 ? std::max(0, optionLineSkip - line.texture.height) : kOptionLineSpacingFallback;
                        contentY += spacing;
                    }
                }

                if (optionIndex + 1 < section.optionLines.size())
                {
                    contentY += kOptionSpacing;
                }
            }

            cursorY += cardHeight + kSectionSpacing;
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

void SimpleTextView::RebuildSectionTextures(const RenderContext& context, int maxWidth)
{
    sectionRenderData_.clear();
    lastSectionLayoutWidth_ = maxWidth;

    if (context.renderer == nullptr || context.paragraphFont == nullptr || maxWidth <= 0)
    {
        return;
    }

    const int cardInnerWidth = std::max(0, maxWidth - 2 * kSectionPadding);
    if (cardInnerWidth <= 0)
    {
        return;
    }

    sectionRenderData_.reserve(content_.sections.size());
    for (const auto& sectionContent : content_.sections)
    {
        SectionRenderData renderData;

        if (!sectionContent.title.empty())
        {
            TTF_Font* titleFont = context.headingFont != nullptr ? context.headingFont : context.paragraphFont;
            if (titleFont != nullptr)
            {
                renderData.titleTexture = CreateTextTexture(
                    context.renderer,
                    titleFont,
                    sectionContent.title,
                    context.primaryColor);
            }
        }

        const int optionWidth = std::max(0, cardInnerWidth - kBulletIndent);
        for (const auto& option : sectionContent.options)
        {
            if (option.empty() || optionWidth <= 0)
            {
                continue;
            }

            auto wrappedLines = WrapTextToWidth(context.paragraphFont, option, optionWidth);
            if (wrappedLines.empty())
            {
                wrappedLines.emplace_back(option);
            }

            std::vector<SectionLine> lineTextures;
            lineTextures.reserve(wrappedLines.size());

            for (std::size_t lineIndex = 0; lineIndex < wrappedLines.size(); ++lineIndex)
            {
                const bool firstLine = lineIndex == 0;
                std::string lineText = wrappedLines[lineIndex];
                if (firstLine)
                {
                    lineText.insert(0, "\xE2\x80\xA2 ");
                }

                SectionLine line;
                line.texture = CreateTextTexture(context.renderer, context.paragraphFont, lineText, context.primaryColor);
                line.indent = !firstLine;
                lineTextures.emplace_back(std::move(line));
            }

            if (!lineTextures.empty())
            {
                renderData.optionLines.emplace_back(std::move(lineTextures));
            }
        }

        if (renderData.titleTexture.texture != nullptr || !renderData.optionLines.empty())
        {
            sectionRenderData_.emplace_back(std::move(renderData));
        }
    }

    if (sectionRenderData_.empty())
    {
        lastSectionLayoutWidth_ = 0;
    }
}

} // namespace colony
