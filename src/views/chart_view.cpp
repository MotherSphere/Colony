#include "views/chart_view.hpp"

#include "ui/layout.hpp"
#include "utils/color.hpp"
#include "utils/drawing.hpp"
#include "utils/text.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace colony
{
namespace
{
constexpr int kHeadingSpacing = ui::Scale(24);
constexpr int kBarSpacing = ui::Scale(18);
constexpr int kBarHeight = ui::Scale(44);
constexpr int kBarPadding = ui::Scale(18);
constexpr int kLabelWidth = ui::Scale(180);
constexpr int kActionTopSpacing = ui::Scale(32);
constexpr int kActionWidth = ui::Scale(200);
constexpr int kActionHeight = ui::Scale(48);
constexpr int kActionRadius = ui::Scale(20);
constexpr SDL_Color kBarBackground{18, 24, 36, 200};
} // namespace

ChartView::ChartView(std::string id) : id_(std::move(id)) {}

std::string_view ChartView::Id() const noexcept
{
    return id_;
}

void ChartView::BindContent(const ViewContent& content)
{
    content_ = content;
}

void ChartView::Activate(const RenderContext& context)
{
    headingTexture_ = CreateTextTexture(context.renderer, context.headingFont, content_.heading, context.primaryColor);
    actionTexture_ = CreateTextTexture(context.renderer, context.buttonFont, content_.primaryActionLabel, context.accentColor);
    entries_.clear();
    lastActionRect_.reset();

    double maxValue = 0.0;
    for (const auto& datum : content_.chartData)
    {
        maxValue = std::max(maxValue, std::abs(datum.value));
    }
    if (maxValue <= 0.0)
    {
        maxValue = 1.0;
    }

    entries_.reserve(content_.chartData.size());
    for (const auto& datum : content_.chartData)
    {
        BarEntry entry;
        entry.label = CreateTextTexture(context.renderer, context.paragraphFont, datum.label, context.primaryColor);
        std::string valueString = std::to_string(datum.value);
        // Trim trailing zeros for readability.
        const auto dotPos = valueString.find('.');
        if (dotPos != std::string::npos)
        {
            while (!valueString.empty() && valueString.back() == '0')
            {
                valueString.pop_back();
            }
            if (!valueString.empty() && valueString.back() == '.')
            {
                valueString.pop_back();
            }
        }
        entry.valueText = CreateTextTexture(context.renderer, context.paragraphFont, valueString, context.mutedColor);
        entry.value = datum.value;
        entry.normalized = maxValue > 0.0 ? std::clamp(std::abs(datum.value) / maxValue, 0.0, 1.0) : 0.0;
        entries_.push_back(std::move(entry));
    }
}

void ChartView::Deactivate()
{
    headingTexture_ = {};
    actionTexture_ = {};
    entries_.clear();
    lastActionRect_.reset();
}

void ChartView::Render(const RenderContext& context, const SDL_Rect& bounds)
{
    int cursorY = bounds.y;

    if (headingTexture_.texture)
    {
        SDL_Rect headingRect{bounds.x, cursorY, headingTexture_.width, headingTexture_.height};
        RenderTexture(context.renderer, headingTexture_, headingRect);
        cursorY += headingRect.h + kHeadingSpacing;
    }

    const int availableWidth = std::max(0, bounds.w - kLabelWidth - kBarPadding * 2);
    for (const auto& entry : entries_)
    {
        SDL_Rect barArea{bounds.x + kLabelWidth + kBarPadding, cursorY, availableWidth, kBarHeight};
        SDL_Rect backgroundRect{barArea.x, barArea.y, barArea.w, barArea.h};
        SDL_SetRenderDrawColor(
            context.renderer,
            kBarBackground.r,
            kBarBackground.g,
            kBarBackground.b,
            kBarBackground.a);
        drawing::RenderFilledRoundedRect(context.renderer, backgroundRect, 12);

        SDL_Rect fillRect{barArea.x, barArea.y, static_cast<int>(barArea.w * entry.normalized), barArea.h};
        SDL_SetRenderDrawColor(
            context.renderer,
            context.accentColor.r,
            context.accentColor.g,
            context.accentColor.b,
            context.accentColor.a);
        drawing::RenderFilledRoundedRect(context.renderer, fillRect, 12);

        if (entry.label.texture)
        {
            SDL_Rect labelRect{bounds.x, cursorY + (kBarHeight - entry.label.height) / 2, entry.label.width, entry.label.height};
            RenderTexture(context.renderer, entry.label, labelRect);
        }
        if (entry.valueText.texture)
        {
            SDL_Rect valueRect{
                barArea.x + barArea.w - entry.valueText.width,
                cursorY + (kBarHeight - entry.valueText.height) / 2,
                entry.valueText.width,
                entry.valueText.height};
            RenderTexture(context.renderer, entry.valueText, valueRect);
        }

        cursorY += kBarHeight + kBarSpacing;
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

void ChartView::OnPrimaryAction(std::string& statusBuffer)
{
    statusBuffer = content_.statusMessage.empty() ? content_.primaryActionLabel : content_.statusMessage;
}

std::optional<SDL_Rect> ChartView::PrimaryActionRect() const
{
    return lastActionRect_;
}

} // namespace colony
