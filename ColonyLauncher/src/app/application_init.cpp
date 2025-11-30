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
namespace
{
void RemoveLastUtf8Codepoint(std::string& value)
{
    if (value.empty())
    {
        return;
    }

    auto it = value.end();
    do
    {
        --it;
    } while (it != value.begin() && ((*it & 0xC0) == 0x80));

    value.erase(it, value.end());
}

float ComputeCustomizationSliderValue(const SDL_Rect& rect, int mouseX)
{
    const int knobSize = ui::Scale(28);
    const int knobTravel = std::max(1, rect.w - knobSize);
    const int relative = std::clamp(mouseX - rect.x - knobSize / 2, 0, knobTravel);
    if (knobTravel <= 0)
    {
        return 0.0f;
    }

    return static_cast<float>(relative) / static_cast<float>(knobTravel);
}

} // namespace


bool Application::IsSettingsProgramId(std::string_view programId)
{
    return programId == kSettingsAppearanceProgramId || programId == kSettingsLanguageProgramId
        || programId == kSettingsGeneralProgramId;
}

std::string_view Application::SettingsSectionForProgram(std::string_view programId)
{
    if (programId == kSettingsAppearanceProgramId)
    {
        return ui::SettingsPanel::kAppearanceSectionId;
    }
    if (programId == kSettingsLanguageProgramId)
    {
        return ui::SettingsPanel::kLanguageSectionId;
    }
    if (programId == kSettingsGeneralProgramId)
    {
        return ui::SettingsPanel::kGeneralSectionId;
    }
    return {};
}

Application::Application()
    : navigationInputHandler_(*this)
    , hubInputHandler_(*this)
    , dialogInputHandler_(*this)
    , libraryInputHandler_(*this)
    , addAppDialogController_(*this)
    , editUserAppDialogController_(*this)
    , customThemeDialogController_(*this)
    , themeService_(themeManager_)
{}

int Application::Run()
{
    if (!rendererHost_.Init("Colony Launcher", kWindowWidth, kWindowHeight))
    {
        return EXIT_FAILURE;
    }

    if (!InitializeFonts())
    {
        rendererHost_.Shutdown();
        return EXIT_FAILURE;
    }

    if (!LoadContent())
    {
        rendererHost_.Shutdown();
        return EXIT_FAILURE;
    }

    settingsService_.Load(ResolveSettingsPath(), themeManager_);

    if (!InitializeLocalization())
    {
        rendererHost_.Shutdown();
        return EXIT_FAILURE;
    }

    InitializeNavigation();
    InitializeViews();
    RebuildTheme();

    channelButtonRects_.assign(content_.channels.size(), SDL_Rect{});
    InitializeInputRouter();

    bool running = true;
    SDL_Event event{};
    lastFrameCounter_ = SDL_GetPerformanceCounter();
    animationTimeSeconds_ = 0.0;

    while (running)
    {
        const Uint64 now = SDL_GetPerformanceCounter();
        const Uint64 elapsedTicks = now - lastFrameCounter_;
        lastFrameCounter_ = now;

        double deltaSeconds = 0.0;
        if (SDL_GetPerformanceFrequency() != 0)
        {
            deltaSeconds = static_cast<double>(elapsedTicks) / static_cast<double>(SDL_GetPerformanceFrequency());
        }
        deltaSeconds = std::min(deltaSeconds, 0.25);

        const auto& toggleStates = settingsService_.ToggleStates();
        const auto reduceMotionIt = toggleStates.find("reduced_motion");
        const bool reduceMotion = reduceMotionIt != toggleStates.end() && reduceMotionIt->second;
        if (!reduceMotion)
        {
            animationTimeSeconds_ += deltaSeconds;
        }

        while (SDL_PollEvent(&event))
        {
            inputRouter_.Dispatch(event, running);
        }

        RenderFrame(reduceMotion ? 0.0 : deltaSeconds);
    }

    settingsService_.Save(ResolveSettingsPath(), themeManager_);
    rendererHost_.Shutdown();
    return EXIT_SUCCESS;
}

