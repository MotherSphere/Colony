#include "app/application.h"

#include "core/content_loader.hpp"
#include "frontend/utils/font_loader.hpp"
#include "frontend/views/dashboard_page.hpp"
#include "nexus/nexus_main.hpp"
#include "ui/layout.hpp"
#include "ui/theme.hpp"
#include "utils/color.hpp"
#include "utils/drawing.hpp"
#include "utils/asset_paths.hpp"
#include "utils/font_manager.hpp"
#include "utils/text.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <ctime>
#include <cstring>
#include <iomanip>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <vector>
#include <cstdlib>
#if !defined(_WIN32)
#include <unistd.h>
#endif

namespace colony
{

void Application::RenderFrame(double deltaSeconds)
{
    switch (interfaceState_)
    {
    case InterfaceState::Hub:
        RenderHubFrame(deltaSeconds);
        break;
    case InterfaceState::MainInterface:
        RenderMainInterfaceFrame(deltaSeconds);
        break;
    }
}

void Application::RenderHubFrame(double deltaSeconds)
{
    (void)deltaSeconds;

    SDL_Renderer* renderer = rendererHost_.Renderer();
    if (!renderer)
    {
        return;
    }

    const platform::RendererDimensions outputDimensions = rendererHost_.OutputSize();
    const int outputWidth = outputDimensions.width;
    const int outputHeight = outputDimensions.height;

    SDL_SetRenderDrawColor(renderer, theme_.background.r, theme_.background.g, theme_.background.b, theme_.background.a);
    SDL_RenderClear(renderer);

    const SDL_Rect bounds{0, 0, std::max(0, outputWidth), std::max(0, outputHeight)};
    const double timeSeconds = animationTimeSeconds_;

    int activeBranchIndex = focusedHubBranchIndex_;
    if (activeBranchIndex < 0 || activeBranchIndex >= static_cast<int>(content_.hub.branches.size()))
    {
        activeBranchIndex = content_.hub.branches.empty() ? -1 : 0;
    }

    ui::panels::HubRenderResult renderResult = hubPanel_.Render(
        renderer,
        theme_,
        bounds,
        timeSeconds,
        hoveredHubBranchIndex_,
        activeBranchIndex,
        focusedHubBranchIndex_,
        hubScrollOffset_,
        isHubHeroCollapsed_,
        hubSearchQuery_,
        hubSearchFocused_,
        hubWidgetPage_,
        hubWidgetsPerPage_);

    hubBranchHitboxes_ = renderResult.branchHitboxes;
    hubWidgetPagerHitboxes_ = renderResult.widgetPagerHitboxes;
    hubScrollViewport_ = renderResult.scrollViewport;
    hubScrollViewportValid_ = hubScrollViewport_.w > 0 && hubScrollViewport_.h > 0;
    hubScrollMaxOffset_ = std::max(0, renderResult.scrollableContentHeight - renderResult.visibleContentHeight);
    EnsureHubScrollWithinBounds();
    hubHeroToggleRect_ = (renderResult.heroToggleRect.w > 0 && renderResult.heroToggleRect.h > 0)
        ? std::optional<SDL_Rect>(renderResult.heroToggleRect)
        : std::nullopt;
    hubSearchInputRect_ = (renderResult.searchInputRect.w > 0 && renderResult.searchInputRect.h > 0)
        ? std::optional<SDL_Rect>(renderResult.searchInputRect)
        : std::nullopt;
    hubSearchClearRect_ = (renderResult.searchClearRect.w > 0 && renderResult.searchClearRect.h > 0)
        ? std::optional<SDL_Rect>(renderResult.searchClearRect)
        : std::nullopt;
    hubDetailActionRect_ = (renderResult.detailActionRect.w > 0 && renderResult.detailActionRect.h > 0)
        ? std::optional<SDL_Rect>(renderResult.detailActionRect)
        : std::nullopt;
    hubWidgetPageCount_ = renderResult.widgetPageCount;
    if (hubWidgetPageCount_ == 0)
    {
        hubWidgetPage_ = 0;
    }
    else
    {
        hubWidgetPage_ = std::clamp(hubWidgetPage_, 0, hubWidgetPageCount_ - 1);
    }

    SDL_RenderPresent(renderer);
}


void Application::RenderMainInterfaceFrame(double deltaSeconds)
{
    SDL_Renderer* renderer = rendererHost_.Renderer();
    if (!renderer)
    {
        return;
    }

    const platform::RendererDimensions outputDimensions = rendererHost_.OutputSize();
    int outputWidth = outputDimensions.width;
    int outputHeight = outputDimensions.height;

    SDL_SetRenderDrawColor(renderer, theme_.background.r, theme_.background.g, theme_.background.b, theme_.background.a);
    SDL_RenderClear(renderer);

    const double timeSeconds = animationTimeSeconds_;
    const double realtimeSeconds = static_cast<double>(SDL_GetTicks64()) / 1000.0;
    libraryFilterDebouncer_.Flush(realtimeSeconds);

    if (!layoutSizesInitialized_)
    {
        navRailWidth_ = ui::Scale(112);
        layoutSizesInitialized_ = true;
    }

    UpdateLayoutForOutputWidth(outputWidth);

    SDL_Rect navRailRect{0, 0, std::max(0, navRailWidth_), outputHeight};
    SDL_SetRenderDrawColor(renderer, theme_.navRail.r, theme_.navRail.g, theme_.navRail.b, theme_.navRail.a);
    SDL_RenderFillRect(renderer, &navRailRect);
    navRailRect_ = navRailRect;

    const SDL_Rect contentRect{navRailRect.w, 0, std::max(0, outputWidth - navRailRect.w), outputHeight};
    const int topBarHeight = ui::Scale(96);
    const int detailWidth = std::clamp(outputWidth / 3, ui::Scale(360), ui::Scale(520));
    const int layoutGutter = ui::Scale(24);

    frontend::views::DashboardPage dashboardPage;
    frontend::views::DashboardLayout layout = dashboardPage.Compute(contentRect, detailWidth, topBarHeight, layoutGutter);
    libraryRect_ = layout.libraryArea;
    heroRect_ = layout.detailArea;

    const int statusBarHeight = ui::Scale(kStatusBarHeight);

    ui::panels::NavigationRenderResult navigationRender = navigationRail_.Render(
        renderer,
        theme_,
        typography_,
        interactions_,
        navRailRect,
        statusBarHeight,
        content_,
        channelSelections_,
        activeChannelIndex_,
        programVisuals_,
        timeSeconds);
    channelButtonRects_ = std::move(navigationRender.channelButtonRects);
    hubButtonRect_ = navigationRender.hubButtonRect;

    auto topBarResult = topBar_.Render(
        renderer,
        theme_,
        typography_,
        interactions_,
        layout.topBar,
        libraryFilterDraft_,
        libraryFilterFocused_,
        timeSeconds);
    libraryFilterInputRect_ = topBarResult.searchFieldRect;

    bool showAddButton = false;
    if (activeChannelIndex_ >= 0 && activeChannelIndex_ < static_cast<int>(content_.channels.size()))
    {
        auto toLower = [](std::string value) {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            return value;
        };

        const std::string channelIdLower = toLower(content_.channels[activeChannelIndex_].id);
        const std::string localIdLower = toLower(std::string(kLocalAppsChannelId));
        showAddButton = channelIdLower == localIdLower;
    }

    const auto sortChips = libraryViewModel_.BuildSortChips([this](std::string_view key) {
        return GetLocalizedString(key);
    });
    auto programEntries = libraryViewModel_.BuildProgramList(content_, activeChannelIndex_, channelSelections_);

    auto libraryResult = libraryPanel_.Render(
        renderer,
        theme_,
        interactions_,
        layout.libraryArea,
        content_,
        activeChannelIndex_,
        programVisuals_,
        fonts_.channel.get(),
        fonts_.tileMeta.get(),
        showAddButton,
        timeSeconds,
        deltaSeconds,
        libraryFilterDraft_,
        libraryFilterFocused_,
        programEntries,
        sortChips);
    programTileRects_ = libraryResult.tileRects;
    addAppButtonRect_ = libraryResult.addButtonRect;
    programTileProgramIds_ = libraryResult.programIds;
    librarySortChipHitboxes_.clear();

    navResizeHandleRect_ = SDL_Rect{0, 0, 0, 0};
    libraryResizeHandleRect_ = SDL_Rect{0, 0, 0, 0};

    const auto visualsIt = programVisuals_.find(activeProgramId_);
    const ui::ProgramVisuals* activeVisuals = visualsIt != programVisuals_.end() ? &visualsIt->second : nullptr;
    if (activeVisuals != nullptr)
    {
        const float gradientPulse = static_cast<float>(0.5 + 0.5 * std::sin(timeSeconds * 0.6));
        SDL_Color gradientStart = color::Mix(activeVisuals->gradientStart, activeVisuals->accent, 0.15f + 0.1f * gradientPulse);
        SDL_Color gradientEnd = color::Mix(activeVisuals->gradientEnd, theme_.heroGradientFallbackEnd, 0.2f * gradientPulse);
        color::RenderVerticalGradient(renderer, heroRect_, gradientStart, gradientEnd);
    }
    else
    {
        const float gradientPulse = static_cast<float>(0.5 + 0.5 * std::sin(timeSeconds * 0.8));
        SDL_Color gradientStart = color::Mix(theme_.heroGradientFallbackStart, theme_.channelBadge, 0.1f + 0.15f * gradientPulse);
        SDL_Color gradientEnd = color::Mix(theme_.heroGradientFallbackEnd, theme_.border, 0.1f * static_cast<float>(std::cos(timeSeconds * 0.6) * 0.5 + 0.5));
        color::RenderVerticalGradient(renderer, heroRect_, gradientStart, gradientEnd);
    }

    heroActionRect_.reset();
    SDL_Rect previousSettingsViewport = settingsRenderResult_.viewport;
    const int previousSettingsContentHeight = settingsRenderResult_.contentHeight;
    settingsRenderResult_.interactiveRegions.clear();
    settingsRenderResult_.contentHeight = 0;
    settingsRenderResult_.viewport = SDL_Rect{0, 0, 0, 0};

    if (IsSettingsProgramId(activeProgramId_))
    {
        const int previousViewportHeight = previousSettingsViewport.h;
        const int previousMaxScroll = std::max(0, previousSettingsContentHeight - previousViewportHeight);
        settingsScrollOffset_ = std::clamp(settingsScrollOffset_, 0, previousMaxScroll);

        heroPanel_.RenderSettings(
            renderer,
            theme_,
            heroRect_,
            settingsPanel_,
            settingsScrollOffset_,
            themeManager_.ActiveScheme().id,
            settingsService_.ActiveLanguageId(),
            settingsSectionStates_,
            settingsService_.AppearanceCustomizationValues(),
            settingsService_.ToggleStates(),
            settingsRenderResult_,
            timeSeconds);

        int maxScroll = 0;
        if (settingsRenderResult_.viewport.w > 0 && settingsRenderResult_.viewport.h > 0)
        {
            maxScroll = std::max(0, settingsRenderResult_.contentHeight - settingsRenderResult_.viewport.h);
            settingsScrollOffset_ = std::clamp(settingsScrollOffset_, 0, maxScroll);
        }

        if (pendingSettingsSectionId_.has_value())
        {
            const auto anchorIt = std::find_if(
                settingsRenderResult_.sectionAnchors.begin(),
                settingsRenderResult_.sectionAnchors.end(),
                [&](const ui::SettingsPanel::RenderResult::SectionAnchor& anchor) {
                    return anchor.id == *pendingSettingsSectionId_;
                });
            if (anchorIt != settingsRenderResult_.sectionAnchors.end())
            {
                if (settingsRenderResult_.viewport.w > 0 && settingsRenderResult_.viewport.h > 0)
                {
                    maxScroll = std::max(0, settingsRenderResult_.contentHeight - settingsRenderResult_.viewport.h);
                }
                settingsScrollOffset_ = std::clamp(anchorIt->offset, 0, maxScroll);
                pendingSettingsSectionId_.reset();
            }
        }
    }
    else if (activeVisuals != nullptr)
    {
        const auto heroResult = heroPanel_.RenderHero(
            renderer,
            theme_,
            heroRect_,
            visualsIt->second,
            fonts_.heroBody.get(),
            fonts_.patchTitle.get(),
            fonts_.patchBody.get(),
            timeSeconds,
            deltaSeconds);
        heroActionRect_ = heroResult.actionButtonRect;
    }

    if (!IsSettingsProgramId(activeProgramId_))
    {
        settingsScrollOffset_ = 0;
    }

    heroPanel_.RenderStatusBar(renderer, theme_, heroRect_, statusBarHeight, activeVisuals, timeSeconds);

    if (customThemeDialog_.visible)
    {
        RenderCustomThemeDialog(timeSeconds);
    }

    if (addAppDialog_.visible)
    {
        RenderAddAppDialog(timeSeconds);
    }

    if (editAppDialog_.visible)
    {
        RenderEditUserAppDialog(timeSeconds);
    }

    SDL_RenderPresent(renderer);
}


void Application::LaunchNexusApp()
{
    const std::string previousStatus = statusBuffer_;

    UpdateStatusMessage("Nexus is running in a separate window. Close it to return to Colony.");

    const nexus::NexusResult result = nexus::LaunchStandalone();

    if (result.propagateQuit)
    {
        SDL_Event quitEvent{};
        quitEvent.type = SDL_QUIT;
        SDL_PushEvent(&quitEvent);
    }

    UpdateStatusMessage(previousStatus);
}

void Application::UpdateStatusMessage(const std::string& statusText)
{
    statusBuffer_ = statusText;
    if (activeProgramId_.empty())
    {
        return;
    }

    if (auto it = programVisuals_.find(activeProgramId_); it != programVisuals_.end())
    {
        it->second.statusBar = CreateTextTexture(
            rendererHost_.Renderer(),
            fonts_.status.get(),
            statusBuffer_,
            theme_.statusBarText);
    }
}

void Application::UpdateViewContextAccent()
{
    if (activeProgramId_.empty())
    {
        viewContext_.accentColor = theme_.channelBadge;
        return;
    }

    if (const auto it = programVisuals_.find(activeProgramId_); it != programVisuals_.end())
    {
        viewContext_.accentColor = it->second.accent;
    }
    else
    {
        viewContext_.accentColor = theme_.channelBadge;
    }
}

bool Application::SetAppearanceCustomizationValue(const std::string& id, float value)
{
    return settingsService_.SetAppearanceCustomizationValue(id, value);
}

float Application::GetAppearanceCustomizationValue(std::string_view id) const
{
    return settingsService_.GetAppearanceCustomizationValue(id);
}

void Application::QueueLibraryFilterUpdate()
{
    const double nowSeconds = static_cast<double>(SDL_GetTicks64()) / 1000.0;
    libraryFilterDebouncer_.Schedule(nowSeconds, [this, draft = libraryFilterDraft_]() {
        libraryViewModel_.SetFilter(draft);
        libraryFilterDraft_ = libraryViewModel_.Filter();
    });
}

} // namespace colony

