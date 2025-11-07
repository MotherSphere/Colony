#include "views/media_view.hpp"

#include "ui/layout.hpp"
#include "utils/drawing.hpp"
#include "utils/text.hpp"

#include <utility>

namespace colony
{
namespace
{
constexpr int kHeadingSpacing = ui::Scale(18);
constexpr int kTaglineSpacing = ui::Scale(20);
constexpr int kCardPadding = ui::Scale(18);
constexpr int kCardSpacing = ui::Scale(16);
constexpr int kActionTopSpacing = ui::Scale(28);
constexpr int kActionHeight = ui::Scale(48);
constexpr int kActionWidth = ui::Scale(220);
constexpr int kActionRadius = ui::Scale(20);
constexpr SDL_Color kCardBackground{15, 17, 26, 210};
constexpr SDL_Color kCardBorder{52, 61, 91, SDL_ALPHA_OPAQUE};
} // namespace

MediaView::MediaView(std::string id) : id_(std::move(id)) {}

std::string_view MediaView::Id() const noexcept
{
    return id_;
}

void MediaView::BindContent(const ViewContent& content)
{
    content_ = content;
}

void MediaView::Activate(const RenderContext& context)
{
    headingTexture_ = CreateTextTexture(context.renderer, context.headingFont, content_.heading, context.primaryColor);
    taglineTexture_ = CreateTextTexture(context.renderer, context.paragraphFont, content_.tagline, context.mutedColor);
    actionTexture_ = CreateTextTexture(context.renderer, context.buttonFont, content_.primaryActionLabel, context.accentColor);

    entries_.clear();
    entries_.reserve(content_.mediaItems.size());
    for (const auto& item : content_.mediaItems)
    {
        MediaEntry entry;
        entry.title = CreateTextTexture(context.renderer, context.paragraphFont, item.title, context.primaryColor);
        entry.description = CreateTextTexture(context.renderer, context.paragraphFont, item.description, context.mutedColor);
        entries_.push_back(std::move(entry));
    }
    lastActionRect_.reset();
}

void MediaView::Deactivate()
{
    headingTexture_ = {};
    taglineTexture_ = {};
    actionTexture_ = {};
    entries_.clear();
    lastActionRect_.reset();
}

void MediaView::Render(const RenderContext& context, const SDL_Rect& bounds)
{
    int cursorY = bounds.y;

    if (headingTexture_.texture)
    {
        SDL_Rect headingRect{bounds.x, cursorY, headingTexture_.width, headingTexture_.height};
        RenderTexture(context.renderer, headingTexture_, headingRect);
        cursorY += headingRect.h + kHeadingSpacing;
    }

    if (taglineTexture_.texture)
    {
        SDL_Rect taglineRect{bounds.x, cursorY, taglineTexture_.width, taglineTexture_.height};
        RenderTexture(context.renderer, taglineTexture_, taglineRect);
        cursorY += taglineRect.h + kTaglineSpacing;
    }

    for (const auto& entry : entries_)
    {
        const int cardHeight = kCardPadding * 2 + entry.title.height + (entry.description.texture ? entry.description.height + kHeadingSpacing / 2 : 0);
        SDL_Rect cardRect{bounds.x, cursorY, bounds.w, cardHeight};
        SDL_SetRenderDrawColor(context.renderer, kCardBackground.r, kCardBackground.g, kCardBackground.b, kCardBackground.a);
        drawing::RenderFilledRoundedRect(context.renderer, cardRect, 18);
        SDL_SetRenderDrawColor(context.renderer, kCardBorder.r, kCardBorder.g, kCardBorder.b, kCardBorder.a);
        drawing::RenderRoundedRect(context.renderer, cardRect, 18);

        int textY = cardRect.y + kCardPadding;
        SDL_Rect titleRect{cardRect.x + kCardPadding, textY, entry.title.width, entry.title.height};
        RenderTexture(context.renderer, entry.title, titleRect);
        textY += entry.title.height;

        if (entry.description.texture)
        {
            textY += kHeadingSpacing / 2;
            SDL_Rect descriptionRect{
                cardRect.x + kCardPadding,
                textY,
                entry.description.width,
                entry.description.height};
            RenderTexture(context.renderer, entry.description, descriptionRect);
        }

        cursorY += cardRect.h + kCardSpacing;
    }

    if (actionTexture_.texture)
    {
        cursorY += kActionTopSpacing;
        SDL_Rect buttonRect{
            bounds.x + bounds.w - kActionWidth,
            cursorY,
            kActionWidth,
            kActionHeight};
        SDL_SetRenderDrawColor(
            context.renderer,
            context.accentColor.r,
            context.accentColor.g,
            context.accentColor.b,
            context.accentColor.a);
        drawing::RenderFilledRoundedRect(context.renderer, buttonRect, kActionRadius);
        SDL_Rect textRect{
            buttonRect.x + (buttonRect.w - actionTexture_.width) / 2,
            buttonRect.y + (buttonRect.h - actionTexture_.height) / 2,
            actionTexture_.width,
            actionTexture_.height};
        RenderTexture(context.renderer, actionTexture_, textRect);
        lastActionRect_ = buttonRect;
    }
}

void MediaView::OnPrimaryAction(std::string& statusBuffer)
{
    statusBuffer = content_.statusMessage.empty() ? content_.primaryActionLabel : content_.statusMessage;
}

std::optional<SDL_Rect> MediaView::PrimaryActionRect() const
{
    return lastActionRect_;
}

} // namespace colony