void Application::ShowHub()
{
    interfaceState_ = InterfaceState::Hub;
    ResetHubInteractionState();
    HideAddAppDialog();
    HideEditUserAppDialog();
    HideCustomThemeDialog();
    activeCustomizationDragId_.reset();
    libraryFilterFocused_ = false;
    UpdateTextInputState();
    const std::string statusText = GetLocalizedString("hub.status", "Select a destination to continue.");
    UpdateStatusMessage(statusText);
}

void Application::EnterMainInterface()
{
    interfaceState_ = InterfaceState::MainInterface;
    hubBranchHitboxes_.clear();
    hoveredHubBranchIndex_ = -1;
    focusedHubBranchIndex_ = -1;
}

bool Application::InitializeFonts()
{
    const fonts::FontConfiguration fontConfiguration = fonts::BuildFontConfiguration(settingsService_.ActiveLanguageId());
    if (fontConfiguration.primaryFontPath.empty())
    {
        std::cerr << "Unable to locate a usable font file. Provide JetBrainsMono-Regular.ttf in assets/fonts or set COLONY_FONT_PATH." << '\n';
        return false;
    }

    const ui::Typography typography = themeManager_.ActiveScheme().typography;
    frontend::fonts::LoadFontSetParams fontParams{typography, fontConfiguration};

    const auto openRoleFont = [&](frontend::fonts::FontRole role, int size) -> sdl::FontHandle {
        if (size <= 0)
        {
            return {};
        }

        std::filesystem::path path = frontend::fonts::ResolveFontForRole(role, fontParams);
        if (path.empty())
        {
            path = fontConfiguration.primaryFontPath;
        }

        return sdl::FontHandle{TTF_OpenFont(path.string().c_str(), ui::ScaleDynamic(size))};
    };

    const auto openFontPath = [&](const std::string& path, int size) -> sdl::FontHandle {
        if (size <= 0 || path.empty())
        {
            return {};
        }
        return sdl::FontHandle{TTF_OpenFont(path.c_str(), ui::ScaleDynamic(size))};
    };

    fonts_.brand = openRoleFont(frontend::fonts::FontRole::Headline, typography.headline.size);
    fonts_.navigation = openRoleFont(frontend::fonts::FontRole::Label, typography.label.size);
    fonts_.channel = openRoleFont(frontend::fonts::FontRole::Title, typography.title.size);
    fonts_.tileTitle = openRoleFont(frontend::fonts::FontRole::Title, typography.title.size);
    fonts_.tileSubtitle = openRoleFont(frontend::fonts::FontRole::Body, typography.body.size);
    fonts_.tileMeta = openRoleFont(frontend::fonts::FontRole::Caption, typography.caption.size);
    fonts_.heroTitle = openRoleFont(frontend::fonts::FontRole::Display, typography.display.size);
    fonts_.heroSubtitle = openRoleFont(frontend::fonts::FontRole::Subtitle, typography.subtitle.size);
    fonts_.heroBody = openRoleFont(frontend::fonts::FontRole::Body, typography.body.size);
    fonts_.patchTitle = openRoleFont(frontend::fonts::FontRole::Subtitle, typography.subtitle.size);
    fonts_.patchBody = openRoleFont(frontend::fonts::FontRole::Caption, typography.caption.size);
    fonts_.button = openRoleFont(frontend::fonts::FontRole::Label, typography.label.size);
    fonts_.status = openRoleFont(frontend::fonts::FontRole::Caption, std::max(typography.caption.size - 1, 12));

    if (!fonts_.brand || !fonts_.navigation || !fonts_.channel || !fonts_.tileTitle || !fonts_.tileSubtitle || !fonts_.tileMeta
        || !fonts_.heroTitle || !fonts_.heroSubtitle || !fonts_.heroBody || !fonts_.patchTitle || !fonts_.patchBody
        || !fonts_.button || !fonts_.status)
    {
        std::cerr << "Failed to load required fonts from " << fontConfiguration.primaryFontPath << ": " << TTF_GetError()
                  << '\n';
        return false;
    }

    languageFonts_.clear();
    constexpr int kBodyFontPointSize = 16;

    for (const auto& [languageId, fontPath] : fontConfiguration.nativeLanguageFonts)
    {
        if (fontPath == fontConfiguration.primaryFontPath)
        {
            continue;
        }

        sdl::FontHandle fontHandle = openFontPath(fontPath, kBodyFontPointSize);
        if (!fontHandle)
        {
            std::cerr << "Warning: failed to load language font for '" << languageId << "' from " << fontPath << ": "
                      << TTF_GetError() << '\n';
            continue;
        }

        languageFonts_.emplace(languageId, std::move(fontHandle));
    }

    return true;
}

