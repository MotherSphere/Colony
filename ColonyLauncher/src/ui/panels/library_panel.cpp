#include "ui/panels/library_panel.hpp"

#include "frontend/components/brand_card.hpp"
#include "frontend/components/buttons.hpp"
#include "frontend/components/badge.hpp"
#include "ui/layout.hpp"

#include "utils/color.hpp"
#include "utils/drawing.hpp"

#include <algorithm>
#include <cctype>

namespace colony::ui::panels
{
namespace
{
bool IsReadyState(std::string_view value)
{
    std::string lowercase;
    lowercase.reserve(value.size());
    for (unsigned char ch : value)
    {
        lowercase.push_back(static_cast<char>(std::tolower(ch)));
    }

    return lowercase == "ready" || lowercase == "live" || lowercase == "online";
}

SDL_Color ResolveAccentColor(
    const std::unordered_map<std::string, ProgramVisuals>& visuals,
    const colony::ViewContent& view,
    std::string_view programId)
{
    if (const auto it = visuals.find(std::string{programId}); it != visuals.end())
    {
        return it->second.accent;
    }
    if (!view.accentColor.empty())
    {
        return colony::color::ParseHexColor(view.accentColor);
    }
    return SDL_Color{static_cast<Uint8>(0x4F), static_cast<Uint8>(0x46), static_cast<Uint8>(0xE5), SDL_ALPHA_OPAQUE};
}
}

void LibraryPanel::Build(
    SDL_Renderer* renderer,
    TTF_Font* bodyFont,
    const ThemeColors& theme,
    const std::function<std::string(std::string_view)>& localize)
{
    (void)renderer;
    (void)bodyFont;
    (void)theme;
    (void)localize;
}

LibraryRenderResult LibraryPanel::Render(
    SDL_Renderer* renderer,
    const ThemeColors& theme,
    const InteractionColors& interactions,
    const SDL_Rect& libraryRect,
    const colony::AppContent& content,
    int activeChannelIndex,
    const std::unordered_map<std::string, ProgramVisuals>& programVisuals,
    TTF_Font* channelFont,
    TTF_Font* bodyFont,
    bool showAddButton,
    double timeSeconds,
    double deltaSeconds,
    std::string_view filterText,
    bool filterFocused,
    const std::vector<colony::frontend::models::LibraryProgramEntry>& programs,
    const std::vector<colony::frontend::models::LibrarySortChip>& sortChips) const
{
    (void)deltaSeconds;
    (void)filterText;
    (void)filterFocused;
    (void)sortChips;

    LibraryRenderResult result;
    result.addButtonRect.reset();
    result.filterInputRect.reset();
    result.sortChipHitboxes.clear();
    result.programIds.clear();

    SDL_SetRenderDrawColor(renderer, theme.libraryBackground.r, theme.libraryBackground.g, theme.libraryBackground.b, theme.libraryBackground.a);
    SDL_RenderFillRect(renderer, &libraryRect);

    const int padding = Scale(24);
    int cursorY = libraryRect.y + padding;
    if (activeChannelIndex >= 0 && activeChannelIndex < static_cast<int>(content.channels.size()))
    {
        const auto& channel = content.channels[activeChannelIndex];
        colony::TextTexture channelTitle = colony::CreateTextTexture(renderer, channelFont, channel.label, theme.heroTitle);
        if (channelTitle.texture)
        {
            SDL_Rect titleRect{libraryRect.x + padding, cursorY, channelTitle.width, channelTitle.height};
            colony::RenderTexture(renderer, channelTitle, titleRect);
            cursorY += titleRect.h + padding;
        }
    }

    const int cardWidth = Scale(320);
    const int cardHeight = Scale(220);
    const int gutter = Scale(20);
    const int availableWidth = libraryRect.w - 2 * padding;
    const int columns = std::max(1, availableWidth / (cardWidth + gutter));

    int row = 0;
    int column = 0;

    for (const auto& entry : programs)
    {
        if (content.views.find(entry.programId) == content.views.end())
        {
            continue;
        }
        const auto& view = content.views.at(entry.programId);
        frontend::components::BrandCard::Content cardContent;
        cardContent.id = entry.programId;
        cardContent.title = view.heading.empty() ? entry.programId : view.heading;
        cardContent.subtitle = view.tagline;
        cardContent.category = view.installState.empty() ? view.availability : view.installState;
        cardContent.metric = view.lastLaunched.empty() ? view.version : view.lastLaunched;
        cardContent.statusLabel = view.availability.empty() ? view.installState : view.availability;
        cardContent.metricBadgeLabel = view.version;
        cardContent.primaryActionLabel = view.primaryActionLabel.empty()
            ? (view.installState == "Installed" ? "Launch" : "Preview")
            : view.primaryActionLabel;
        cardContent.secondaryActionLabel = view.installState == "Installed" ? "Manage" : "Install";
        cardContent.highlights.assign(view.heroHighlights.begin(), view.heroHighlights.end());
        cardContent.ready = IsReadyState(cardContent.statusLabel);
        cardContent.accent = ResolveAccentColor(programVisuals, view, entry.programId);

        frontend::components::BrandCard card;
        card.Build(renderer, cardContent, channelFont, bodyFont, bodyFont, theme);

        SDL_Rect cardRect{
            libraryRect.x + padding + column * (cardWidth + gutter),
            cursorY + row * (cardHeight + gutter),
            cardWidth,
            cardHeight};

        cardRect.h = card.Render(renderer, theme, interactions, cardRect, bodyFont, bodyFont, false, entry.selected, timeSeconds).h;
        result.tileRects.push_back(cardRect);
        result.programIds.push_back(entry.programId);

        ++column;
        if (column >= columns)
        {
            column = 0;
            ++row;
        }
    }

    if (showAddButton)
    {
        SDL_Rect addRect{
            libraryRect.x + padding + column * (cardWidth + gutter),
            cursorY + row * (cardHeight + gutter),
            cardWidth,
            cardHeight};
        SDL_SetRenderDrawColor(renderer, theme.card.r, theme.card.g, theme.card.b, 200);
        colony::drawing::RenderFilledRoundedRect(renderer, addRect, Scale(18));
        SDL_SetRenderDrawColor(renderer, theme.border.r, theme.border.g, theme.border.b, 160);
        colony::drawing::RenderRoundedRect(renderer, addRect, Scale(18));
        SDL_RenderDrawLine(
            renderer,
            addRect.x + addRect.w / 2 - Scale(20),
            addRect.y + addRect.h / 2,
            addRect.x + addRect.w / 2 + Scale(20),
            addRect.y + addRect.h / 2);
        SDL_RenderDrawLine(
            renderer,
            addRect.x + addRect.w / 2,
            addRect.y + addRect.h / 2 - Scale(20),
            addRect.x + addRect.w / 2,
            addRect.y + addRect.h / 2 + Scale(20));
        result.addButtonRect = addRect;
    }

    return result;
}

} // namespace colony::ui::panels
