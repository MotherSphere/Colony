#include "frontend/views/settings_view.hpp"

#include "frontend/components/theme_swatch.hpp"
#include "ui/layout.hpp"
#include "ui/theme.hpp"
#include "utils/color.hpp"
#include "utils/drawing.hpp"
#include "utils/text.hpp"

#include <algorithm>
#include <array>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace colony::frontend::views
{
namespace
{
int ScaleValue(int value)
{
    return colony::ui::Scale(value);
}

int HeadingSpacing()
{
    return ScaleValue(22);
}

int SectionSpacing()
{
    return ScaleValue(28);
}

int SectionPadding()
{
    return ScaleValue(24);
}

int StickyHeight()
{
    return ScaleValue(48);
}

SDL_Color Blend(const SDL_Color& a, const SDL_Color& b, float t)
{
    return colony::color::Mix(a, b, std::clamp(t, 0.0f, 1.0f));
}

std::vector<SettingsView::ToggleRow> BuildToggleRows(
    SDL_Renderer* renderer,
    TTF_Font* titleFont,
    TTF_Font* bodyFont,
    SDL_Color titleColor,
    SDL_Color bodyColor)
{
    if (renderer == nullptr || titleFont == nullptr || bodyFont == nullptr)
    {
        return {};
    }

    struct ToggleDefinition
    {
        std::string id;
        std::string label;
        std::string description;
        bool prominent;
    };

    const std::array<ToggleDefinition, 4> definitions{
        ToggleDefinition{
            "notifications",
            "Notifications",
            "Be alerted when teammates invite you or when deployments complete.",
            true},
        ToggleDefinition{
            "reduced_motion",
            "Reduced motion",
            "Minimise animations to support accessibility needs.",
            true},
        ToggleDefinition{
            "sound",
            "Sound effects",
            "Play subtle interface sounds for events and interactions.",
            false},
        ToggleDefinition{
            "auto_updates",
            "Automatic updates",
            "Download new builds in the background when your device is idle.",
            false}};

    std::vector<SettingsView::ToggleRow> rows;
    rows.reserve(definitions.size());

    for (const auto& def : definitions)
    {
        SettingsView::ToggleRow row;
        row.id = def.id;
        row.prominent = def.prominent;
        row.label = colony::CreateTextTexture(renderer, titleFont, def.label, titleColor);
        row.description = colony::CreateTextTexture(renderer, bodyFont, def.description, bodyColor);
        rows.emplace_back(std::move(row));
    }

    return rows;
}

std::vector<SettingsView::SectionLabel> BuildSectionLabels(
    SDL_Renderer* renderer,
    TTF_Font* font,
    SDL_Color color)
{
    if (renderer == nullptr || font == nullptr)
    {
        return {};
    }

    std::vector<SettingsView::SectionLabel> labels;
    const std::array<std::pair<const char*, const char*>, 3> data{
        {{"appearance", "Appearance"}, {"language", "Language"}, {"toggles", "Toggles"}}};

    labels.reserve(data.size());
    for (const auto& [id, label] : data)
    {
        SettingsView::SectionLabel sectionLabel;
        sectionLabel.id = id;
        sectionLabel.label = colony::CreateTextTexture(renderer, font, label, color);
        labels.emplace_back(std::move(sectionLabel));
    }

    return labels;
}

std::vector<SettingsView::NavLink> BuildNavLinks(
    SDL_Renderer* renderer,
    TTF_Font* font,
    SDL_Color color)
{
    if (renderer == nullptr || font == nullptr)
    {
        return {};
    }

    std::vector<SettingsView::NavLink> nav;
    const std::array<std::pair<const char*, const char*>, 3> data{
        {{"appearance", "Appearance"}, {"language", "Language"}, {"toggles", "Toggles"}}};

    nav.reserve(data.size());
    for (const auto& [id, label] : data)
    {
        SettingsView::NavLink link;
        link.id = id;
        link.label = colony::CreateTextTexture(renderer, font, label, color);
        nav.emplace_back(std::move(link));
    }

    return nav;
}

std::unordered_map<std::string, frontend::components::ThemeSwatchText> BuildLanguageSamples()
{
    using frontend::components::ThemeSwatchText;
    return {
        {"en", ThemeSwatchText{"All systems go", "Primary panels and actions stay focused."}},
        {"fr", ThemeSwatchText{"Tout est prêt", "Les commandes restent accessibles et soignées."}},
        {"de", ThemeSwatchText{"Bereit", "Die wichtigsten Werkzeuge bleiben im Blick."}},
        {"hi", ThemeSwatchText{"सब तैयार है", "आपके लिए प्राथमिक नियंत्रण स्पष्ट रहते हैं."}},
        {"zh", ThemeSwatchText{"系统就绪", "关键操作保持清晰易达。"}},
        {"ar", ThemeSwatchText{"كل شيء جاهز", "تظل عناصر التحكم الأساسية واضحة ومتاحة."}}};
}

frontend::components::ThemeSwatchText MakeThemeSampleText(std::string_view name)
{
    return frontend::components::ThemeSwatchText{name, "See controls, cards, and typography in context."};
}

} // namespace

SettingsView::SettingsView(std::string id) : id_(std::move(id)) {}

std::string_view SettingsView::Id() const noexcept
{
    return id_;
}

void SettingsView::BindContent(const colony::ViewContent& content)
{
    content_ = content;
}

void SettingsView::Activate(const colony::RenderContext& context)
{
    if (context.renderer != nullptr && context.headingFont != nullptr)
    {
        headingTexture_ = colony::CreateTextTexture(context.renderer, context.headingFont, content_.heading, context.primaryColor);
    }
    else
    {
        headingTexture_ = {};
    }

    if (context.renderer != nullptr && context.paragraphFont != nullptr)
    {
        taglineTexture_ = colony::CreateTextTexture(context.renderer, context.paragraphFont, content_.tagline, context.mutedColor);
    }
    else
    {
        taglineTexture_ = {};
    }

    navLinks_ = BuildNavLinks(context.renderer, context.paragraphFont, context.primaryColor);
    sectionLabels_ = BuildSectionLabels(context.renderer, context.paragraphFont, context.primaryColor);
    toggles_ = BuildToggleRows(context.renderer, context.paragraphFont, context.paragraphFont, context.primaryColor, context.mutedColor);
    anchors_.clear();
    primaryActionRect_.reset();
    lastLayoutWidth_ = 0;
}

void SettingsView::Deactivate()
{
    headingTexture_ = {};
    taglineTexture_ = {};
    navLinks_.clear();
    sectionLabels_.clear();
    anchors_.clear();
    toggles_.clear();
    primaryActionRect_.reset();
    lastLayoutWidth_ = 0;
}

void SettingsView::Render(const colony::RenderContext& context, const SDL_Rect& bounds)
{
    if (context.renderer == nullptr)
    {
        return;
    }

    anchors_.clear();

    const int width = bounds.w;
    if (width != lastLayoutWidth_)
    {
        lastLayoutWidth_ = width;
    }

    const SDL_Color navBackground = Blend(context.mutedColor, context.primaryColor, 0.08f);
    const SDL_Color navHighlight = Blend(context.accentColor, context.primaryColor, 0.5f);

    int cursorY = bounds.y;

    if (headingTexture_.texture)
    {
        SDL_Rect headingRect{bounds.x, cursorY, headingTexture_.width, headingTexture_.height};
        colony::RenderTexture(context.renderer, headingTexture_, headingRect);
        cursorY += headingRect.h + HeadingSpacing();
    }

    if (taglineTexture_.texture)
    {
        SDL_Rect taglineRect{bounds.x, cursorY, taglineTexture_.width, taglineTexture_.height};
        colony::RenderTexture(context.renderer, taglineTexture_, taglineRect);
        cursorY += taglineRect.h + SectionSpacing();
    }

    const int navHeight = StickyHeight();
    SDL_Rect navRect{bounds.x, cursorY, bounds.w, navHeight};
    SDL_SetRenderDrawColor(context.renderer, navBackground.r, navBackground.g, navBackground.b, navBackground.a);
    SDL_RenderFillRect(context.renderer, &navRect);
    SDL_SetRenderDrawColor(context.renderer, navHighlight.r, navHighlight.g, navHighlight.b, navHighlight.a);
    SDL_Rect navTopBorder{navRect.x, navRect.y, navRect.w, ScaleValue(1)};
    SDL_RenderFillRect(context.renderer, &navTopBorder);

    const int navItemCount = static_cast<int>(navLinks_.size());
    const int navItemWidth = navItemCount > 0 ? bounds.w / navItemCount : 0;
    for (int index = 0; index < navItemCount; ++index)
    {
        auto& link = navLinks_[index];
        SDL_Rect itemRect{bounds.x + index * navItemWidth, navRect.y, navItemWidth, navRect.h};
        link.rect = itemRect;

        if (link.label.texture)
        {
            SDL_Rect labelRect{
                itemRect.x + (itemRect.w - link.label.width) / 2,
                itemRect.y + (itemRect.h - link.label.height) / 2,
                link.label.width,
                link.label.height};
            colony::RenderTexture(context.renderer, link.label, labelRect);
        }

        SDL_SetRenderDrawColor(context.renderer, navHighlight.r, navHighlight.g, navHighlight.b, navHighlight.a);
        SDL_Rect underline{
            itemRect.x + ScaleValue(18),
            itemRect.y + itemRect.h - ScaleValue(6),
            std::max(ScaleValue(24), itemRect.w - ScaleValue(36)),
            ScaleValue(3)};
        SDL_RenderFillRect(context.renderer, &underline);

        if (index + 1 < navItemCount)
        {
            SDL_SetRenderDrawColor(context.renderer, navHighlight.r, navHighlight.g, navHighlight.b, navHighlight.a);
            SDL_Rect divider{itemRect.x + itemRect.w - ScaleValue(1), itemRect.y + ScaleValue(12), ScaleValue(1), itemRect.h - ScaleValue(24)};
            SDL_RenderFillRect(context.renderer, &divider);
        }
    }

    cursorY += navHeight + SectionSpacing();

    auto findSectionLabel = [&](std::string_view id) -> const colony::TextTexture* {
        for (const auto& label : sectionLabels_)
        {
            if (label.id == id)
            {
                return &label.label;
            }
        }
        return nullptr;
    };

    const int contentStartX = bounds.x + SectionPadding();
    const int contentWidth = bounds.w - SectionPadding() * 2;

    const auto renderSectionHeader = [&](std::string_view id) {
        anchors_.push_back({std::string{id}, cursorY - bounds.y});
        const int latestOffset = anchors_.back().offsetY;
        for (auto& link : navLinks_)
        {
            if (link.id == id)
            {
                link.targetOffset = latestOffset;
                break;
            }
        }
        if (const auto* label = findSectionLabel(id); label != nullptr && label->texture)
        {
            SDL_Rect labelRect{contentStartX, cursorY, label->width, label->height};
            colony::RenderTexture(context.renderer, *label, labelRect);
            cursorY += labelRect.h + ScaleValue(16);
        }
    };

    renderSectionHeader("appearance");
    colony::ui::ThemeManager themeManager;
    const auto& schemes = themeManager.Schemes();
    const int swatchHeight = ScaleValue(140);
    const int swatchSpacing = ScaleValue(16);
    const int maxColumns = contentWidth > ScaleValue(640) ? 3 : 2;
    const int columnSpacing = ScaleValue(14);
    const int columnWidth = maxColumns > 0 ? (contentWidth - columnSpacing * (maxColumns - 1)) / maxColumns : contentWidth;

    for (std::size_t index = 0; index < schemes.size(); ++index)
    {
        const auto column = static_cast<int>(index % maxColumns);
        const auto row = static_cast<int>(index / maxColumns);
        const int swatchX = contentStartX + column * (columnWidth + columnSpacing);
        const int swatchY = cursorY + row * (swatchHeight + swatchSpacing);
        SDL_Rect swatchRect{swatchX, swatchY, columnWidth, swatchHeight};

        auto sampleText = MakeThemeSampleText(schemes[index].name);
        frontend::components::ThemeSwatchStyle swatchStyle{};
        swatchStyle.accentPulse = static_cast<float>((index % maxColumns) + 1) / static_cast<float>(maxColumns + 1);
        frontend::components::RenderThemeSwatch(
            context.renderer,
            schemes[index].colors,
            swatchRect,
            context.paragraphFont,
            context.paragraphFont,
            sampleText,
            swatchStyle);
    }

    if (!schemes.empty())
    {
        const int rows = static_cast<int>((schemes.size() + maxColumns - 1) / maxColumns);
        cursorY += rows * (swatchHeight + swatchSpacing);
    }
    cursorY += SectionSpacing();

    renderSectionHeader("language");
    const auto languageSamples = BuildLanguageSamples();
    const std::array<std::string, 6> languageOrder{"en", "fr", "de", "hi", "zh", "ar"};
    const int languageSwatchHeight = ScaleValue(130);

    colony::ui::ThemeColors languageTheme{};
    if (!schemes.empty())
    {
        languageTheme = schemes.front().colors;
    }
    else
    {
        colony::ui::ThemeManager fallback;
        const auto& fallbackSchemes = fallback.Schemes();
        if (!fallbackSchemes.empty())
        {
            languageTheme = fallbackSchemes.front().colors;
        }
    }

    for (std::size_t index = 0; index < languageOrder.size(); ++index)
    {
        const auto column = static_cast<int>(index % maxColumns);
        const auto row = static_cast<int>(index / maxColumns);
        const int swatchX = contentStartX + column * (columnWidth + columnSpacing);
        const int swatchY = cursorY + row * (languageSwatchHeight + swatchSpacing);
        SDL_Rect swatchRect{swatchX, swatchY, columnWidth, languageSwatchHeight};

        auto it = languageSamples.find(languageOrder[index]);
        frontend::components::ThemeSwatchText text =
            it != languageSamples.end() ? it->second : frontend::components::ThemeSwatchText{"", ""};
        frontend::components::RenderThemeSwatch(
            context.renderer,
            languageTheme,
            swatchRect,
            context.paragraphFont,
            context.paragraphFont,
            text,
            {});
    }

    if (!languageOrder.empty())
    {
        const int rows = static_cast<int>((languageOrder.size() + maxColumns - 1) / maxColumns);
        cursorY += rows * (languageSwatchHeight + swatchSpacing);
    }
    cursorY += SectionSpacing();

    renderSectionHeader("toggles");
    const int toggleAreaWidth = contentWidth;
    const int columnCount = 2;
    const int columnSpacingToggles = ScaleValue(14);
    const int toggleColumnWidth = (toggleAreaWidth - columnSpacingToggles) / columnCount;
    const int prominentHeight = ScaleValue(92);
    const int standardHeight = ScaleValue(74);

    std::vector<ToggleRow*> prominent;
    std::vector<ToggleRow*> standard;
    prominent.reserve(toggles_.size());
    standard.reserve(toggles_.size());

    for (auto& toggle : toggles_)
    {
        if (toggle.prominent)
        {
            prominent.push_back(&toggle);
        }
        else
        {
            standard.push_back(&toggle);
        }
    }

    int toggleCursorY = cursorY;
    const int toggleStartX = contentStartX;

    for (std::size_t index = 0; index < prominent.size(); ++index)
    {
        const int column = static_cast<int>(index % columnCount);
        if (column == 0 && index != 0)
        {
            toggleCursorY += prominentHeight + swatchSpacing;
        }
        const int toggleX = toggleStartX + column * (toggleColumnWidth + columnSpacingToggles);
        SDL_Rect rect{toggleX, toggleCursorY, toggleColumnWidth, prominentHeight};
        prominent[index]->rect = rect;

        SDL_Color fill = Blend(context.primaryColor, context.accentColor, 0.1f);
        SDL_Color stroke = Blend(context.accentColor, context.primaryColor, 0.35f);
        colony::drawing::RenderFilledRoundedRect(context.renderer, rect, ScaleValue(16));
        SDL_SetRenderDrawColor(context.renderer, stroke.r, stroke.g, stroke.b, stroke.a);
        colony::drawing::RenderRoundedRect(context.renderer, rect, ScaleValue(16));

        const int padding = ScaleValue(16);
        int textY = rect.y + padding;
        if (prominent[index]->label.texture)
        {
            SDL_Rect labelRect{rect.x + padding, textY, prominent[index]->label.width, prominent[index]->label.height};
            colony::RenderTexture(context.renderer, prominent[index]->label, labelRect);
            textY += labelRect.h + ScaleValue(8);
        }
        if (prominent[index]->description.texture)
        {
            SDL_Rect descriptionRect{rect.x + padding, textY, prominent[index]->description.width, prominent[index]->description.height};
            colony::RenderTexture(context.renderer, prominent[index]->description, descriptionRect);
        }
    }

    if (!prominent.empty())
    {
        toggleCursorY += prominentHeight + swatchSpacing;
    }

    for (auto* toggle : standard)
    {
        SDL_Rect rect{toggleStartX, toggleCursorY, toggleAreaWidth, standardHeight};
        toggle->rect = rect;
        SDL_Color fill = Blend(context.primaryColor, context.mutedColor, 0.08f);
        SDL_Color stroke = Blend(context.mutedColor, context.primaryColor, 0.2f);
        colony::drawing::RenderFilledRoundedRect(context.renderer, rect, ScaleValue(12));
        SDL_SetRenderDrawColor(context.renderer, stroke.r, stroke.g, stroke.b, stroke.a);
        colony::drawing::RenderRoundedRect(context.renderer, rect, ScaleValue(12));

        const int padding = ScaleValue(16);
        int textY = rect.y + padding;
        if (toggle->label.texture)
        {
            SDL_Rect labelRect{rect.x + padding, textY, toggle->label.width, toggle->label.height};
            colony::RenderTexture(context.renderer, toggle->label, labelRect);
            textY += labelRect.h + ScaleValue(6);
        }
        if (toggle->description.texture)
        {
            SDL_Rect descriptionRect{rect.x + padding, textY, toggle->description.width, toggle->description.height};
            colony::RenderTexture(context.renderer, toggle->description, descriptionRect);
        }

        toggleCursorY += standardHeight + swatchSpacing;
    }

    cursorY = toggleCursorY;
}

void SettingsView::OnPrimaryAction(std::string& statusBuffer)
{
    statusBuffer = content_.statusMessage;
}

std::optional<SDL_Rect> SettingsView::PrimaryActionRect() const
{
    return primaryActionRect_;
}

} // namespace colony::frontend::views