bool Application::LoadContent()
{
    try
    {
        content_ = LoadContentFromFile(ResolveContentPath().string());
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << '\n';
        return false;
    }

    if (content_.channels.empty())
    {
        std::cerr << "No channels defined in content file." << '\n';
        return false;
    }

    channelSelections_.assign(content_.channels.size(), 0);
    EnsureLocalAppsChannel();
    return true;
}

bool Application::InitializeLocalization()
{
    localizationManager_.SetResourceDirectory(ResolveLocalizationDirectory());
    localizationManager_.SetFallbackLanguage("en");

    const std::string currentLanguage = settingsService_.ActiveLanguageId();
    if (!localizationManager_.LoadLanguage(currentLanguage))
    {
        std::cerr << "Failed to load localization for language '" << currentLanguage << "'." << '\n';
        if (currentLanguage != localizationManager_.FallbackLanguage()
            && localizationManager_.LoadLanguage(localizationManager_.FallbackLanguage()))
        {
            settingsService_.SetActiveLanguageId(localizationManager_.FallbackLanguage());
        }
        else
        {
            return false;
        }
    }

    return true;
}

void Application::SyncNavigationEntries()
{
    std::vector<std::string> entries;
    entries.reserve(content_.channels.size());
    for (const auto& channel : content_.channels)
    {
        entries.emplace_back(channel.id);
    }

    navigationController_.SetEntries(std::move(entries));
}

void Application::InitializeNavigation()
{
    SyncNavigationEntries();
    navigationController_.OnSelectionChanged([this](int index) { ActivateChannel(index); });
    ActivateChannel(navigationController_.ActiveIndex());
}

void Application::InitializeViews()
{
    for (const auto& [id, view] : content_.views)
    {
        if (IsSettingsProgramId(id))
        {
            continue;
        }
        viewRegistry_.Register(viewFactory_.CreateSimpleTextView(id));
    }
    viewRegistry_.BindContent(content_);
}

void Application::InitializeInputRouter()
{
    navigationInputHandler_.Register(inputRouter_);
    hubInputHandler_.Register(inputRouter_);
    dialogInputHandler_.Register(inputRouter_);
    libraryInputHandler_.Register(inputRouter_);
}

void Application::RebuildTheme()
{
    const int previousSettingsScrollOffset = settingsScrollOffset_;

    const auto themeData = themeService_.BuildTheme(settingsService_);
    theme_ = themeData.theme;
    typography_ = themeData.typography;
    interactions_ = themeData.interactions;
    motion_ = themeData.motion;

    const auto localize = [this](std::string_view key) { return GetLocalizedString(key); };

    navigationRail_.Build(
        rendererHost_.Renderer(),
        fonts_.brand.get(),
        fonts_.navigation.get(),
        fonts_.tileMeta.get(),
        content_,
        theme_,
        typography_);

    libraryPanel_.Build(rendererHost_.Renderer(), fonts_.tileMeta.get(), theme_, localize);
    heroPanel_.Build(rendererHost_.Renderer(), fonts_.tileMeta.get(), theme_, localize);

    std::string searchPlaceholder = localize("library.filter_placeholder");
    if (searchPlaceholder.empty())
    {
        searchPlaceholder = localize("library.filter_label");
    }
    if (searchPlaceholder.empty())
    {
        searchPlaceholder = "Search";
    }

    topBar_.Build(
        rendererHost_.Renderer(),
        fonts_.heroSubtitle.get(),
        fonts_.tileMeta.get(),
        theme_,
        typography_,
        searchPlaceholder,
        ResolveTopBarTitle());
    UpdateTopBarTitle();
    settingsPanel_.Build(
        rendererHost_.Renderer(),
        fonts_.heroTitle.get(),
        fonts_.heroBody.get(),
        theme_.heroTitle,
        theme_.heroBody,
        themeManager_,
        localize,
        [this](std::string_view languageId) -> TTF_Font* {
            const auto it = languageFonts_.find(std::string{languageId});
            if (it != languageFonts_.end())
            {
                return it->second.get();
            }
            return fonts_.heroBody.get();
        });
    settingsScrollOffset_ = std::max(0, previousSettingsScrollOffset);

    BuildHubPanel();
    RebuildProgramVisuals();
    UpdateStatusMessage(statusBuffer_.empty() && !activeProgramId_.empty()
            ? content_.views.at(activeProgramId_).statusMessage
            : statusBuffer_);

    viewContext_.renderer = rendererHost_.Renderer();
    viewContext_.headingFont = fonts_.heroTitle.get();
    viewContext_.paragraphFont = fonts_.heroBody.get();
    viewContext_.buttonFont = fonts_.button.get();
    viewContext_.primaryColor = theme_.heroTitle;
    viewContext_.mutedColor = theme_.heroBody;
    UpdateViewContextAccent();

    if (!activeProgramId_.empty() && !IsSettingsProgramId(activeProgramId_))
    {
        viewRegistry_.Activate(activeProgramId_, viewContext_);
    }
    else
    {
        viewRegistry_.DeactivateActive();
    }

    if (addAppDialog_.visible)
    {
        RefreshAddAppDialogEntries();
    }
}

void Application::RebuildProgramVisuals()
{
    programVisuals_.clear();
    const SDL_Color heroSubtitleColor = color::Mix(theme_.heroBody, theme_.heroTitle, 0.35f);

    for (const auto& [id, view] : content_.views)
    {
        programVisuals_.emplace(
            id,
            ui::BuildProgramVisuals(
                view,
                rendererHost_.Renderer(),
                fonts_.heroTitle.get(),
                fonts_.heroSubtitle.get(),
                fonts_.heroBody.get(),
                fonts_.button.get(),
                fonts_.tileTitle.get(),
                fonts_.tileSubtitle.get(),
                fonts_.tileMeta.get(),
                fonts_.patchTitle.get(),
                fonts_.patchBody.get(),
                fonts_.status.get(),
                theme_.heroTitle,
                theme_.heroBody,
                heroSubtitleColor,
                theme_.muted,
                theme_.statusBarText,
                theme_.heroGradientFallbackStart,
                theme_.heroGradientFallbackEnd));
    }
}

void Application::UpdateTopBarTitle()
{
    if (!rendererHost_.Renderer() || !fonts_.heroSubtitle)
    {
        return;
    }

    const std::string title = ResolveTopBarTitle();
    topBar_.UpdateTitle(rendererHost_.Renderer(), title, theme_.heroTitle);
}

std::string Application::ResolveTopBarTitle() const
{
    if (IsSettingsProgramId(activeProgramId_))
    {
        return GetLocalizedString("navigation.settings", "Settings");
    }

    if (activeChannelIndex_ >= 0 && activeChannelIndex_ < static_cast<int>(content_.channels.size()))
    {
        return content_.channels[activeChannelIndex_].label;
    }

    if (!content_.brandName.empty())
    {
        return content_.brandName;
    }

    return GetLocalizedString("navigation.dashboard", "Dashboard");
}

void Application::ActivateChannel(int index)
{
    if (index < 0 || index >= static_cast<int>(content_.channels.size()))
    {
        return;
    }

    activeChannelIndex_ = index;
    const std::string programId = GetActiveProgramId();
    ActivateProgram(programId);
}

void Application::ActivateProgram(const std::string& programId)
{
    if (programId.empty())
    {
        activeProgramId_.clear();
        heroActionRect_.reset();
        viewRegistry_.DeactivateActive();
        return;
    }

    const std::string previousProgramId = activeProgramId_;
    const bool wasSettingsProgram = IsSettingsProgramId(previousProgramId);
    activeProgramId_ = programId;

    if (IsSettingsProgramId(activeProgramId_))
    {
        const bool programChanged = !wasSettingsProgram || previousProgramId != activeProgramId_;
        if (programChanged)
        {
            const std::string_view targetSectionId = SettingsSectionForProgram(activeProgramId_);
            settingsSectionStates_.appearanceExpanded = targetSectionId == ui::SettingsPanel::kAppearanceSectionId;
            settingsSectionStates_.languageExpanded = targetSectionId == ui::SettingsPanel::kLanguageSectionId;
            settingsSectionStates_.generalExpanded = targetSectionId == ui::SettingsPanel::kGeneralSectionId;
            if (targetSectionId.empty())
            {
                settingsSectionStates_.appearanceExpanded = true;
                settingsSectionStates_.languageExpanded = true;
                settingsSectionStates_.generalExpanded = true;
                pendingSettingsSectionId_.reset();
            }
            else
            {
                pendingSettingsSectionId_ = std::string{targetSectionId};
            }

            settingsScrollOffset_ = 0;
            if (!targetSectionId.empty())
            {
                const auto anchorIt = std::find_if(
                    settingsRenderResult_.sectionAnchors.begin(),
                    settingsRenderResult_.sectionAnchors.end(),
                    [&](const ui::SettingsPanel::RenderResult::SectionAnchor& anchor) {
                        return anchor.id == targetSectionId;
                    });
                if (anchorIt != settingsRenderResult_.sectionAnchors.end())
                {
                    settingsScrollOffset_ = anchorIt->offset;
                }
            }
        }

        viewRegistry_.DeactivateActive();
        UpdateStatusMessage(content_.views[activeProgramId_].statusMessage);
        UpdateViewContextAccent();
        return;
    }

    pendingSettingsSectionId_.reset();

    if (const auto visualsIt = programVisuals_.find(activeProgramId_); visualsIt != programVisuals_.end())
    {
        UpdateStatusMessage(visualsIt->second.content->statusMessage);
        viewContext_.accentColor = visualsIt->second.accent;
        viewRegistry_.Activate(activeProgramId_, viewContext_);
    }
    else
    {
        viewRegistry_.DeactivateActive();
    }

    UpdateViewContextAccent();
    UpdateTopBarTitle();
}

void Application::ActivateProgramInChannel(int programIndex)
{
    if (activeChannelIndex_ < 0 || activeChannelIndex_ >= static_cast<int>(content_.channels.size()))
    {
        return;
    }

    auto& selection = channelSelections_[activeChannelIndex_];
    const auto& channel = content_.channels[activeChannelIndex_];
    if (channel.programs.empty())
    {
        selection = 0;
        ActivateProgram({});
        return;
    }

    const int clamped = std::clamp(programIndex, 0, static_cast<int>(channel.programs.size()) - 1);
    selection = clamped;
    ActivateProgram(channel.programs[clamped]);
}

std::string Application::GetActiveProgramId() const
{
    if (activeChannelIndex_ < 0 || activeChannelIndex_ >= static_cast<int>(content_.channels.size()))
    {
        return {};
    }

    const auto& channel = content_.channels[activeChannelIndex_];
    if (channel.programs.empty())
    {
        return {};
    }

    const int clamped = std::clamp(channelSelections_[activeChannelIndex_], 0, static_cast<int>(channel.programs.size()) - 1);
    return channel.programs[clamped];
}

bool Application::UpdateCustomizationValueFromPosition(const std::string& id, int mouseX)
{
    const auto it = std::find_if(
        settingsRenderResult_.interactiveRegions.begin(),
        settingsRenderResult_.interactiveRegions.end(),
        [&](const ui::SettingsPanel::RenderResult::InteractiveRegion& region) {
            return region.type == ui::SettingsPanel::RenderResult::InteractionType::Customization && region.id == id;
        });

    if (it == settingsRenderResult_.interactiveRegions.end())
    {
        return false;
    }

    const float newValue = ComputeCustomizationSliderValue(it->rect, mouseX);
    if (SetAppearanceCustomizationValue(id, newValue))
    {
        RebuildTheme();
        return true;
    }

    return false;
}

void Application::BeginResizeDrag(int x, int y, bool adjustNavRail)
{
    (void)y;

    if (!rendererHost_.Renderer())
    {
        return;
    }

    resizeState_.target = adjustNavRail ? ResizeState::Target::NavRail : ResizeState::Target::Library;
    resizeState_.startX = x;
    resizeState_.initialNavWidth = navRailWidth_;
    resizeState_.initialLibraryWidth = libraryWidth_;
    layoutSizesInitialized_ = true;
    SDL_CaptureMouse(SDL_TRUE);
}

void Application::EndResizeDrag()
{
    resizeState_.target = ResizeState::Target::None;
    resizeState_.startX = 0;
    SDL_CaptureMouse(SDL_FALSE);
}

void Application::UpdateResizeDrag(int x)
{
    if (!rendererHost_.Renderer() || resizeState_.target == ResizeState::Target::None)
    {
        return;
    }

    const platform::RendererDimensions outputDimensions = rendererHost_.OutputSize();
    const int outputWidth = outputDimensions.width;
    const int outputHeight = outputDimensions.height;

    if (resizeState_.target == ResizeState::Target::NavRail)
    {
        const int delta = x - resizeState_.startX;
        navRailWidth_ = resizeState_.initialNavWidth + delta;
    }
    else if (resizeState_.target == ResizeState::Target::Library)
    {
        const int delta = x - resizeState_.startX;
        libraryWidth_ = resizeState_.initialLibraryWidth + delta;
    }

    UpdateLayoutForOutputWidth(outputWidth);
}

void Application::UpdateLayoutForOutputWidth(int outputWidth)
{
    if (outputWidth <= 0)
    {
        return;
    }

    const int navMin = ui::Scale(120);
    const int navMax = ui::Scale(200);
    const int libraryMin = ui::Scale(220);
    const int libraryMax = ui::Scale(560);
    const int heroMin = ui::Scale(220);

    if (navRailWidth_ <= 0)
    {
        navRailWidth_ = ui::Scale(140);
    }

    if (libraryWidth_ <= 0)
    {
        const int defaultLibrary = std::clamp(outputWidth / 4, ui::Scale(220), ui::Scale(320));
        libraryWidth_ = defaultLibrary;
    }

    const int maxNavAllowed = std::max(navMin, std::min(navMax, outputWidth - libraryMin - heroMin));
    navRailWidth_ = std::clamp(navRailWidth_, navMin, maxNavAllowed);

    const int maxLibraryAllowed = std::max(libraryMin, std::min(libraryMax, outputWidth - navRailWidth_ - heroMin));
    libraryWidth_ = std::clamp(libraryWidth_, libraryMin, maxLibraryAllowed);

    int heroSpace = outputWidth - navRailWidth_ - libraryWidth_;
    if (heroSpace < heroMin)
    {
        const int deficit = heroMin - heroSpace;
        const int reducibleLibrary = std::max(0, libraryWidth_ - libraryMin);
        const int libraryReduction = std::min(deficit, reducibleLibrary);
        libraryWidth_ -= libraryReduction;

        const int remainingDeficit = deficit - libraryReduction;
        if (remainingDeficit > 0)
        {
            const int reducibleNav = std::max(0, navRailWidth_ - navMin);
            const int navReduction = std::min(remainingDeficit, reducibleNav);
            navRailWidth_ -= navReduction;
        }
    }

    heroSpace = outputWidth - navRailWidth_ - libraryWidth_;
    if (heroSpace < heroMin)
    {
        libraryWidth_ = std::max(0, outputWidth - navRailWidth_ - heroMin);
        heroSpace = outputWidth - navRailWidth_ - libraryWidth_;
    }

    if (heroSpace < 0)
    {
        heroSpace = 0;
    }

    navRailWidth_ = std::clamp(navRailWidth_, std::max(0, navMin), std::min(navMax, std::max(navMin, outputWidth - heroMin)));
    libraryWidth_ = std::clamp(libraryWidth_, 0, std::min(libraryMax, std::max(0, outputWidth - navRailWidth_ - heroMin)));

    layoutSizesInitialized_ = true;
}


} // namespace colony

