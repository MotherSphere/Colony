#include "app/application.hpp"

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

namespace
{
inline int AddDialogRowHeight()
{
    return ui::Scale(40);
}

constexpr int kAddDialogCornerRadius = 18;

struct AddDialogSortOption
{
    const char* label;
};

struct AddDialogFileTypeFilter
{
    const char* label;
    std::vector<std::string> extensions;
    bool includeDirectories = false;
    bool directoriesOnly = false;
    bool requireExecutablePermission = false;
};

const std::vector<AddDialogSortOption>& GetAddDialogSortOptions()
{
    static const std::vector<AddDialogSortOption> kOptions = {
        {"Name (A→Z)"},
        {"Name (Z→A)"},
        {"Modified (newest first)"},
        {"Modified (oldest first)"},
    };
    return kOptions;
}

const std::vector<AddDialogFileTypeFilter>& GetAddDialogFileTypeFilters()
{
    static const std::vector<AddDialogFileTypeFilter> kFilters = [] {
        auto makeFilter = [](const char* label, const char* extension) {
            std::vector<std::string> normalized;
            if (extension && *extension)
            {
                std::string value = extension;
                if (!value.empty() && value.front() != '.')
                {
                    value.insert(value.begin(), '.');
                }
                std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
                    return static_cast<char>(std::tolower(ch));
                });
                normalized.emplace_back(std::move(value));
            }
            return AddDialogFileTypeFilter{label, std::move(normalized), true, false, false};
        };

        std::vector<AddDialogFileTypeFilter> filters;
        filters.emplace_back(AddDialogFileTypeFilter{"All files (*.*)", {}, true, false, false});
        filters.emplace_back(AddDialogFileTypeFilter{"Folders", {}, true, true, false});

#if defined(_WIN32)
        filters.emplace_back(makeFilter("Executable (*.exe)", ".exe"));
        filters.emplace_back(makeFilter("Batch script (*.bat)", ".bat"));
        filters.emplace_back(makeFilter("Command script (*.cmd)", ".cmd"));
        filters.emplace_back(makeFilter("Dynamic library (*.dll)", ".dll"));
#else
        AddDialogFileTypeFilter executableFilter{"Executable files", {}, true, false, true};
        filters.emplace_back(std::move(executableFilter));
        filters.emplace_back(makeFilter("Shell script (*.sh)", ".sh"));
        filters.emplace_back(makeFilter("Run package (*.run)", ".run"));
        filters.emplace_back(makeFilter("Binary file (*.bin)", ".bin"));
        filters.emplace_back(makeFilter("AppImage (*.AppImage)", ".appimage"));
#if defined(__APPLE__)
        filters.emplace_back(makeFilter("Mac application (*.app)", ".app"));
        filters.emplace_back(makeFilter("Mac package (*.pkg)", ".pkg"));
        filters.emplace_back(makeFilter("Dynamic library (*.dylib)", ".dylib"));
#else
        filters.emplace_back(makeFilter("Shared object (*.so)", ".so"));
#endif
#endif
        return filters;
    }();
    return kFilters;
}

}

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

    const int navMin = ui::Scale(72);
    const int navMax = ui::Scale(160);
    const int libraryMin = ui::Scale(220);
    const int libraryMax = ui::Scale(560);
    const int heroMin = ui::Scale(220);

    if (navRailWidth_ <= 0)
    {
        navRailWidth_ = ui::Scale(88);
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

void Application::BuildHubPanel()
{
    SDL_Renderer* renderer = rendererHost_.Renderer();
    if (!renderer)
    {
        return;
    }

    const auto& hubConfig = content_.hub;

    hubSearchTokens_ = TokenizeHubSearch(hubSearchQuery_);

    ui::panels::HubContent hubContent;
    hubContent.searchPlaceholder = GetLocalizedString("hub.search.placeholder", "Rechercher une destination");

    if (!hubConfig.headlineLocalizationKey.empty())
    {
        hubContent.headline = GetLocalizedString(hubConfig.headlineLocalizationKey, hubConfig.headlineLocalizationKey);
    }
    if (hubContent.headline.empty())
    {
        hubContent.headline = content_.brandName.empty() ? std::string{"COLONY"} : content_.brandName;
    }

    if (!hubConfig.descriptionLocalizationKey.empty())
    {
        hubContent.description = GetLocalizedString(
            hubConfig.descriptionLocalizationKey,
            hubConfig.descriptionLocalizationKey);
    }
    if (hubContent.description.empty())
    {
        hubContent.description = GetLocalizedString("hub.status", "Select a destination to continue.");
    }

    for (const auto& highlightKey : hubConfig.highlightLocalizationKeys)
    {
        if (!highlightKey.empty())
        {
            hubContent.highlights.push_back(GetLocalizedString(highlightKey, highlightKey));
        }
    }

    hubRenderedBranchIds_.clear();
    hubContent.branches.reserve(hubConfig.branches.size());

    for (std::size_t index = 0; index < hubConfig.branches.size(); ++index)
    {
        const auto& branch = hubConfig.branches[index];

        std::string title = branch.titleLocalizationKey.empty()
            ? branch.id
            : GetLocalizedString(branch.titleLocalizationKey, branch.id);
        std::string description = branch.descriptionLocalizationKey.empty()
            ? branch.id
            : GetLocalizedString(branch.descriptionLocalizationKey, branch.descriptionLocalizationKey);

        std::vector<std::string> tags;
        tags.reserve(branch.tagLocalizationKeys.size());
        for (const auto& tagKey : branch.tagLocalizationKeys)
        {
            if (!tagKey.empty())
            {
                tags.push_back(GetLocalizedString(tagKey, tagKey));
            }
        }

        std::string metricsText = branch.metricsLocalizationKey.empty()
            ? std::string{}
            : GetLocalizedString(branch.metricsLocalizationKey, branch.metricsLocalizationKey);

        std::string haystack = title;
        haystack.push_back(' ');
        haystack += description;
        for (const auto& tag : tags)
        {
            haystack.push_back(' ');
            haystack += tag;
        }
        if (!metricsText.empty())
        {
            haystack.push_back(' ');
            haystack += metricsText;
        }
        const std::string normalizedHaystack = NormalizeHubSearchString(haystack);
        bool matchesQuery = true;
        for (const auto& token : hubSearchTokens_)
        {
            if (normalizedHaystack.find(token) == std::string::npos)
            {
                matchesQuery = false;
                break;
            }
        }
        if (!matchesQuery)
        {
            continue;
        }

        ui::panels::HubBranchContent branchContent;
        branchContent.id = branch.id;
        branchContent.title = title;
        branchContent.description = description;
        branchContent.accent = branch.accentColor.empty() ? theme_.channelBadge
                                                         : color::ParseHexColor(branch.accentColor, theme_.channelBadge);
        branchContent.tags = tags;
        branchContent.actionLabel = branch.actionLocalizationKey.empty()
            ? GetLocalizedString("hub.branch.default_action", "Open")
            : GetLocalizedString(branch.actionLocalizationKey, branchContent.title);
        branchContent.metrics = metricsText;

        if (!branch.channelId.empty())
        {
            auto channelIt = std::find_if(
                content_.channels.begin(),
                content_.channels.end(),
                [&](const Channel& channel) { return channel.id == branch.channelId; });
            if (channelIt != content_.channels.end())
            {
                branchContent.channelLabel = std::string{"Canal : "} + channelIt->label;
            }
        }

        if (!branch.programId.empty())
        {
            branchContent.programLabel = std::string{"Programme : "} + branch.programId;
        }

        branchContent.detailBullets.reserve(branchContent.tags.size());
        for (const auto& tag : branchContent.tags)
        {
            branchContent.detailBullets.push_back(std::string{"#"} + tag);
        }

        hubContent.branches.emplace_back(std::move(branchContent));
        hubRenderedBranchIds_.push_back(branch.id);
    }

    if (hubContent.highlights.empty())
    {
        const int count = static_cast<int>(hubContent.branches.size());
        if (!hubSearchTokens_.empty())
        {
            hubContent.highlights.push_back(std::to_string(count) + (count == 1 ? " résultat" : " résultats"));
        }
        else
        {
            const int total = static_cast<int>(hubConfig.branches.size());
            hubContent.highlights.push_back(std::to_string(total) + (total == 1 ? " destination" : " destinations"));
        }
    }

    if (!hubConfig.primaryActionLocalizationKey.empty())
    {
        hubContent.primaryActionLabel =
            GetLocalizedString(hubConfig.primaryActionLocalizationKey, hubConfig.primaryActionLocalizationKey);
    }
    if (!hubConfig.primaryActionDescriptionLocalizationKey.empty())
    {
        hubContent.primaryActionDescription = GetLocalizedString(
            hubConfig.primaryActionDescriptionLocalizationKey,
            hubConfig.primaryActionDescriptionLocalizationKey);
    }

    hubContent.widgets.reserve(hubConfig.widgets.size());
    for (const auto& widget : hubConfig.widgets)
    {
        ui::panels::HubWidgetContent widgetContent;
        widgetContent.id = widget.id;
        widgetContent.title = widget.titleLocalizationKey.empty() ? widget.id
                                                                  : GetLocalizedString(widget.titleLocalizationKey, widget.id);
        widgetContent.description = widget.descriptionLocalizationKey.empty()
            ? std::string{}
            : GetLocalizedString(widget.descriptionLocalizationKey, widget.descriptionLocalizationKey);
        for (const auto& itemKey : widget.itemLocalizationKeys)
        {
            if (!itemKey.empty())
            {
                widgetContent.items.push_back(GetLocalizedString(itemKey, itemKey));
            }
        }
        widgetContent.accent = widget.accentColor.empty() ? theme_.channelBadge
                                                          : color::ParseHexColor(widget.accentColor, theme_.channelBadge);
        hubContent.widgets.emplace_back(std::move(widgetContent));
    }

    const int widgetPageCount = hubContent.widgets.empty()
        ? 0
        : (static_cast<int>(hubContent.widgets.size()) + hubWidgetsPerPage_ - 1) / hubWidgetsPerPage_;
    hubWidgetPageCount_ = widgetPageCount;
    if (widgetPageCount == 0)
    {
        hubWidgetPage_ = 0;
    }
    else
    {
        hubWidgetPage_ = std::clamp(hubWidgetPage_, 0, widgetPageCount - 1);
    }

    if (focusedHubBranchIndex_ >= static_cast<int>(hubContent.branches.size()))
    {
        focusedHubBranchIndex_ = hubContent.branches.empty() ? -1 : 0;
    }
    if (hubContent.branches.empty())
    {
        hoveredHubBranchIndex_ = -1;
    }

    hubPanel_.Build(
        renderer,
        hubContent,
        fonts_.heroTitle.get(),
        fonts_.heroBody.get(),
        fonts_.tileTitle.get(),
        fonts_.tileSubtitle.get(),
        theme_);

    EnsureHubScrollWithinBounds();
}

void Application::ResetHubInteractionState()
{
    hoveredHubBranchIndex_ = -1;
    focusedHubBranchIndex_ = -1;
    hubBranchHitboxes_.clear();
    hubRenderedBranchIds_.clear();
    hubSearchTokens_.clear();
    hubScrollOffset_ = 0;
    hubScrollMaxOffset_ = 0;
    hubScrollViewportValid_ = false;
    hubSearchFocused_ = false;
    hubSearchQuery_.clear();
    hubWidgetPage_ = 0;
    hubWidgetPageCount_ = 0;
    hubWidgetPagerHitboxes_.clear();
    hubSearchInputRect_.reset();
    hubSearchClearRect_.reset();
    hubHeroToggleRect_.reset();
    hubDetailActionRect_.reset();
}

std::vector<std::string> Application::TokenizeHubSearch(std::string_view value) const
{
    std::vector<std::string> tokens;
    std::string normalized = NormalizeHubSearchString(value);
    std::string current;
    current.reserve(normalized.size());
    for (char ch : normalized)
    {
        if (ch == ' ')
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
        }
        else
        {
            current.push_back(ch);
        }
    }
    if (!current.empty())
    {
        tokens.push_back(current);
    }
    return tokens;
}

std::string Application::NormalizeHubSearchString(std::string_view value) const
{
    std::string normalized;
    normalized.reserve(value.size());
    bool previousSpace = false;
    for (unsigned char raw : value)
    {
        if (std::isalnum(raw) != 0)
        {
            normalized.push_back(static_cast<char>(std::tolower(raw)));
            previousSpace = false;
        }
        else if (std::isspace(raw) != 0)
        {
            if (!previousSpace && !normalized.empty())
            {
                normalized.push_back(' ');
                previousSpace = true;
            }
        }
        else
        {
            if (!previousSpace && !normalized.empty())
            {
                normalized.push_back(' ');
                previousSpace = true;
            }
        }
    }
    if (!normalized.empty() && normalized.back() == ' ')
    {
        normalized.pop_back();
    }
    return normalized;
}

void Application::EnsureHubScrollWithinBounds()
{
    hubScrollOffset_ = std::clamp(hubScrollOffset_, 0, std::max(0, hubScrollMaxOffset_));
}

void Application::FocusHubSearch()
{
    hubSearchFocused_ = true;
    UpdateTextInputState();
}

void Application::ClearHubSearchQuery()
{
    hubSearchQuery_.clear();
    hubSearchTokens_.clear();
    hubScrollOffset_ = 0;
}

void Application::SyncFocusedHubBranch()
{
    hoveredHubBranchIndex_ = focusedHubBranchIndex_;
    if (!hubScrollViewportValid_ || focusedHubBranchIndex_ < 0)
    {
        return;
    }

    auto it = std::find_if(
        hubBranchHitboxes_.begin(),
        hubBranchHitboxes_.end(),
        [&](const ui::panels::HubRenderResult::BranchHitbox& hitbox) { return hitbox.branchIndex == focusedHubBranchIndex_; });
    if (it == hubBranchHitboxes_.end())
    {
        return;
    }

    const SDL_Rect& rect = it->rect;
    const int viewportTop = hubScrollViewport_.y;
    const int viewportBottom = hubScrollViewport_.y + hubScrollViewport_.h;
    if (rect.y < viewportTop)
    {
        hubScrollOffset_ = std::clamp(hubScrollOffset_ - (viewportTop - rect.y), 0, hubScrollMaxOffset_);
    }
    else if (rect.y + rect.h > viewportBottom)
    {
        hubScrollOffset_ = std::clamp(hubScrollOffset_ + (rect.y + rect.h - viewportBottom), 0, hubScrollMaxOffset_);
    }
}

void Application::HandleHubMouseWheel(const SDL_MouseWheelEvent& wheel)
{
    if (!hubScrollViewportValid_)
    {
        return;
    }

    int mouseX = 0;
    int mouseY = 0;
    SDL_GetMouseState(&mouseX, &mouseY);
    if (!PointInRect(hubScrollViewport_, mouseX, mouseY))
    {
        return;
    }

    const int scrollStep = ui::Scale(96);
    hubScrollOffset_ = std::clamp(
        hubScrollOffset_ - wheel.y * scrollStep,
        0,
        std::max(0, hubScrollMaxOffset_));
}

void Application::HandleHubMouseClick(int x, int y)
{
    bool handled = false;

    if (hubHeroToggleRect_ && PointInRect(*hubHeroToggleRect_, x, y))
    {
        isHubHeroCollapsed_ = !isHubHeroCollapsed_;
        handled = true;
        BuildHubPanel();
    }

    if (!handled && hubSearchClearRect_ && PointInRect(*hubSearchClearRect_, x, y) && !hubSearchQuery_.empty())
    {
        ClearHubSearchQuery();
        BuildHubPanel();
        handled = true;
    }

    if (!handled && hubSearchInputRect_ && PointInRect(*hubSearchInputRect_, x, y))
    {
        FocusHubSearch();
        handled = true;
    }

    if (!handled)
    {
        for (const auto& pagerHitbox : hubWidgetPagerHitboxes_)
        {
            if (!pagerHitbox.enabled)
            {
                continue;
            }
            if (PointInRect(pagerHitbox.rect, x, y))
            {
                const int maxPage = std::max(0, hubWidgetPageCount_ - 1);
                switch (pagerHitbox.type)
                {
                case ui::panels::HubRenderResult::WidgetPagerHitbox::Type::Previous:
                case ui::panels::HubRenderResult::WidgetPagerHitbox::Type::Next:
                case ui::panels::HubRenderResult::WidgetPagerHitbox::Type::Page:
                    hubWidgetPage_ = std::clamp(pagerHitbox.pageIndex, 0, maxPage);
                    break;
                }
                handled = true;
                break;
            }
        }
    }

    if (!handled && hubDetailActionRect_ && PointInRect(*hubDetailActionRect_, x, y))
    {
        if (focusedHubBranchIndex_ >= 0)
        {
            ActivateHubBranchByIndex(focusedHubBranchIndex_);
        }
        handled = true;
    }

    if (!handled)
    {
        for (const auto& hitbox : hubBranchHitboxes_)
        {
            if (PointInRect(hitbox.rect, x, y))
            {
                focusedHubBranchIndex_ = hitbox.branchIndex;
                hoveredHubBranchIndex_ = hitbox.branchIndex;
                hubSearchFocused_ = false;
                UpdateTextInputState();
                handled = true;
                break;
            }
        }
    }

    if (!handled)
    {
        hubSearchFocused_ = false;
    }

    if (!handled && hubSearchFocused_)
    {
        FocusHubSearch();
    }
    else if (!hubSearchFocused_)
    {
        UpdateTextInputState();
    }
}

void Application::HandleHubMouseMotion(const SDL_MouseMotionEvent& motion)
{
    int hoveredIndex = -1;
    for (const auto& hitbox : hubBranchHitboxes_)
    {
        if (PointInRect(hitbox.rect, motion.x, motion.y))
        {
            hoveredIndex = hitbox.branchIndex;
            break;
        }
    }
    hoveredHubBranchIndex_ = hoveredIndex;
}

bool Application::HandleHubKeyDown(SDL_Keycode key)
{
    const int branchCount = static_cast<int>(hubRenderedBranchIds_.size());

    switch (key)
    {
    case SDLK_ESCAPE:
        if (hubSearchFocused_)
        {
            if (!hubSearchQuery_.empty())
            {
                ClearHubSearchQuery();
                BuildHubPanel();
            }
            else
            {
                hubSearchFocused_ = false;
                UpdateTextInputState();
            }
            return true;
        }
        EnterMainInterface();
        return true;
    case SDLK_BACKSPACE:
        if (hubSearchFocused_)
        {
            if (!hubSearchQuery_.empty())
            {
                auto eraseIt = hubSearchQuery_.end();
                do
                {
                    --eraseIt;
                } while (eraseIt != hubSearchQuery_.begin() && ((*eraseIt & 0xC0) == 0x80));
                hubSearchQuery_.erase(eraseIt, hubSearchQuery_.end());
                BuildHubPanel();
            }
            else
            {
                hubSearchFocused_ = false;
                UpdateTextInputState();
            }
            return true;
        }
        EnterMainInterface();
        return true;
    case SDLK_SLASH:
        if (!hubSearchFocused_)
        {
            FocusHubSearch();
            return true;
        }
        break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        if (hubSearchFocused_)
        {
            hubSearchFocused_ = false;
            UpdateTextInputState();
            return true;
        }
        if (focusedHubBranchIndex_ >= 0 && focusedHubBranchIndex_ < branchCount)
        {
            ActivateHubBranchByIndex(focusedHubBranchIndex_);
        }
        return true;
    case SDLK_SPACE:
        if (hubSearchFocused_)
        {
            return false;
        }
        if (focusedHubBranchIndex_ >= 0 && focusedHubBranchIndex_ < branchCount)
        {
            ActivateHubBranchByIndex(focusedHubBranchIndex_);
        }
        return true;
    case SDLK_PAGEDOWN:
        if (hubScrollViewportValid_)
        {
            hubScrollOffset_ = std::clamp(hubScrollOffset_ + hubScrollViewport_.h, 0, hubScrollMaxOffset_);
            return true;
        }
        break;
    case SDLK_PAGEUP:
        if (hubScrollViewportValid_)
        {
            hubScrollOffset_ = std::clamp(hubScrollOffset_ - hubScrollViewport_.h, 0, hubScrollMaxOffset_);
            return true;
        }
        break;
    case SDLK_HOME:
        hubScrollOffset_ = 0;
        if (branchCount > 0)
        {
            focusedHubBranchIndex_ = 0;
            SyncFocusedHubBranch();
        }
        return true;
    case SDLK_END:
        hubScrollOffset_ = hubScrollMaxOffset_;
        if (branchCount > 0)
        {
            focusedHubBranchIndex_ = branchCount - 1;
            SyncFocusedHubBranch();
        }
        return true;
    case SDLK_LEFT:
    case SDLK_UP:
        if (branchCount == 0)
        {
            return true;
        }
        if (focusedHubBranchIndex_ < 0)
        {
            focusedHubBranchIndex_ = branchCount - 1;
        }
        else
        {
            focusedHubBranchIndex_ = (focusedHubBranchIndex_ - 1 + branchCount) % branchCount;
        }
        SyncFocusedHubBranch();
        return true;
    case SDLK_RIGHT:
    case SDLK_DOWN:
        if (branchCount == 0)
        {
            return true;
        }
        if (focusedHubBranchIndex_ < 0)
        {
            focusedHubBranchIndex_ = 0;
        }
        else
        {
            focusedHubBranchIndex_ = (focusedHubBranchIndex_ + 1) % branchCount;
        }
        SyncFocusedHubBranch();
        return true;
    case SDLK_TAB:
        if (branchCount == 0)
        {
            return true;
        }
        if ((SDL_GetModState() & KMOD_SHIFT) != 0)
        {
            if (focusedHubBranchIndex_ < 0)
            {
                focusedHubBranchIndex_ = branchCount - 1;
            }
            else
            {
                focusedHubBranchIndex_ = (focusedHubBranchIndex_ - 1 + branchCount) % branchCount;
            }
        }
        else
        {
            if (focusedHubBranchIndex_ < 0)
            {
                focusedHubBranchIndex_ = 0;
            }
            else
            {
                focusedHubBranchIndex_ = (focusedHubBranchIndex_ + 1) % branchCount;
            }
        }
        SyncFocusedHubBranch();
        return true;
    default:
        break;
    }

    return false;
}


void Application::ActivateHubBranch(const std::string& branchId)
{
    const int index = FindHubBranchIndexById(branchId);
    if (index < 0)
    {
        return;
    }

    auto visibleIt = std::find(hubRenderedBranchIds_.begin(), hubRenderedBranchIds_.end(), branchId);
    if (visibleIt != hubRenderedBranchIds_.end())
    {
        focusedHubBranchIndex_ = static_cast<int>(std::distance(hubRenderedBranchIds_.begin(), visibleIt));
    }
    else
    {
        focusedHubBranchIndex_ = -1;
    }
    const auto& branch = content_.hub.branches[static_cast<std::size_t>(index)];

    EnterMainInterface();

    int targetChannelIndex = -1;
    if (!branch.channelId.empty())
    {
        for (std::size_t i = 0; i < content_.channels.size(); ++i)
        {
            if (content_.channels[i].id == branch.channelId)
            {
                targetChannelIndex = static_cast<int>(i);
                break;
            }
        }
    }

    auto findChannelForProgram = [&](const std::string& programId) {
        for (std::size_t i = 0; i < content_.channels.size(); ++i)
        {
            const auto& channel = content_.channels[i];
            if (std::find(channel.programs.begin(), channel.programs.end(), programId) != channel.programs.end())
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    };

    const bool hasProgramTarget = !branch.programId.empty();
    if (targetChannelIndex == -1 && hasProgramTarget)
    {
        targetChannelIndex = findChannelForProgram(branch.programId);
    }

    if (targetChannelIndex != -1)
    {
        navigationController_.Activate(targetChannelIndex);
        if (hasProgramTarget)
        {
            auto& channel = content_.channels[static_cast<std::size_t>(targetChannelIndex)];
            auto it = std::find(channel.programs.begin(), channel.programs.end(), branch.programId);
            if (it != channel.programs.end())
            {
                const int programIndex = static_cast<int>(std::distance(channel.programs.begin(), it));
                channelSelections_[targetChannelIndex] = programIndex;
                ActivateProgramInChannel(programIndex);
            }
            else
            {
                ActivateProgram(branch.programId);
            }
        }
    }
    else if (hasProgramTarget)
    {
        ActivateProgram(branch.programId);
    }
}

void Application::ActivateHubBranchByIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(hubRenderedBranchIds_.size()))
    {
        return;
    }

    ActivateHubBranch(hubRenderedBranchIds_[static_cast<std::size_t>(index)]);
}

int Application::FindHubBranchIndexById(const std::string& branchId) const
{
    for (std::size_t i = 0; i < content_.hub.branches.size(); ++i)
    {
        if (content_.hub.branches[i].id == branchId)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::string Application::ColorToHex(SDL_Color color)
{
    std::ostringstream stream;
    stream << '#' << std::uppercase << std::hex << std::setfill('0') << std::setw(2)
           << static_cast<int>(color.r) << std::setw(2) << static_cast<int>(color.g) << std::setw(2)
           << static_cast<int>(color.b);
    return stream.str();
}

std::string Application::MakeDisplayNameFromPath(const std::filesystem::path& path)
{
    std::string name = path.stem().string();
    if (name.empty())
    {
        name = path.filename().string();
    }
    if (name.empty())
    {
        name = "Application";
    }
    return name;
}

void Application::ShowEditUserAppDialog(const std::string& programId)
{
    auto viewIt = content_.views.find(programId);
    if (viewIt == content_.views.end())
    {
        return;
    }

    HideAddAppDialog();

    editAppDialog_.visible = true;
    editAppDialog_.programId = programId;
    editAppDialog_.errorMessage.clear();
    editAppDialog_.nameInput = viewIt->second.heading;
    editAppDialog_.nameFocused = true;
    editAppDialog_.colorFocused = false;

    std::string colorValue = viewIt->second.accentColor;
    if (colorValue.empty())
    {
        colorValue = ColorToHex(theme_.channelBadge);
    }
    if (!colorValue.empty() && colorValue.front() != '#')
    {
        colorValue.insert(colorValue.begin(), '#');
    }
    std::transform(colorValue.begin(), colorValue.end(), colorValue.begin(), [](unsigned char ch) {
        return ch == '#' ? '#' : static_cast<char>(std::toupper(ch));
    });
    editAppDialog_.colorInput = colorValue;

    UpdateTextInputState();
}

void Application::HideEditUserAppDialog()
{
    if (!editAppDialog_.visible)
    {
        return;
    }

    editAppDialog_.visible = false;
    editAppDialog_.programId.clear();
    editAppDialog_.nameFocused = false;
    editAppDialog_.colorFocused = false;
    editAppDialog_.errorMessage.clear();
    UpdateTextInputState();
}

void Application::ShowCustomThemeDialog()
{
    HideAddAppDialog();
    HideEditUserAppDialog();

    customThemeDialog_.visible = true;
    customThemeDialog_.errorMessage.clear();
    customThemeDialog_.focusedIndex = 0;
    customThemeDialog_.nameInput.clear();
    customThemeDialog_.panelRect = SDL_Rect{0, 0, 0, 0};
    customThemeDialog_.nameFieldRect = SDL_Rect{0, 0, 0, 0};
    customThemeDialog_.saveButtonRect = SDL_Rect{0, 0, 0, 0};
    customThemeDialog_.cancelButtonRect = SDL_Rect{0, 0, 0, 0};
    std::fill(customThemeDialog_.colorFieldRects.begin(), customThemeDialog_.colorFieldRects.end(), SDL_Rect{0, 0, 0, 0});
    std::fill(
        customThemeDialog_.colorFieldContentOffsets.begin(),
        customThemeDialog_.colorFieldContentOffsets.end(),
        0);
    customThemeDialog_.colorFieldViewport = SDL_Rect{0, 0, 0, 0};
    customThemeDialog_.scrollOffset = 0;
    customThemeDialog_.colorFieldContentHeight = 0;

    const ui::ColorScheme& activeScheme = themeManager_.ActiveScheme();
    const auto& customThemeFields = services::CustomThemeFields();
    for (std::size_t index = 0; index < customThemeFields.size(); ++index)
    {
        SDL_Color color = activeScheme.colors.*(customThemeFields[index].member);
        customThemeDialog_.colorInputs[index] = ColorToHex(color);
    }

    UpdateTextInputState();
}

void Application::HideCustomThemeDialog()
{
    if (!customThemeDialog_.visible)
    {
        return;
    }

    customThemeDialog_.visible = false;
    customThemeDialog_.focusedIndex = -1;
    customThemeDialog_.errorMessage.clear();
    customThemeDialog_.colorFieldViewport = SDL_Rect{0, 0, 0, 0};
    customThemeDialog_.colorFieldContentHeight = 0;
    customThemeDialog_.scrollOffset = 0;
    std::fill(
        customThemeDialog_.colorFieldContentOffsets.begin(),
        customThemeDialog_.colorFieldContentOffsets.end(),
        0);
    UpdateTextInputState();
}

void Application::RenderCustomThemeDialog(double timeSeconds)
{
    if (!customThemeDialog_.visible)
    {
        return;
    }

    SDL_Renderer* renderer = rendererHost_.Renderer();
    if (!renderer)
    {
        return;
    }
    SDL_BlendMode previousBlendMode = SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(renderer, &previousBlendMode);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    const platform::RendererDimensions outputDimensions = rendererHost_.OutputSize();
    int outputWidth = outputDimensions.width;
    int outputHeight = outputDimensions.height;

    SDL_Rect overlayRect{0, 0, outputWidth, outputHeight};
    SDL_SetRenderDrawColor(renderer, 6, 10, 26, 208);
    SDL_RenderFillRect(renderer, &overlayRect);

    const int panelPadding = ui::Scale(26);
    int panelWidth = std::min(outputWidth - ui::Scale(220), ui::Scale(880));
    panelWidth = std::max(panelWidth, ui::Scale(620));
    const int maxAvailableHeight = outputHeight - ui::Scale(140);
    int panelHeight = std::min(maxAvailableHeight, ui::Scale(720));
    const int minPanelHeight = std::min(maxAvailableHeight, ui::Scale(560));
    panelHeight = std::max(panelHeight, minPanelHeight);

    SDL_Rect panelRect{
        overlayRect.x + (overlayRect.w - panelWidth) / 2,
        overlayRect.y + (overlayRect.h - panelHeight) / 2,
        panelWidth,
        panelHeight};
    customThemeDialog_.panelRect = panelRect;

    SDL_Color panelFill = color::Mix(theme_.libraryCardActive, theme_.background, 0.38f);
    SDL_SetRenderDrawColor(renderer, panelFill.r, panelFill.g, panelFill.b, panelFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, panelRect, kAddDialogCornerRadius);
    SDL_SetRenderDrawColor(renderer, theme_.border.r, theme_.border.g, theme_.border.b, theme_.border.a);
    colony::drawing::RenderRoundedRect(renderer, panelRect, kAddDialogCornerRadius);

    int cursorX = panelRect.x + panelPadding;
    int cursorY = panelRect.y + panelPadding;

    std::string titleText = GetLocalizedString("settings.appearance.custom_theme.dialog.title", "Create custom colors");
    TextTexture titleTexture = CreateTextTexture(renderer, fonts_.heroTitle.get(), titleText, theme_.heroTitle);
    if (titleTexture.texture)
    {
        SDL_Rect titleRect{cursorX, cursorY, titleTexture.width, titleTexture.height};
        RenderTexture(renderer, titleTexture, titleRect);
        cursorY += titleRect.h + ui::Scale(6);
    }

    std::string subtitleText = GetLocalizedString(
        "settings.appearance.custom_theme.button.description",
        "Define each interface color manually.");
    TextTexture subtitleTexture = CreateTextTexture(renderer, fonts_.tileSubtitle.get(), subtitleText, theme_.muted);
    if (subtitleTexture.texture)
    {
        SDL_Rect subtitleRect{cursorX, cursorY, subtitleTexture.width, subtitleTexture.height};
        RenderTexture(renderer, subtitleTexture, subtitleRect);
        cursorY += subtitleRect.h + ui::Scale(18);
    }

    std::string nameLabelText = GetLocalizedString(
        "settings.appearance.custom_theme.dialog.name_label",
        "Scheme name");
    TextTexture nameLabel = CreateTextTexture(renderer, fonts_.tileSubtitle.get(), nameLabelText, theme_.muted);
    if (nameLabel.texture)
    {
        SDL_Rect labelRect{cursorX, cursorY, nameLabel.width, nameLabel.height};
        RenderTexture(renderer, nameLabel, labelRect);
        cursorY += labelRect.h + ui::Scale(6);
    }

    const int fieldHeight = ui::Scale(44);
    customThemeDialog_.nameFieldRect = SDL_Rect{cursorX, cursorY, panelRect.w - 2 * panelPadding, fieldHeight};
    const bool nameFocused = customThemeDialog_.focusedIndex == 0;
    SDL_Color nameFill = nameFocused ? color::Mix(theme_.libraryCardActive, theme_.background, 0.6f)
                                     : color::Mix(theme_.libraryCard, theme_.background, 0.55f);
    SDL_SetRenderDrawColor(renderer, nameFill.r, nameFill.g, nameFill.b, nameFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, customThemeDialog_.nameFieldRect, 12);
    SDL_Color nameBorder = nameFocused ? theme_.channelBadge : theme_.border;
    SDL_SetRenderDrawColor(renderer, nameBorder.r, nameBorder.g, nameBorder.b, nameBorder.a);
    colony::drawing::RenderRoundedRect(renderer, customThemeDialog_.nameFieldRect, 12);

    SDL_Rect nameClip{
        customThemeDialog_.nameFieldRect.x + ui::Scale(12),
        customThemeDialog_.nameFieldRect.y,
        customThemeDialog_.nameFieldRect.w - ui::Scale(24),
        customThemeDialog_.nameFieldRect.h};
    SDL_RenderSetClipRect(renderer, &nameClip);

    const bool hasName = !customThemeDialog_.nameInput.empty();
    std::string namePlaceholder = GetLocalizedString(
        "settings.appearance.custom_theme.dialog.name_placeholder",
        "Enter a name");
    TextTexture nameValueTexture = CreateTextTexture(
        renderer,
        fonts_.tileSubtitle.get(),
        hasName ? customThemeDialog_.nameInput : namePlaceholder,
        hasName ? theme_.heroTitle : theme_.muted);
    if (nameValueTexture.texture)
    {
        SDL_Rect valueRect{
            nameClip.x,
            customThemeDialog_.nameFieldRect.y + (customThemeDialog_.nameFieldRect.h - nameValueTexture.height) / 2,
            nameValueTexture.width,
            nameValueTexture.height};
        RenderTexture(renderer, nameValueTexture, valueRect);
    }

    SDL_RenderSetClipRect(renderer, nullptr);

    if (nameFocused)
    {
        const bool caretVisible = std::fmod(timeSeconds, 1.0) < 0.5;
        if (caretVisible)
        {
            const int caretOffset = hasName ? nameValueTexture.width : 0;
            const int caretX = nameClip.x + caretOffset + ui::Scale(2);
            SDL_Rect caretClip{
                nameClip.x,
                nameClip.y + ui::Scale(6),
                nameClip.w,
                nameClip.h - ui::Scale(12)};
            SDL_RenderSetClipRect(renderer, &caretClip);
            SDL_SetRenderDrawColor(renderer, theme_.heroTitle.r, theme_.heroTitle.g, theme_.heroTitle.b, theme_.heroTitle.a);
            SDL_RenderDrawLine(
                renderer,
                caretX,
                customThemeDialog_.nameFieldRect.y + ui::Scale(6),
                caretX,
                customThemeDialog_.nameFieldRect.y + customThemeDialog_.nameFieldRect.h - ui::Scale(6));
            SDL_RenderSetClipRect(renderer, nullptr);
        }
    }

    cursorY += fieldHeight + ui::Scale(24);

    const int buttonSpacing = ui::Scale(14);
    const int buttonWidth = ui::Scale(170);
    const int buttonHeight = ui::Scale(48);
    const int buttonAreaTop = panelRect.y + panelRect.h - panelPadding - buttonHeight;
    const int viewportBottomPadding = ui::Scale(32);
    const int availableViewportHeight = std::max(0, buttonAreaTop - viewportBottomPadding - cursorY);
    const int labelHeightEstimate = fonts_.tileSubtitle ? TTF_FontHeight(fonts_.tileSubtitle.get()) : ui::Scale(18);
    const int estimatedRowHeight = labelHeightEstimate + ui::Scale(6) + fieldHeight + ui::Scale(20);
    int fieldsViewportHeight = availableViewportHeight;
    if (estimatedRowHeight > 0)
    {
        const int desiredHeight = estimatedRowHeight * 5;
        if (fieldsViewportHeight > desiredHeight)
        {
            fieldsViewportHeight = desiredHeight;
        }
    }

    SDL_Rect fieldsViewport{
        panelRect.x + panelPadding,
        cursorY,
        panelRect.w - 2 * panelPadding,
        fieldsViewportHeight};
    bool viewportValid = fieldsViewport.w > 0 && fieldsViewport.h > 0;
    if (!viewportValid)
    {
        fieldsViewport = SDL_Rect{0, 0, 0, 0};
    }
    customThemeDialog_.colorFieldViewport = fieldsViewport;

    std::fill(customThemeDialog_.colorFieldRects.begin(), customThemeDialog_.colorFieldRects.end(), SDL_Rect{0, 0, 0, 0});

    if (viewportValid)
    {
        SDL_RenderSetClipRect(renderer, &fieldsViewport);
    }

    const int columns = 2;
    const int columnSpacing = ui::Scale(22);
    const int columnWidth = (panelRect.w - 2 * panelPadding - columnSpacing * (columns - 1)) / columns;
    std::array<int, columns> columnOffsets{};
    columnOffsets.fill(0);

    const auto& customThemeFields = services::CustomThemeFields();
    for (std::size_t index = 0; index < customThemeFields.size(); ++index)
    {
        const auto& field = customThemeFields[index];
        const int column = static_cast<int>(index % columns);
        const int fieldX = panelRect.x + panelPadding + column * (columnWidth + columnSpacing);
        int localOffset = columnOffsets[column];

        std::string labelText = GetLocalizedString(field.localizationKey, field.id);
        TextTexture labelTexture = CreateTextTexture(renderer, fonts_.tileSubtitle.get(), labelText, theme_.muted);
        if (viewportValid && labelTexture.texture)
        {
            SDL_Rect labelRect{
                fieldX,
                fieldsViewport.y + localOffset - customThemeDialog_.scrollOffset,
                labelTexture.width,
                labelTexture.height};
            RenderTexture(renderer, labelTexture, labelRect);
        }

        int fieldLocalTop = localOffset;
        if (labelTexture.texture)
        {
            fieldLocalTop += labelTexture.height + ui::Scale(6);
        }
        customThemeDialog_.colorFieldContentOffsets[index] = fieldLocalTop;

        SDL_Rect fieldRect{
            fieldX,
            fieldsViewport.y + fieldLocalTop - customThemeDialog_.scrollOffset,
            columnWidth,
            fieldHeight};

        if (viewportValid)
        {
            SDL_Rect visibleRect{};
            if (SDL_IntersectRect(&fieldRect, &fieldsViewport, &visibleRect) == SDL_TRUE)
            {
                customThemeDialog_.colorFieldRects[index] = visibleRect;
                const bool colorFocused = customThemeDialog_.focusedIndex == static_cast<int>(index) + 1;
                SDL_Color colorFill = colorFocused ? color::Mix(theme_.libraryCardActive, theme_.background, 0.6f)
                                                   : color::Mix(theme_.libraryCard, theme_.background, 0.55f);
                SDL_SetRenderDrawColor(renderer, colorFill.r, colorFill.g, colorFill.b, colorFill.a);
                colony::drawing::RenderFilledRoundedRect(renderer, fieldRect, 12);
                SDL_Color colorBorder = colorFocused ? theme_.channelBadge : theme_.border;
                SDL_SetRenderDrawColor(renderer, colorBorder.r, colorBorder.g, colorBorder.b, colorBorder.a);
                colony::drawing::RenderRoundedRect(renderer, fieldRect, 12);

                const int previewSize = ui::Scale(22);
                SDL_Rect previewRect{
                    fieldRect.x + fieldRect.w - previewSize - ui::Scale(8),
                    fieldRect.y + (fieldRect.h - previewSize) / 2,
                    previewSize,
                    previewSize};
                SDL_Color previewColor = color::ParseHexColor(customThemeDialog_.colorInputs[index], theme_.channelBadge);
                SDL_SetRenderDrawColor(renderer, previewColor.r, previewColor.g, previewColor.b, previewColor.a);
                colony::drawing::RenderFilledRoundedRect(renderer, previewRect, 8);
                SDL_SetRenderDrawColor(renderer, theme_.border.r, theme_.border.g, theme_.border.b, theme_.border.a);
                colony::drawing::RenderRoundedRect(renderer, previewRect, 8);

                SDL_Rect textClip{
                    fieldRect.x + ui::Scale(10),
                    fieldRect.y,
                    fieldRect.w - previewSize - ui::Scale(28),
                    fieldRect.h};
                SDL_Rect textClipIntersection{};
                if (SDL_IntersectRect(&textClip, &fieldsViewport, &textClipIntersection) == SDL_TRUE)
                {
                    SDL_RenderSetClipRect(renderer, &textClipIntersection);

                    const std::string& fieldValue = customThemeDialog_.colorInputs[index];
                    const bool hasValue = !fieldValue.empty();
                    TextTexture valueTexture = CreateTextTexture(
                        renderer,
                        fonts_.tileSubtitle.get(),
                        hasValue ? fieldValue : std::string{"#RRGGBB"},
                        hasValue ? theme_.heroTitle : theme_.muted);
                    if (valueTexture.texture)
                    {
                        SDL_Rect valueRect{
                            textClipIntersection.x,
                            fieldRect.y + (fieldRect.h - valueTexture.height) / 2,
                            valueTexture.width,
                            valueTexture.height};
                        RenderTexture(renderer, valueTexture, valueRect);
                    }

                    if (colorFocused)
                    {
                        const bool caretVisible = std::fmod(timeSeconds, 1.0) < 0.5;
                        if (caretVisible)
                        {
                            const int caretOffset = (!customThemeDialog_.colorInputs[index].empty() && valueTexture.texture)
                                                        ? valueTexture.width
                                                        : 0;
                            const int caretX = textClip.x + caretOffset + ui::Scale(2);
                            SDL_Rect caretClip{
                                textClip.x,
                                fieldRect.y + ui::Scale(6),
                                textClip.w,
                                fieldRect.h - ui::Scale(12)};
                            SDL_Rect caretClipIntersection{};
                            if (SDL_IntersectRect(&caretClip, &fieldsViewport, &caretClipIntersection) == SDL_TRUE)
                            {
                                SDL_RenderSetClipRect(renderer, &caretClipIntersection);
                                SDL_SetRenderDrawColor(
                                    renderer,
                                    theme_.heroTitle.r,
                                    theme_.heroTitle.g,
                                    theme_.heroTitle.b,
                                    theme_.heroTitle.a);
                                SDL_RenderDrawLine(
                                    renderer,
                                    caretX,
                                    caretClipIntersection.y,
                                    caretX,
                                    caretClipIntersection.y + caretClipIntersection.h);
                            }
                        }
                    }

                    SDL_RenderSetClipRect(renderer, &fieldsViewport);
                }
            }
        }

        columnOffsets[column] = fieldLocalTop + fieldHeight + ui::Scale(20);
    }

    if (viewportValid)
    {
        SDL_RenderSetClipRect(renderer, nullptr);
    }

    int contentHeight = 0;
    for (int value : columnOffsets)
    {
        contentHeight = std::max(contentHeight, value);
    }
    customThemeDialog_.colorFieldContentHeight = contentHeight;
    if (viewportValid)
    {
        const int maxScroll = std::max(0, customThemeDialog_.colorFieldContentHeight - fieldsViewport.h);
        customThemeDialog_.scrollOffset = std::clamp(customThemeDialog_.scrollOffset, 0, maxScroll);
    }
    else
    {
        customThemeDialog_.scrollOffset = 0;
    }

    if (viewportValid)
    {
        cursorY = fieldsViewport.y + fieldsViewport.h + ui::Scale(18);
    }
    else
    {
        cursorY += ui::Scale(18);
    }

    if (!customThemeDialog_.errorMessage.empty())
    {
        TextTexture errorTexture = CreateTextTexture(
            renderer,
            fonts_.tileSubtitle.get(),
            customThemeDialog_.errorMessage,
            theme_.channelBadge);
        if (errorTexture.texture)
        {
            SDL_Rect errorRect{cursorX, cursorY, errorTexture.width, errorTexture.height};
            RenderTexture(renderer, errorTexture, errorRect);
            cursorY += errorRect.h + ui::Scale(10);
        }
    }

    customThemeDialog_.saveButtonRect = SDL_Rect{
        panelRect.x + panelRect.w - panelPadding - buttonWidth,
        panelRect.y + panelRect.h - panelPadding - buttonHeight,
        buttonWidth,
        buttonHeight};
    customThemeDialog_.cancelButtonRect = SDL_Rect{
        customThemeDialog_.saveButtonRect.x - buttonSpacing - buttonWidth,
        customThemeDialog_.saveButtonRect.y,
        buttonWidth,
        buttonHeight};

    SDL_Color saveFill = color::Mix(theme_.channelBadge, theme_.libraryCardActive, 0.4f);
    SDL_SetRenderDrawColor(renderer, saveFill.r, saveFill.g, saveFill.b, saveFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, customThemeDialog_.saveButtonRect, 14);
    SDL_SetRenderDrawColor(renderer, theme_.border.r, theme_.border.g, theme_.border.b, theme_.border.a);
    colony::drawing::RenderRoundedRect(renderer, customThemeDialog_.saveButtonRect, 14);

    SDL_Color cancelFill = color::Mix(theme_.libraryCard, theme_.libraryBackground, 0.6f);
    SDL_SetRenderDrawColor(renderer, cancelFill.r, cancelFill.g, cancelFill.b, cancelFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, customThemeDialog_.cancelButtonRect, 14);
    SDL_SetRenderDrawColor(renderer, theme_.border.r, theme_.border.g, theme_.border.b, theme_.border.a);
    colony::drawing::RenderRoundedRect(renderer, customThemeDialog_.cancelButtonRect, 14);

    TextTexture saveLabel = CreateTextTexture(
        renderer,
        fonts_.button.get(),
        GetLocalizedString("settings.appearance.custom_theme.dialog.save", "Save palette"),
        theme_.heroTitle);
    if (saveLabel.texture)
    {
        SDL_Rect saveRect{
            customThemeDialog_.saveButtonRect.x
                + (customThemeDialog_.saveButtonRect.w - saveLabel.width) / 2,
            customThemeDialog_.saveButtonRect.y
                + (customThemeDialog_.saveButtonRect.h - saveLabel.height) / 2,
            saveLabel.width,
            saveLabel.height};
        RenderTexture(renderer, saveLabel, saveRect);
    }

    TextTexture cancelLabel = CreateTextTexture(
        renderer,
        fonts_.button.get(),
        GetLocalizedString("settings.appearance.custom_theme.dialog.cancel", "Cancel"),
        theme_.heroTitle);
    if (cancelLabel.texture)
    {
        SDL_Rect cancelRect{
            customThemeDialog_.cancelButtonRect.x
                + (customThemeDialog_.cancelButtonRect.w - cancelLabel.width) / 2,
            customThemeDialog_.cancelButtonRect.y
                + (customThemeDialog_.cancelButtonRect.h - cancelLabel.height) / 2,
            cancelLabel.width,
            cancelLabel.height};
        RenderTexture(renderer, cancelLabel, cancelRect);
    }

    SDL_SetRenderDrawBlendMode(renderer, previousBlendMode);
}

bool Application::HandleCustomThemeDialogMouseWheel(const SDL_MouseWheelEvent& wheel)
{
    if (!customThemeDialog_.visible)
    {
        return false;
    }

    if (customThemeDialog_.colorFieldViewport.w <= 0 || customThemeDialog_.colorFieldViewport.h <= 0)
    {
        return true;
    }

    int mouseX = 0;
    int mouseY = 0;
    SDL_GetMouseState(&mouseX, &mouseY);
    if (!PointInRect(customThemeDialog_.colorFieldViewport, mouseX, mouseY))
    {
        return true;
    }

    int wheelY = wheel.y;
    if (wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
    {
        wheelY = -wheelY;
    }

    if (wheelY == 0)
    {
        return true;
    }

    const int maxScroll = std::max(0, customThemeDialog_.colorFieldContentHeight - customThemeDialog_.colorFieldViewport.h);
    if (maxScroll <= 0)
    {
        return true;
    }

    const int fieldHeight = ui::Scale(44);
    const int labelHeightEstimate = fonts_.tileSubtitle ? TTF_FontHeight(fonts_.tileSubtitle.get()) : ui::Scale(18);
    const int rowStride = labelHeightEstimate + ui::Scale(6) + fieldHeight + ui::Scale(20);
    const int scrollStep = std::max(rowStride, ui::Scale(40));

    customThemeDialog_.scrollOffset = std::clamp(
        customThemeDialog_.scrollOffset - wheelY * scrollStep,
        0,
        maxScroll);
    return true;
}

bool Application::HandleCustomThemeDialogMouseClick(int x, int y)
{
    if (!customThemeDialog_.visible)
    {
        return false;
    }

    if (!PointInRect(customThemeDialog_.panelRect, x, y))
    {
        HideCustomThemeDialog();
        return true;
    }

    if (PointInRect(customThemeDialog_.cancelButtonRect, x, y))
    {
        HideCustomThemeDialog();
        return true;
    }

    if (PointInRect(customThemeDialog_.saveButtonRect, x, y))
    {
        ApplyCustomThemeDialog();
        return true;
    }

    if (PointInRect(customThemeDialog_.nameFieldRect, x, y))
    {
        if (customThemeDialog_.focusedIndex != 0)
        {
            customThemeDialog_.focusedIndex = 0;
            customThemeDialog_.errorMessage.clear();
            UpdateTextInputState();
        }
        return true;
    }

    for (std::size_t index = 0; index < customThemeDialog_.colorFieldRects.size(); ++index)
    {
        if (PointInRect(customThemeDialog_.colorFieldRects[index], x, y))
        {
            const int desiredFocus = static_cast<int>(index) + 1;
            if (customThemeDialog_.focusedIndex != desiredFocus)
            {
                customThemeDialog_.focusedIndex = desiredFocus;
                customThemeDialog_.errorMessage.clear();
                UpdateTextInputState();
                EnsureCustomThemeFieldVisible(desiredFocus);
            }
            return true;
        }
    }

    return PointInRect(customThemeDialog_.panelRect, x, y);
}

bool Application::HandleCustomThemeDialogKey(SDL_Keycode key)
{
    if (!customThemeDialog_.visible)
    {
        return false;
    }

    switch (key)
    {
    case SDLK_ESCAPE:
        HideCustomThemeDialog();
        return true;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        ApplyCustomThemeDialog();
        return true;
    case SDLK_TAB:
    {
    const int focusable = 1 + static_cast<int>(services::CustomThemeFields().size());
        if (focusable <= 0)
        {
            return true;
        }
        int current = customThemeDialog_.focusedIndex;
        if (current < 0)
        {
            current = 0;
        }
        const bool reverse = (SDL_GetModState() & KMOD_SHIFT) != 0;
        if (reverse)
        {
            current = (current - 1 + focusable) % focusable;
        }
        else
        {
            current = (current + 1) % focusable;
        }
        customThemeDialog_.focusedIndex = current;
        customThemeDialog_.errorMessage.clear();
        UpdateTextInputState();
        EnsureCustomThemeFieldVisible(current);
        return true;
    }
    case SDLK_BACKSPACE:
    {
        if (customThemeDialog_.focusedIndex == 0)
        {
            if (!customThemeDialog_.nameInput.empty())
            {
                customThemeDialog_.nameInput.pop_back();
            }
        }
        else if (customThemeDialog_.focusedIndex > 0)
        {
            const std::size_t colorIndex = static_cast<std::size_t>(customThemeDialog_.focusedIndex - 1);
            if (colorIndex < customThemeDialog_.colorInputs.size())
            {
                std::string& value = customThemeDialog_.colorInputs[colorIndex];
                if (!value.empty())
                {
                    value.pop_back();
                    if (value == "#")
                    {
                        value.clear();
                    }
                }
            }
        }
        customThemeDialog_.errorMessage.clear();
        return true;
    }
    default:
        break;
    }

    return false;
}

bool Application::HandleCustomThemeDialogText(const SDL_TextInputEvent& text)
{
    if (!customThemeDialog_.visible)
    {
        return false;
    }

    const char* input = text.text;
    if (input == nullptr || *input == '\0')
    {
        return false;
    }

    customThemeDialog_.errorMessage.clear();

    if (customThemeDialog_.focusedIndex == 0)
    {
        constexpr std::size_t kMaxNameLength = 60;
        const std::size_t currentLength = customThemeDialog_.nameInput.size();
        const std::size_t incomingLength = std::strlen(input);
        if (currentLength < kMaxNameLength)
        {
            customThemeDialog_.nameInput.append(input, std::min(incomingLength, kMaxNameLength - currentLength));
        }
        return true;
    }

    if (customThemeDialog_.focusedIndex <= 0)
    {
        return false;
    }

    const std::size_t colorIndex = static_cast<std::size_t>(customThemeDialog_.focusedIndex - 1);
    if (colorIndex >= customThemeDialog_.colorInputs.size())
    {
        return false;
    }

    std::string& value = customThemeDialog_.colorInputs[colorIndex];
    constexpr std::size_t kMaxColorLength = 7; // # + RRGGBB
    for (std::size_t i = 0; input[i] != '\0'; ++i)
    {
        unsigned char ch = static_cast<unsigned char>(input[i]);
        if (ch == '#')
        {
            if (value.empty())
            {
                value.push_back('#');
            }
            continue;
        }

        if (std::isxdigit(ch) == 0)
        {
            continue;
        }

        if (value.empty())
        {
            value.push_back('#');
        }

        if (value.size() >= kMaxColorLength)
        {
            continue;
        }

        value.push_back(static_cast<char>(std::toupper(ch)));
    }

    return true;
}

void Application::EnsureCustomThemeFieldVisible(int focusIndex)
{
    if (focusIndex <= 0)
    {
        return;
    }

    const int colorIndex = focusIndex - 1;
    if (colorIndex < 0 || colorIndex >= static_cast<int>(customThemeDialog_.colorFieldContentOffsets.size()))
    {
        return;
    }

    if (customThemeDialog_.colorFieldViewport.h <= 0)
    {
        return;
    }

    const int maxScroll = std::max(0, customThemeDialog_.colorFieldContentHeight - customThemeDialog_.colorFieldViewport.h);
    const int fieldHeight = ui::Scale(44);
    const int fieldTop = customThemeDialog_.colorFieldContentOffsets[colorIndex];
    const int fieldBottom = fieldTop + fieldHeight;
    const int viewportTop = customThemeDialog_.scrollOffset;
    const int viewportBottom = viewportTop + customThemeDialog_.colorFieldViewport.h;

    int desiredOffset = customThemeDialog_.scrollOffset;
    if (fieldTop < viewportTop)
    {
        desiredOffset = fieldTop;
    }
    else if (fieldBottom > viewportBottom)
    {
        desiredOffset = fieldBottom - customThemeDialog_.colorFieldViewport.h;
    }

    if (maxScroll <= 0)
    {
        customThemeDialog_.scrollOffset = 0;
        return;
    }

    customThemeDialog_.scrollOffset = std::clamp(desiredOffset, 0, maxScroll);
}

bool Application::ApplyCustomThemeDialog()
{
    if (!customThemeDialog_.visible)
    {
        return false;
    }

    std::string trimmedName = TrimString(customThemeDialog_.nameInput);
    if (trimmedName.empty())
    {
        customThemeDialog_.errorMessage = GetLocalizedString(
            "settings.appearance.custom_theme.dialog.errors.name_required",
            "Provide a scheme name.");
        customThemeDialog_.focusedIndex = 0;
        UpdateTextInputState();
        EnsureCustomThemeFieldVisible(customThemeDialog_.focusedIndex);
        return false;
    }

    std::array<std::string, CustomThemeDialogState::kColorFieldCount> normalizedInputs{};
    std::array<SDL_Color, CustomThemeDialogState::kColorFieldCount> parsedColors{};

    const auto& customThemeFields = services::CustomThemeFields();
    for (std::size_t index = 0; index < customThemeFields.size(); ++index)
    {
        std::string value = TrimString(customThemeDialog_.colorInputs[index]);
        if (value.empty())
        {
            customThemeDialog_.errorMessage = GetLocalizedString(
                "settings.appearance.custom_theme.dialog.errors.colors_required",
                "Set a value for every color.");
            customThemeDialog_.focusedIndex = static_cast<int>(index) + 1;
            UpdateTextInputState();
            EnsureCustomThemeFieldVisible(customThemeDialog_.focusedIndex);
            return false;
        }

        if (value.front() != '#')
        {
            value.insert(value.begin(), '#');
        }

        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return ch == '#' ? '#' : static_cast<char>(std::toupper(ch));
        });

        if (!IsValidHexColor(value))
        {
            customThemeDialog_.errorMessage = GetLocalizedString(
                "settings.appearance.custom_theme.dialog.errors.color_format",
                "Use #RGB or #RRGGBB color codes.");
            customThemeDialog_.focusedIndex = static_cast<int>(index) + 1;
            UpdateTextInputState();
            EnsureCustomThemeFieldVisible(customThemeDialog_.focusedIndex);
            return false;
        }

        normalizedInputs[index] = value;
        parsedColors[index] = color::ParseHexColor(value, theme_.heroTitle);
    }

    auto normalizedLower = [](std::string value) {
        value = TrimString(std::move(value));
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return value;
    };

    std::string normalizedName = normalizedLower(trimmedName);
    const auto& schemes = themeManager_.Schemes();
    const bool duplicateName = std::any_of(schemes.begin(), schemes.end(), [&](const ui::ColorScheme& scheme) {
        return normalizedLower(scheme.name) == normalizedName;
    });
    if (duplicateName)
    {
        customThemeDialog_.errorMessage = GetLocalizedString(
            "settings.appearance.custom_theme.dialog.errors.duplicate_name",
            "A scheme with this name already exists.");
        customThemeDialog_.focusedIndex = 0;
        UpdateTextInputState();
        EnsureCustomThemeFieldVisible(customThemeDialog_.focusedIndex);
        return false;
    }

    std::string baseId;
    baseId.reserve(trimmedName.size());
    auto appendUnderscore = [&]() {
        if (baseId.empty() || baseId.back() == '_')
        {
            return;
        }
        baseId.push_back('_');
    };
    for (unsigned char ch : trimmedName)
    {
        if (std::isalnum(ch))
        {
            baseId.push_back(static_cast<char>(std::tolower(ch)));
        }
        else if (ch == ' ' || ch == '-' || ch == '_')
        {
            appendUnderscore();
        }
    }
    if (!baseId.empty() && baseId.back() == '_')
    {
        baseId.pop_back();
    }
    if (baseId.empty())
    {
        baseId = "custom_palette";
    }

    auto idInUse = [&](const std::string& id) {
        return std::any_of(schemes.begin(), schemes.end(), [&](const ui::ColorScheme& scheme) {
            return scheme.id == id;
        });
    };

    std::string candidateId = baseId;
    int suffix = 1;
    while (idInUse(candidateId))
    {
        candidateId = baseId + '_' + std::to_string(suffix++);
    }

    ui::ColorScheme scheme;
    scheme.id = candidateId;
    scheme.name = trimmedName;
    for (std::size_t index = 0; index < customThemeFields.size(); ++index)
    {
        scheme.colors.*(customThemeFields[index].member) = parsedColors[index];
        customThemeDialog_.colorInputs[index] = normalizedInputs[index];
    }

    themeManager_.AddCustomScheme(std::move(scheme), true);
    HideCustomThemeDialog();
    RebuildTheme();
    settingsService_.Save(ResolveSettingsPath(), themeManager_);
    return true;
}

void Application::ShowAddAppDialog()
{
    HideEditUserAppDialog();
    addAppDialog_.visible = true;
    addAppDialog_.errorMessage.clear();
    addAppDialog_.entryRects.clear();
    addAppDialog_.entries.clear();
    addAppDialog_.selectedIndex = -1;
    addAppDialog_.scrollOffset = 0;
    addAppDialog_.contentHeight = 0;
    addAppDialog_.parentAvailable = false;
    addAppDialog_.searchFocused = true;
    addAppDialog_.searchQuery.clear();
    addAppDialog_.sortButtonRect = SDL_Rect{0, 0, 0, 0};
    addAppDialog_.filterButtonRect = SDL_Rect{0, 0, 0, 0};
    addAppDialog_.filterDropdownRect = SDL_Rect{0, 0, 0, 0};
    addAppDialog_.filterDropdownOptionRects.clear();
    addAppDialog_.filterDropdownOpen = false;
    addAppDialog_.filterDropdownVisible = false;
    addAppDialog_.filterDropdownOptionHeight = 0;
    addAppDialog_.filterDropdownOptionCount = 0;

    const auto& sortOptions = GetAddDialogSortOptions();
    if (addAppDialog_.sortModeIndex < 0 || addAppDialog_.sortModeIndex >= static_cast<int>(sortOptions.size()))
    {
        addAppDialog_.sortModeIndex = 0;
    }

    const auto& fileFilters = GetAddDialogFileTypeFilters();
    if (addAppDialog_.fileTypeFilterIndex < 0 || addAppDialog_.fileTypeFilterIndex >= static_cast<int>(fileFilters.size()))
    {
        addAppDialog_.fileTypeFilterIndex = 0;
    }

    if (addAppDialog_.currentDirectory.empty())
    {
        std::error_code ec;
        std::filesystem::path current = std::filesystem::current_path(ec);
        if (ec)
        {
            current = std::filesystem::path{"."};
        }
        addAppDialog_.currentDirectory = current;
    }

    RefreshAddAppDialogEntries();
    UpdateTextInputState();
}

void Application::HideAddAppDialog()
{
    addAppDialog_.visible = false;
    addAppDialog_.entries.clear();
    addAppDialog_.entryRects.clear();
    addAppDialog_.errorMessage.clear();
    addAppDialog_.parentAvailable = false;
    addAppDialog_.selectedIndex = -1;
    addAppDialog_.scrollOffset = 0;
    addAppDialog_.contentHeight = 0;
    addAppDialog_.searchFocused = false;
    addAppDialog_.sortButtonRect = SDL_Rect{0, 0, 0, 0};
    addAppDialog_.filterButtonRect = SDL_Rect{0, 0, 0, 0};
    addAppDialog_.filterDropdownRect = SDL_Rect{0, 0, 0, 0};
    addAppDialog_.filterDropdownOptionRects.clear();
    addAppDialog_.filterDropdownOpen = false;
    addAppDialog_.filterDropdownVisible = false;
    addAppDialog_.filterDropdownOptionHeight = 0;
    addAppDialog_.filterDropdownOptionCount = 0;
    UpdateTextInputState();
}

void Application::RefreshAddAppDialogEntries()
{
    const int previousScroll = addAppDialog_.scrollOffset;
    std::filesystem::path previouslySelectedPath;
    if (addAppDialog_.selectedIndex >= 0 && addAppDialog_.selectedIndex < static_cast<int>(addAppDialog_.entries.size()))
    {
        previouslySelectedPath = addAppDialog_.entries[addAppDialog_.selectedIndex].path;
    }

    addAppDialog_.entries.clear();
    addAppDialog_.entryRects.clear();
    addAppDialog_.filterDropdownOpen = false;
    addAppDialog_.filterDropdownRect = SDL_Rect{0, 0, 0, 0};
    addAppDialog_.filterDropdownOptionRects.clear();
    addAppDialog_.filterDropdownVisible = false;
    addAppDialog_.filterDropdownOptionHeight = 0;
    addAppDialog_.filterDropdownOptionCount = 0;
    addAppDialog_.contentHeight = 0;
    addAppDialog_.errorMessage.clear();
    addAppDialog_.parentAvailable = false;
    addAppDialog_.selectedIndex = -1;

    if (!addAppDialog_.visible)
    {
        return;
    }

    SDL_Renderer* renderer = rendererHost_.Renderer();
    TTF_Font* font = fonts_.heroBody.get();
    if (renderer == nullptr || font == nullptr)
    {
        return;
    }

    std::error_code ec;
    const std::filesystem::path directory = addAppDialog_.currentDirectory;
    if (directory.empty())
    {
        addAppDialog_.errorMessage = "Directory unavailable.";
        return;
    }

    if (!std::filesystem::exists(directory, ec) || !std::filesystem::is_directory(directory, ec))
    {
        addAppDialog_.errorMessage = "Directory unavailable.";
        return;
    }

    addAppDialog_.parentAvailable = directory.has_parent_path() && directory.parent_path() != directory;

    struct RawEntry
    {
        std::filesystem::path path;
        bool isDirectory = false;
        std::filesystem::file_time_type lastWriteTime{};
        bool hasWriteTime = false;
        bool isExecutable = false;
        bool hasExecutableInfo = false;
    };

    std::vector<RawEntry> directories;
    std::vector<RawEntry> files;
    bool enumeratedAny = false;

    std::string searchFilter = addAppDialog_.searchQuery;
    bool globalSearch = false;
    if (!searchFilter.empty() && searchFilter.front() == '*')
    {
        globalSearch = true;
        searchFilter.erase(searchFilter.begin());
        while (!searchFilter.empty() && std::isspace(static_cast<unsigned char>(searchFilter.front())))
        {
            searchFilter.erase(searchFilter.begin());
        }
    }

    std::transform(searchFilter.begin(), searchFilter.end(), searchFilter.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    const bool hasFilter = !searchFilter.empty();
    if (globalSearch && !hasFilter)
    {
        globalSearch = false;
    }

    auto normalizedKey = [globalSearch](const std::filesystem::path& path) {
        std::string key;
        if (globalSearch)
        {
            key = path.string();
        }
        else
        {
            key = path.filename().string();
        }
        if (key.empty())
        {
            key = path.string();
        }
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return key;
    };

    if (globalSearch && hasFilter)
    {
        constexpr std::size_t kMaxResults = 512;
        std::filesystem::path searchRoot = directory;
        if (searchRoot.has_root_path())
        {
            searchRoot = searchRoot.root_path();
            if (searchRoot.empty())
            {
                searchRoot = directory;
            }
        }
        else if (!searchRoot.is_absolute())
        {
            std::error_code rootError;
            searchRoot = std::filesystem::current_path(rootError);
            if (rootError)
            {
                searchRoot = directory;
            }
        }

        std::error_code iteratorError;
        std::filesystem::recursive_directory_iterator iterator(
            searchRoot,
            std::filesystem::directory_options::skip_permission_denied,
            iteratorError);
        std::filesystem::recursive_directory_iterator end;
        if (iteratorError)
        {
            addAppDialog_.errorMessage = "Unable to enumerate directory.";
            return;
        }

        for (; iterator != end && (directories.size() + files.size()) < kMaxResults; iterator.increment(iteratorError))
        {
            if (iteratorError)
            {
                iteratorError.clear();
                continue;
            }

            RawEntry entry;
            entry.path = iterator->path();
            std::error_code statusError;
            std::filesystem::file_status status = iterator->status(statusError);
            if (statusError)
            {
                statusError.clear();
                status = iterator->symlink_status(statusError);
            }
            if (!statusError)
            {
                entry.isDirectory = std::filesystem::is_directory(status);
            }
            else
            {
                statusError.clear();
                std::error_code dirError;
                entry.isDirectory = std::filesystem::is_directory(entry.path, dirError);
                if (dirError)
                {
                    entry.isDirectory = false;
                }
            }

            if (!entry.isDirectory)
            {
                entry.hasExecutableInfo = true;
#if !defined(_WIN32)
                entry.isExecutable = (::access(entry.path.c_str(), X_OK) == 0);
#endif
            }

            std::error_code timeError;
            entry.lastWriteTime = std::filesystem::last_write_time(entry.path, timeError);
            entry.hasWriteTime = !timeError;

            enumeratedAny = true;

            const std::string key = normalizedKey(entry.path);
            if (key.find(searchFilter) == std::string::npos)
            {
                continue;
            }

            if (entry.isDirectory)
            {
                directories.emplace_back(std::move(entry));
            }
            else
            {
                files.emplace_back(std::move(entry));
            }
        }
    }
    else
    {
        std::filesystem::directory_iterator iterator{directory, ec};
        if (ec)
        {
            addAppDialog_.errorMessage = "Unable to open directory.";
            return;
        }

        const auto end = std::filesystem::directory_iterator{};
        for (; iterator != end; iterator.increment(ec))
        {
            if (ec)
            {
                addAppDialog_.errorMessage = "Unable to enumerate directory.";
                return;
            }

            RawEntry entry;
            entry.path = iterator->path();
            std::error_code statusError;
            std::filesystem::file_status status = iterator->status(statusError);
            if (statusError)
            {
                statusError.clear();
                status = iterator->symlink_status(statusError);
            }
            if (!statusError)
            {
                entry.isDirectory = std::filesystem::is_directory(status);
            }
            else
            {
                statusError.clear();
                std::error_code dirError;
                entry.isDirectory = std::filesystem::is_directory(entry.path, dirError);
                if (dirError)
                {
                    entry.isDirectory = false;
                }
            }

            if (!entry.isDirectory)
            {
                entry.hasExecutableInfo = true;
#if !defined(_WIN32)
                entry.isExecutable = (::access(entry.path.c_str(), X_OK) == 0);
#endif
            }

            std::error_code timeError;
            entry.lastWriteTime = std::filesystem::last_write_time(entry.path, timeError);
            entry.hasWriteTime = !timeError;

            enumeratedAny = true;

            if (entry.isDirectory)
            {
                directories.emplace_back(entry);
            }
            else
            {
                files.emplace_back(entry);
            }
        }
    }

    const auto& sortOptions = GetAddDialogSortOptions();
    int sortModeIndex = addAppDialog_.sortModeIndex;
    if (sortModeIndex < 0 || sortModeIndex >= static_cast<int>(sortOptions.size()))
    {
        sortModeIndex = 0;
    }

    const auto compareByNameAscending = [&](const RawEntry& lhs, const RawEntry& rhs) {
        const std::string lhsKey = normalizedKey(lhs.path);
        const std::string rhsKey = normalizedKey(rhs.path);
        if (lhsKey == rhsKey)
        {
            const std::string lhsLabel = globalSearch ? lhs.path.string() : lhs.path.filename().string();
            const std::string rhsLabel = globalSearch ? rhs.path.string() : rhs.path.filename().string();
            return lhsLabel < rhsLabel;
        }
        return lhsKey < rhsKey;
    };

    const auto compareByNameDescending = [&](const RawEntry& lhs, const RawEntry& rhs) {
        return compareByNameAscending(rhs, lhs);
    };

    const auto compareByModified = [&](const RawEntry& lhs, const RawEntry& rhs, bool newestFirst) {
        if (lhs.hasWriteTime && rhs.hasWriteTime && lhs.lastWriteTime != rhs.lastWriteTime)
        {
            if (newestFirst)
            {
                return lhs.lastWriteTime > rhs.lastWriteTime;
            }
            return lhs.lastWriteTime < rhs.lastWriteTime;
        }

        if (lhs.hasWriteTime != rhs.hasWriteTime)
        {
            if (newestFirst)
            {
                return lhs.hasWriteTime;
            }
            return !lhs.hasWriteTime;
        }

        return compareByNameAscending(lhs, rhs);
    };

    auto sortEntries = [&](std::vector<RawEntry>& entries) {
        std::sort(entries.begin(), entries.end(), [&](const RawEntry& lhs, const RawEntry& rhs) {
            switch (sortModeIndex)
            {
            case 1:
                return compareByNameDescending(lhs, rhs);
            case 2:
                return compareByModified(lhs, rhs, true);
            case 3:
                return compareByModified(lhs, rhs, false);
            case 0:
            default:
                return compareByNameAscending(lhs, rhs);
            }
        });
    };

    sortEntries(directories);
    sortEntries(files);

    const bool filterDuringAppend = hasFilter && !globalSearch;

    const auto& fileFilters = GetAddDialogFileTypeFilters();
    int fileFilterIndex = addAppDialog_.fileTypeFilterIndex;
    if (fileFilterIndex < 0 || fileFilterIndex >= static_cast<int>(fileFilters.size()))
    {
        fileFilterIndex = 0;
    }

    const auto& selectedFilter = fileFilters[fileFilterIndex];
    const bool fileTypeFilterActive = fileFilterIndex != 0;
    const bool allowDirectories = selectedFilter.includeDirectories;
    const bool directoriesOnly = selectedFilter.directoriesOnly;
    const bool filterHasExtensions = !selectedFilter.extensions.empty();
    const bool requireExecutablePermission = selectedFilter.requireExecutablePermission;

    auto passesFileTypeFilter = [&](const RawEntry& raw) {
        if (raw.isDirectory)
        {
            return allowDirectories;
        }

        if (directoriesOnly)
        {
            return false;
        }

        if (requireExecutablePermission)
        {
            if (!raw.hasExecutableInfo || !raw.isExecutable)
            {
                return false;
            }
        }

        if (!filterHasExtensions)
        {
            return true;
        }

        std::string extension = raw.path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });

        for (const auto& allowed : selectedFilter.extensions)
        {
            if (extension == allowed)
            {
                return true;
            }
        }
        return false;
    };

    auto appendEntries = [&](const std::vector<RawEntry>& source) {
        for (const auto& raw : source)
        {
            if (filterDuringAppend)
            {
                const std::string key = normalizedKey(raw.path);
                if (key.find(searchFilter) == std::string::npos)
                {
                    continue;
                }
            }

            if (!passesFileTypeFilter(raw))
            {
                continue;
            }

            AddAppDialogState::Entry entry;
            entry.path = raw.path;
            entry.isDirectory = raw.isDirectory;
            std::string label;
            if (globalSearch)
            {
                label = raw.path.string();
            }
            else
            {
                label = raw.path.filename().string();
                if (label.empty())
                {
                    label = raw.path.string();
                }
            }
            if (entry.isDirectory && (label.empty() || label.back() != '/'))
            {
                label.push_back('/');
            }
            SDL_Color textColor = entry.isDirectory ? theme_.heroTitle : theme_.heroBody;
            entry.label = CreateTextTexture(renderer, font, label, textColor);
            addAppDialog_.entries.emplace_back(std::move(entry));
        }
    };

    appendEntries(directories);
    appendEntries(files);

    addAppDialog_.contentHeight = static_cast<int>(addAppDialog_.entries.size()) * AddDialogRowHeight();
    addAppDialog_.entryRects.assign(addAppDialog_.entries.size(), SDL_Rect{0, 0, 0, 0});

    if (addAppDialog_.entries.empty())
    {
        const bool searchActive = hasFilter && (globalSearch || filterDuringAppend);
        const bool anyFilterActive = searchActive || fileTypeFilterActive;
        if (globalSearch)
        {
            addAppDialog_.errorMessage = anyFilterActive ? "No items match your filters." : "Directory is empty.";
        }
        else
        {
            addAppDialog_.errorMessage = anyFilterActive && enumeratedAny ? "No items match your filters." : "Directory is empty.";
        }
    }

    if (!previouslySelectedPath.empty())
    {
        for (std::size_t index = 0; index < addAppDialog_.entries.size(); ++index)
        {
            if (addAppDialog_.entries[index].path == previouslySelectedPath)
            {
                addAppDialog_.selectedIndex = static_cast<int>(index);
                break;
            }
        }
    }

    const int maxScroll = std::max(0, addAppDialog_.contentHeight - addAppDialog_.listViewport.h);
    addAppDialog_.scrollOffset = std::clamp(previousScroll, 0, maxScroll);
}

void Application::RenderAddAppDialog(double timeSeconds)
{
    if (!addAppDialog_.visible)
    {
        return;
    }

    SDL_Renderer* renderer = rendererHost_.Renderer();
    if (!renderer)
    {
        return;
    }

    SDL_BlendMode previousBlendMode = SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(renderer, &previousBlendMode);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    const platform::RendererDimensions outputDimensions = rendererHost_.OutputSize();
    int outputWidth = outputDimensions.width;
    int outputHeight = outputDimensions.height;

    SDL_Rect overlayRect{0, 0, outputWidth, outputHeight};
    SDL_SetRenderDrawColor(renderer, 6, 10, 26, 190);
    SDL_RenderFillRect(renderer, &overlayRect);

    const int panelPadding = ui::Scale(24);
    const int panelWidth = std::clamp(outputWidth - ui::Scale(240), ui::Scale(520), outputWidth - ui::Scale(80));
    const int maxPanelHeight = std::max(ui::Scale(440), outputHeight - ui::Scale(60));
    int minPanelHeight = AddDialogRowHeight() * 12 + ui::Scale(260);
    if (minPanelHeight > maxPanelHeight)
    {
        minPanelHeight = maxPanelHeight;
    }
    int desiredPanelHeight = outputHeight - ui::Scale(100);
    desiredPanelHeight = std::clamp(desiredPanelHeight, minPanelHeight, maxPanelHeight);
    const int panelHeight = desiredPanelHeight;
    SDL_Rect panelRect{
        overlayRect.x + (overlayRect.w - panelWidth) / 2,
        overlayRect.y + (overlayRect.h - panelHeight) / 2,
        panelWidth,
        panelHeight};
    addAppDialog_.panelRect = panelRect;

    SDL_Color panelFill = color::Mix(theme_.libraryCardActive, theme_.background, 0.35f);
    SDL_SetRenderDrawColor(renderer, panelFill.r, panelFill.g, panelFill.b, panelFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, panelRect, kAddDialogCornerRadius);
    SDL_SetRenderDrawColor(renderer, theme_.border.r, theme_.border.g, theme_.border.b, theme_.border.a);
    colony::drawing::RenderRoundedRect(renderer, panelRect, kAddDialogCornerRadius);

    int cursorX = panelRect.x + panelPadding;
    int cursorY = panelRect.y + panelPadding;

    const std::string titleText = "Add Application";
    TextTexture titleTexture = CreateTextTexture(renderer, fonts_.heroTitle.get(), titleText, theme_.heroTitle);
    if (titleTexture.texture)
    {
        SDL_Rect titleRect{cursorX, cursorY, titleTexture.width, titleTexture.height};
        RenderTexture(renderer, titleTexture, titleRect);
        cursorY += titleRect.h + ui::Scale(10);
    }

    const int parentButtonHeight = ui::Scale(34);
    const int parentButtonWidth = ui::Scale(150);
    addAppDialog_.parentButtonRect = SDL_Rect{
        panelRect.x + panelRect.w - panelPadding - parentButtonWidth,
        cursorY,
        parentButtonWidth,
        parentButtonHeight};

    std::string directoryString = addAppDialog_.currentDirectory.string();
    TextTexture directoryTexture = CreateTextTexture(renderer, fonts_.tileSubtitle.get(), directoryString, theme_.muted);

    const int pathAvailableWidth = addAppDialog_.parentButtonRect.x - ui::Scale(12) - cursorX;
    if (directoryTexture.texture && pathAvailableWidth > 0)
    {
        SDL_Rect pathRect{cursorX, cursorY + (parentButtonHeight - directoryTexture.height) / 2, directoryTexture.width, directoryTexture.height};
        SDL_Rect clipRect{pathRect.x, pathRect.y, std::min(pathRect.w, pathAvailableWidth), pathRect.h};
        SDL_RenderSetClipRect(renderer, &clipRect);
        RenderTexture(renderer, directoryTexture, pathRect);
        SDL_RenderSetClipRect(renderer, nullptr);
    }

    SDL_Color parentFill = addAppDialog_.parentAvailable ? color::Mix(theme_.libraryCardActive, theme_.background, 0.6f)
                                                         : color::Mix(theme_.libraryCard, theme_.background, 0.6f);
    SDL_SetRenderDrawColor(renderer, parentFill.r, parentFill.g, parentFill.b, parentFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, addAppDialog_.parentButtonRect, 12);
    SDL_SetRenderDrawColor(renderer, theme_.border.r, theme_.border.g, theme_.border.b, theme_.border.a);
    colony::drawing::RenderRoundedRect(renderer, addAppDialog_.parentButtonRect, 12);

    TextTexture parentLabel = CreateTextTexture(
        renderer,
        fonts_.tileSubtitle.get(),
        addAppDialog_.parentAvailable ? "Up one level" : "Top level",
        addAppDialog_.parentAvailable ? theme_.heroTitle : theme_.muted);
    if (parentLabel.texture)
    {
        SDL_Rect parentLabelRect{
            addAppDialog_.parentButtonRect.x + (addAppDialog_.parentButtonRect.w - parentLabel.width) / 2,
            addAppDialog_.parentButtonRect.y + (addAppDialog_.parentButtonRect.h - parentLabel.height) / 2,
            parentLabel.width,
            parentLabel.height};
        RenderTexture(renderer, parentLabel, parentLabelRect);
    }

    cursorY += parentButtonHeight + ui::Scale(12);

    const int searchHeight = ui::Scale(36);
    addAppDialog_.searchBoxRect = SDL_Rect{
        panelRect.x + panelPadding,
        cursorY,
        panelRect.w - 2 * panelPadding,
        searchHeight};

    SDL_Color searchFill = color::Mix(theme_.libraryCard, theme_.background, 0.55f);
    SDL_SetRenderDrawColor(renderer, searchFill.r, searchFill.g, searchFill.b, searchFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, addAppDialog_.searchBoxRect, 12);
    SDL_SetRenderDrawColor(renderer, theme_.border.r, theme_.border.g, theme_.border.b, theme_.border.a);
    colony::drawing::RenderRoundedRect(renderer, addAppDialog_.searchBoxRect, 12);

    const int searchIconSize = std::max(ui::Scale(16), searchHeight - ui::Scale(16));
    SDL_Rect searchIconRect{
        addAppDialog_.searchBoxRect.x + ui::Scale(10),
        addAppDialog_.searchBoxRect.y + (addAppDialog_.searchBoxRect.h - searchIconSize) / 2,
        searchIconSize,
        searchIconSize};
    SDL_Color searchIconColor = color::Mix(theme_.muted, theme_.heroTitle, 0.25f);
    SDL_SetRenderDrawColor(renderer, searchIconColor.r, searchIconColor.g, searchIconColor.b, searchIconColor.a);
    colony::drawing::RenderRoundedRect(renderer, searchIconRect, searchIconSize / 2);
    SDL_RenderDrawLine(
        renderer,
        searchIconRect.x + searchIconRect.w - ui::Scale(2),
        searchIconRect.y + searchIconRect.h - ui::Scale(2),
        searchIconRect.x + searchIconRect.w + ui::Scale(6),
        searchIconRect.y + searchIconRect.h + ui::Scale(6));

    const int searchTextX = searchIconRect.x + searchIconRect.w + ui::Scale(8);
    const int searchTextMaxWidth = addAppDialog_.searchBoxRect.x + addAppDialog_.searchBoxRect.w - ui::Scale(14) - searchTextX;
    SDL_Rect searchTextClip{searchTextX, addAppDialog_.searchBoxRect.y, std::max(searchTextMaxWidth, 0), addAppDialog_.searchBoxRect.h};
    SDL_RenderSetClipRect(renderer, &searchTextClip);

    const bool hasSearchText = !addAppDialog_.searchQuery.empty();
    TextTexture searchTextTexture = hasSearchText
        ? CreateTextTexture(renderer, fonts_.tileSubtitle.get(), addAppDialog_.searchQuery, theme_.heroTitle)
        : CreateTextTexture(renderer, fonts_.tileSubtitle.get(), "Search files", theme_.muted);

    if (searchTextTexture.texture)
    {
        SDL_Rect searchTextRect{
            searchTextX,
            addAppDialog_.searchBoxRect.y + (addAppDialog_.searchBoxRect.h - searchTextTexture.height) / 2,
            searchTextTexture.width,
            searchTextTexture.height};
        RenderTexture(renderer, searchTextTexture, searchTextRect);
    }

    SDL_RenderSetClipRect(renderer, nullptr);

    if (addAppDialog_.searchFocused)
    {
        const bool caretVisible = std::fmod(timeSeconds, 1.0) < 0.5;
        if (caretVisible)
        {
            const int caretOffset = hasSearchText ? searchTextTexture.width : 0;
            const int caretX = searchTextX + caretOffset + ui::Scale(2);
            SDL_Rect caretClip{
                searchTextX,
                addAppDialog_.searchBoxRect.y + ui::Scale(6),
                searchTextMaxWidth,
                addAppDialog_.searchBoxRect.h - ui::Scale(12)};
            SDL_RenderSetClipRect(renderer, &caretClip);
            SDL_SetRenderDrawColor(renderer, theme_.heroTitle.r, theme_.heroTitle.g, theme_.heroTitle.b, theme_.heroTitle.a);
            SDL_RenderDrawLine(
                renderer,
                caretX,
                addAppDialog_.searchBoxRect.y + ui::Scale(6),
                caretX,
                addAppDialog_.searchBoxRect.y + addAppDialog_.searchBoxRect.h - ui::Scale(6));
            SDL_RenderSetClipRect(renderer, nullptr);
        }
    }

    cursorY += searchHeight + ui::Scale(16);

    const auto& sortOptions = GetAddDialogSortOptions();
    int sortIndex = addAppDialog_.sortModeIndex;
    if (sortIndex < 0 || sortIndex >= static_cast<int>(sortOptions.size()))
    {
        sortIndex = 0;
    }

    const auto& fileFilters = GetAddDialogFileTypeFilters();
    int fileFilterIndex = addAppDialog_.fileTypeFilterIndex;
    if (fileFilterIndex < 0 || fileFilterIndex >= static_cast<int>(fileFilters.size()))
    {
        fileFilterIndex = 0;
    }
    const auto& selectedFileFilter = fileFilters[fileFilterIndex];
    const bool fileFilterActive = fileFilterIndex != 0;
    const bool sortActive = sortIndex != 0;

    const int optionHeight = ui::Scale(34);
    const int optionSpacing = ui::Scale(12);
    const int optionWidth = (panelRect.w - 2 * panelPadding - optionSpacing) / 2;
    if (optionWidth > 0)
    {
        addAppDialog_.sortButtonRect = SDL_Rect{panelRect.x + panelPadding, cursorY, optionWidth, optionHeight};
        addAppDialog_.filterButtonRect = SDL_Rect{
            addAppDialog_.sortButtonRect.x + optionWidth + optionSpacing,
            cursorY,
            optionWidth,
            optionHeight};

        auto renderOptionButton = [&](const SDL_Rect& rect, const std::string& label, bool active) {
            SDL_Color fill = active ? color::Mix(theme_.libraryCardActive, theme_.background, 0.55f)
                                    : color::Mix(theme_.libraryCard, theme_.background, 0.55f);
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            colony::drawing::RenderFilledRoundedRect(renderer, rect, 12);
            SDL_SetRenderDrawColor(renderer, theme_.border.r, theme_.border.g, theme_.border.b, theme_.border.a);
            colony::drawing::RenderRoundedRect(renderer, rect, 12);

            TextTexture text = CreateTextTexture(renderer, fonts_.tileSubtitle.get(), label, theme_.heroTitle);
            if (text.texture)
            {
                SDL_Rect clipRect = rect;
                SDL_RenderSetClipRect(renderer, &clipRect);
                SDL_Rect textRect{
                    rect.x + (rect.w - text.width) / 2,
                    rect.y + (rect.h - text.height) / 2,
                    text.width,
                    text.height};
                RenderTexture(renderer, text, textRect);
                SDL_RenderSetClipRect(renderer, nullptr);
            }
        };

        std::string sortLabel = "Sort: ";
        sortLabel.append(sortOptions[sortIndex].label);
        renderOptionButton(addAppDialog_.sortButtonRect, sortLabel, sortActive);

        std::string filterLabel = "Type: ";
        filterLabel.append(selectedFileFilter.label);
        const bool filterButtonActive = fileFilterActive || addAppDialog_.filterDropdownOpen;
        renderOptionButton(addAppDialog_.filterButtonRect, filterLabel, filterButtonActive);

        const int dropdownOptionHeight = optionHeight;
        bool dropdownVisible = false;
        int dropdownOptionCount = 0;
        if (addAppDialog_.filterDropdownOpen)
        {
            const int dropdownSpacing = ui::Scale(6);
            const int optionCount = static_cast<int>(fileFilters.size());
            dropdownOptionCount = optionCount;
            if (optionCount > 0)
            {
                int dropdownHeight = optionCount * dropdownOptionHeight;
                SDL_Rect dropdownRect{
                    addAppDialog_.filterButtonRect.x,
                    addAppDialog_.filterButtonRect.y + addAppDialog_.filterButtonRect.h + dropdownSpacing,
                    addAppDialog_.filterButtonRect.w,
                    dropdownHeight};

                const int panelBottom = panelRect.y + panelRect.h - panelPadding;
                if (dropdownRect.y + dropdownRect.h > panelBottom)
                {
                    dropdownRect.y = addAppDialog_.filterButtonRect.y - dropdownSpacing - dropdownRect.h;
                    if (dropdownRect.y < panelRect.y + panelPadding)
                    {
                        dropdownRect.y = panelRect.y + panelPadding;
                    }
                }

                addAppDialog_.filterDropdownRect = dropdownRect;
                addAppDialog_.filterDropdownOptionRects.assign(optionCount, SDL_Rect{0, 0, 0, 0});
                int rowTop = dropdownRect.y;
                for (int index = 0; index < optionCount; ++index)
                {
                    addAppDialog_.filterDropdownOptionRects[index]
                        = SDL_Rect{dropdownRect.x, rowTop, dropdownRect.w, dropdownOptionHeight};
                    rowTop += dropdownOptionHeight;
                }
                dropdownVisible = true;
            }
            else
            {
                addAppDialog_.filterDropdownRect = SDL_Rect{0, 0, 0, 0};
                addAppDialog_.filterDropdownOptionRects.clear();
            }
        }
        else
        {
            addAppDialog_.filterDropdownRect = SDL_Rect{0, 0, 0, 0};
            addAppDialog_.filterDropdownOptionRects.clear();
        }

        addAppDialog_.filterDropdownVisible = dropdownVisible;
        addAppDialog_.filterDropdownOptionHeight = dropdownOptionHeight;
        addAppDialog_.filterDropdownOptionCount = dropdownOptionCount;

        cursorY += optionHeight + ui::Scale(16);
    }
    else
    {
        addAppDialog_.sortButtonRect = SDL_Rect{0, 0, 0, 0};
        addAppDialog_.filterButtonRect = SDL_Rect{0, 0, 0, 0};
        addAppDialog_.filterDropdownRect = SDL_Rect{0, 0, 0, 0};
        addAppDialog_.filterDropdownOptionRects.clear();
        addAppDialog_.filterDropdownVisible = false;
        addAppDialog_.filterDropdownOptionHeight = 0;
        addAppDialog_.filterDropdownOptionCount = 0;
    }

    const int footerHeight = ui::Scale(86);
    int availableHeight = panelRect.h - cursorY - footerHeight - panelPadding;
    int maxViewportHeight = panelRect.h - cursorY - ui::Scale(24);
    maxViewportHeight = std::max(maxViewportHeight, AddDialogRowHeight() * 6);
    const int minVisibleHeight = AddDialogRowHeight() * 12;
    if (availableHeight < minVisibleHeight)
    {
        availableHeight = std::min(minVisibleHeight, maxViewportHeight);
    }
    availableHeight = std::max(availableHeight, AddDialogRowHeight() * 6);

    SDL_Rect listViewport{
        panelRect.x + panelPadding,
        cursorY,
        panelRect.w - 2 * panelPadding,
        availableHeight};
    addAppDialog_.listViewport = listViewport;

    const int maxScroll = std::max(0, addAppDialog_.contentHeight - listViewport.h);
    addAppDialog_.scrollOffset = std::clamp(addAppDialog_.scrollOffset, 0, maxScroll);

    SDL_Rect contentClip = listViewport;
    SDL_RenderSetClipRect(renderer, &contentClip);

    const int rowRadius = ui::Scale(10);
    int rowTop = listViewport.y - addAppDialog_.scrollOffset;
    addAppDialog_.entryRects.assign(addAppDialog_.entries.size(), SDL_Rect{0, 0, 0, 0});
    for (std::size_t index = 0; index < addAppDialog_.entries.size(); ++index)
    {
        SDL_Rect rowRect{listViewport.x, rowTop, listViewport.w, AddDialogRowHeight()};
        addAppDialog_.entryRects[index] = rowRect;
        rowTop += AddDialogRowHeight();

        if (rowRect.y + rowRect.h <= listViewport.y || rowRect.y >= listViewport.y + listViewport.h)
        {
            continue;
        }

        SDL_Rect clippedRow{
            rowRect.x,
            std::max(rowRect.y, listViewport.y),
            rowRect.w,
            std::min(rowRect.y + rowRect.h, listViewport.y + listViewport.h) - std::max(rowRect.y, listViewport.y)};

        const bool isSelected = static_cast<int>(index) == addAppDialog_.selectedIndex;
        SDL_Color rowColor = isSelected ? color::Mix(theme_.libraryCardActive, theme_.channelBadge, 0.35f)
                                        : color::Mix(theme_.libraryBackground, theme_.libraryCard, 0.45f);
        SDL_SetRenderDrawColor(renderer, rowColor.r, rowColor.g, rowColor.b, rowColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, clippedRow, rowRadius);

        SDL_Color borderColor = isSelected ? theme_.channelBadge : theme_.border;
        SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
        colony::drawing::RenderRoundedRect(renderer, clippedRow, rowRadius);

        const int glyphSize = ui::Scale(16);
        SDL_Rect glyphRect{
            rowRect.x + ui::Scale(14),
            rowRect.y + (rowRect.h - glyphSize) / 2,
            glyphSize,
            glyphSize};
        if (glyphRect.y + glyphRect.h > listViewport.y + listViewport.h)
        {
            glyphRect.y = std::min(glyphRect.y, listViewport.y + listViewport.h - glyphRect.h);
        }
        if (glyphRect.y < listViewport.y)
        {
            glyphRect.y = listViewport.y;
        }

        const auto& entry = addAppDialog_.entries[index];
        SDL_Color glyphColor = entry.isDirectory ? theme_.channelBadge : theme_.muted;
        SDL_SetRenderDrawColor(renderer, glyphColor.r, glyphColor.g, glyphColor.b, glyphColor.a);
        colony::drawing::RenderFilledRoundedRect(renderer, glyphRect, ui::Scale(4));

        int textX = glyphRect.x + glyphRect.w + ui::Scale(12);
        if (entry.label.texture)
        {
            SDL_Rect textRect{textX, rowRect.y + (rowRect.h - entry.label.height) / 2, entry.label.width, entry.label.height};
            SDL_Rect clipRect{
                listViewport.x + ui::Scale(12),
                listViewport.y,
                listViewport.w - ui::Scale(24),
                listViewport.h};
            SDL_RenderSetClipRect(renderer, &clipRect);
            RenderTexture(renderer, entry.label, textRect);
            SDL_RenderSetClipRect(renderer, &contentClip);
        }
    }

    SDL_RenderSetClipRect(renderer, nullptr);

    cursorY = listViewport.y + listViewport.h + ui::Scale(12);

    const bool canConfirm = addAppDialog_.selectedIndex >= 0
        && addAppDialog_.selectedIndex < static_cast<int>(addAppDialog_.entries.size())
        && !addAppDialog_.entries[addAppDialog_.selectedIndex].isDirectory;

    TextTexture errorTexture;
    if (!addAppDialog_.errorMessage.empty())
    {
        errorTexture = CreateTextTexture(renderer, fonts_.tileSubtitle.get(), addAppDialog_.errorMessage, theme_.channelBadge);
        if (errorTexture.texture)
        {
            SDL_Rect errorRect{cursorX, cursorY, errorTexture.width, errorTexture.height};
            RenderTexture(renderer, errorTexture, errorRect);
            cursorY += errorRect.h + ui::Scale(8);
        }
    }

    const int buttonSpacing = ui::Scale(14);
    const int buttonWidth = ui::Scale(150);
    const int buttonHeight = ui::Scale(46);

    addAppDialog_.confirmButtonRect = SDL_Rect{
        panelRect.x + panelRect.w - panelPadding - buttonWidth,
        panelRect.y + panelRect.h - panelPadding - buttonHeight,
        buttonWidth,
        buttonHeight};
    addAppDialog_.cancelButtonRect = SDL_Rect{
        addAppDialog_.confirmButtonRect.x - buttonSpacing - buttonWidth,
        addAppDialog_.confirmButtonRect.y,
        buttonWidth,
        buttonHeight};

    SDL_Color confirmFill = canConfirm ? color::Mix(theme_.channelBadge, theme_.libraryCardActive, 0.3f)
                                       : color::Mix(theme_.libraryCard, theme_.libraryBackground, 0.5f);
    SDL_SetRenderDrawColor(renderer, confirmFill.r, confirmFill.g, confirmFill.b, confirmFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, addAppDialog_.confirmButtonRect, 14);
    SDL_SetRenderDrawColor(renderer, theme_.border.r, theme_.border.g, theme_.border.b, theme_.border.a);
    colony::drawing::RenderRoundedRect(renderer, addAppDialog_.confirmButtonRect, 14);

    SDL_SetRenderDrawColor(renderer, theme_.libraryCardActive.r, theme_.libraryCardActive.g, theme_.libraryCardActive.b, theme_.libraryCardActive.a);
    colony::drawing::RenderFilledRoundedRect(renderer, addAppDialog_.cancelButtonRect, 14);
    SDL_SetRenderDrawColor(renderer, theme_.border.r, theme_.border.g, theme_.border.b, theme_.border.a);
    colony::drawing::RenderRoundedRect(renderer, addAppDialog_.cancelButtonRect, 14);

    TextTexture confirmLabel = CreateTextTexture(
        renderer,
        fonts_.button.get(),
        "Add to library",
        canConfirm ? theme_.heroTitle : theme_.muted);
    if (confirmLabel.texture)
    {
        SDL_Rect confirmLabelRect{
            addAppDialog_.confirmButtonRect.x + (addAppDialog_.confirmButtonRect.w - confirmLabel.width) / 2,
            addAppDialog_.confirmButtonRect.y + (addAppDialog_.confirmButtonRect.h - confirmLabel.height) / 2,
            confirmLabel.width,
            confirmLabel.height};
        RenderTexture(renderer, confirmLabel, confirmLabelRect);
    }

    TextTexture cancelLabel = CreateTextTexture(renderer, fonts_.button.get(), "Cancel", theme_.heroTitle);
    if (cancelLabel.texture)
    {
        SDL_Rect cancelLabelRect{
            addAppDialog_.cancelButtonRect.x + (addAppDialog_.cancelButtonRect.w - cancelLabel.width) / 2,
            addAppDialog_.cancelButtonRect.y + (addAppDialog_.cancelButtonRect.h - cancelLabel.height) / 2,
            cancelLabel.width,
            cancelLabel.height};
        RenderTexture(renderer, cancelLabel, cancelLabelRect);
    }

    if (addAppDialog_.filterDropdownVisible && addAppDialog_.filterDropdownRect.w > 0
        && addAppDialog_.filterDropdownRect.h > 0)
    {
        const int optionCount = std::min(
            addAppDialog_.filterDropdownOptionCount,
            static_cast<int>(fileFilters.size()));
        if (optionCount > 0)
        {
            SDL_Rect dropdownRect = addAppDialog_.filterDropdownRect;
            SDL_Color dropdownFill = color::Mix(theme_.libraryBackground, theme_.libraryCard, 0.55f);
            SDL_SetRenderDrawColor(renderer, dropdownFill.r, dropdownFill.g, dropdownFill.b, dropdownFill.a);
            colony::drawing::RenderFilledRoundedRect(renderer, dropdownRect, 12);
            SDL_SetRenderDrawColor(renderer, theme_.border.r, theme_.border.g, theme_.border.b, theme_.border.a);
            colony::drawing::RenderRoundedRect(renderer, dropdownRect, 12);

            SDL_RenderSetClipRect(renderer, &dropdownRect);
            int mouseX = 0;
            int mouseY = 0;
            SDL_GetMouseState(&mouseX, &mouseY);

            for (int index = 0; index < optionCount; ++index)
            {
                SDL_Rect rowRect = addAppDialog_.filterDropdownOptionRects.size() > static_cast<std::size_t>(index)
                    ? addAppDialog_.filterDropdownOptionRects[index]
                    : SDL_Rect{dropdownRect.x, dropdownRect.y + index * addAppDialog_.filterDropdownOptionHeight, dropdownRect.w, addAppDialog_.filterDropdownOptionHeight};
                const int optionHeight = addAppDialog_.filterDropdownOptionHeight;
                if (rowRect.h <= 0)
                {
                    rowRect.h = optionHeight;
                }

                bool isSelected = index == fileFilterIndex;
                bool isHovered = PointInRect(rowRect, mouseX, mouseY);
                SDL_Color rowColor = isSelected ? color::Mix(theme_.channelBadge, theme_.libraryCardActive, 0.35f)
                                                : color::Mix(theme_.libraryBackground, theme_.libraryCard, isHovered ? 0.65f
                                                                                                                    : 0.45f);
                SDL_SetRenderDrawColor(renderer, rowColor.r, rowColor.g, rowColor.b, rowColor.a);
                SDL_Rect insetRect{
                    rowRect.x + ui::Scale(4),
                    rowRect.y + ui::Scale(2),
                    rowRect.w - ui::Scale(8),
                    rowRect.h - ui::Scale(4)};
                const int radius = (index == 0 || index == optionCount - 1) ? 10 : 6;
                colony::drawing::RenderFilledRoundedRect(renderer, insetRect, radius);

                if (index < static_cast<int>(fileFilters.size()))
                {
                    TextTexture optionLabel = CreateTextTexture(renderer, fonts_.tileSubtitle.get(), fileFilters[index].label, theme_.heroTitle);
                    if (optionLabel.texture)
                    {
                        SDL_Rect textRect{
                            insetRect.x + ui::Scale(8),
                            insetRect.y + (insetRect.h - optionLabel.height) / 2,
                            optionLabel.width,
                            optionLabel.height};
                        RenderTexture(renderer, optionLabel, textRect);
                    }
                }
            }
            SDL_RenderSetClipRect(renderer, nullptr);
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, previousBlendMode);
}

void Application::RenderEditUserAppDialog(double timeSeconds)
{
    if (!editAppDialog_.visible)
    {
        return;
    }

    SDL_Renderer* renderer = rendererHost_.Renderer();
    if (!renderer)
    {
        return;
    }

    SDL_BlendMode previousBlendMode = SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(renderer, &previousBlendMode);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    const platform::RendererDimensions outputDimensions = rendererHost_.OutputSize();
    int outputWidth = outputDimensions.width;
    int outputHeight = outputDimensions.height;

    SDL_Rect overlayRect{0, 0, outputWidth, outputHeight};
    SDL_SetRenderDrawColor(renderer, 6, 10, 26, 210);
    SDL_RenderFillRect(renderer, &overlayRect);

    const int panelPadding = ui::Scale(24);
    int panelWidth = std::min(outputWidth - ui::Scale(320), ui::Scale(640));
    panelWidth = std::max(panelWidth, ui::Scale(460));
    int panelHeight = std::min(outputHeight - ui::Scale(200), ui::Scale(460));
    panelHeight = std::max(panelHeight, ui::Scale(360));

    SDL_Rect panelRect{
        overlayRect.x + (overlayRect.w - panelWidth) / 2,
        overlayRect.y + (overlayRect.h - panelHeight) / 2,
        panelWidth,
        panelHeight};
    editAppDialog_.panelRect = panelRect;

    SDL_Color panelFill = color::Mix(theme_.libraryCardActive, theme_.background, 0.4f);
    SDL_SetRenderDrawColor(renderer, panelFill.r, panelFill.g, panelFill.b, panelFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, panelRect, kAddDialogCornerRadius);
    SDL_SetRenderDrawColor(renderer, theme_.border.r, theme_.border.g, theme_.border.b, theme_.border.a);
    colony::drawing::RenderRoundedRect(renderer, panelRect, kAddDialogCornerRadius);

    int cursorX = panelRect.x + panelPadding;
    int cursorY = panelRect.y + panelPadding;

    TextTexture titleTexture = CreateTextTexture(renderer, fonts_.heroTitle.get(), "Customize Application", theme_.heroTitle);
    if (titleTexture.texture)
    {
        SDL_Rect titleRect{cursorX, cursorY, titleTexture.width, titleTexture.height};
        RenderTexture(renderer, titleTexture, titleRect);
        cursorY += titleRect.h + ui::Scale(8);
    }

    TextTexture subtitleTexture = CreateTextTexture(
        renderer,
        fonts_.tileSubtitle.get(),
        "Rename your shortcut and set an accent color.",
        theme_.muted);
    if (subtitleTexture.texture)
    {
        SDL_Rect subtitleRect{cursorX, cursorY, subtitleTexture.width, subtitleTexture.height};
        RenderTexture(renderer, subtitleTexture, subtitleRect);
        cursorY += subtitleRect.h + ui::Scale(16);
    }

    TextTexture nameLabel = CreateTextTexture(renderer, fonts_.tileSubtitle.get(), "Display name", theme_.muted);
    if (nameLabel.texture)
    {
        SDL_Rect labelRect{cursorX, cursorY, nameLabel.width, nameLabel.height};
        RenderTexture(renderer, nameLabel, labelRect);
        cursorY += labelRect.h + ui::Scale(6);
    }

    const int fieldHeight = ui::Scale(44);
    editAppDialog_.nameFieldRect = SDL_Rect{cursorX, cursorY, panelRect.w - 2 * panelPadding, fieldHeight};
    SDL_Color nameFill = editAppDialog_.nameFocused ? color::Mix(theme_.libraryCardActive, theme_.background, 0.6f)
                                                   : color::Mix(theme_.libraryCard, theme_.background, 0.55f);
    SDL_SetRenderDrawColor(renderer, nameFill.r, nameFill.g, nameFill.b, nameFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, editAppDialog_.nameFieldRect, 12);
    SDL_Color nameBorder = editAppDialog_.nameFocused ? theme_.channelBadge : theme_.border;
    SDL_SetRenderDrawColor(renderer, nameBorder.r, nameBorder.g, nameBorder.b, nameBorder.a);
    colony::drawing::RenderRoundedRect(renderer, editAppDialog_.nameFieldRect, 12);

    SDL_Rect nameTextClip{
        editAppDialog_.nameFieldRect.x + ui::Scale(12),
        editAppDialog_.nameFieldRect.y,
        editAppDialog_.nameFieldRect.w - ui::Scale(24),
        editAppDialog_.nameFieldRect.h};
    SDL_RenderSetClipRect(renderer, &nameTextClip);

    const bool hasName = !editAppDialog_.nameInput.empty();
    TextTexture nameValueTexture = hasName
        ? CreateTextTexture(renderer, fonts_.tileSubtitle.get(), editAppDialog_.nameInput, theme_.heroTitle)
        : CreateTextTexture(renderer, fonts_.tileSubtitle.get(), "Enter a name", theme_.muted);
    if (nameValueTexture.texture)
    {
        SDL_Rect valueRect{
            nameTextClip.x,
            editAppDialog_.nameFieldRect.y + (editAppDialog_.nameFieldRect.h - nameValueTexture.height) / 2,
            nameValueTexture.width,
            nameValueTexture.height};
        RenderTexture(renderer, nameValueTexture, valueRect);
    }

    SDL_RenderSetClipRect(renderer, nullptr);

    if (editAppDialog_.nameFocused)
    {
        const bool caretVisible = std::fmod(timeSeconds, 1.0) < 0.5;
        if (caretVisible)
        {
            const int caretOffset = hasName ? nameValueTexture.width : 0;
            const int caretX = nameTextClip.x + caretOffset + ui::Scale(2);
            SDL_Rect caretClip{nameTextClip.x, nameTextClip.y + ui::Scale(6), nameTextClip.w, nameTextClip.h - ui::Scale(12)};
            SDL_RenderSetClipRect(renderer, &caretClip);
            SDL_SetRenderDrawColor(renderer, theme_.heroTitle.r, theme_.heroTitle.g, theme_.heroTitle.b, theme_.heroTitle.a);
            SDL_RenderDrawLine(
                renderer,
                caretX,
                editAppDialog_.nameFieldRect.y + ui::Scale(6),
                caretX,
                editAppDialog_.nameFieldRect.y + editAppDialog_.nameFieldRect.h - ui::Scale(6));
            SDL_RenderSetClipRect(renderer, nullptr);
        }
    }

    cursorY += fieldHeight + ui::Scale(18);

    TextTexture colorLabel = CreateTextTexture(renderer, fonts_.tileSubtitle.get(), "Accent color", theme_.muted);
    if (colorLabel.texture)
    {
        SDL_Rect colorLabelRect{cursorX, cursorY, colorLabel.width, colorLabel.height};
        RenderTexture(renderer, colorLabel, colorLabelRect);
        cursorY += colorLabelRect.h + ui::Scale(6);
    }

    editAppDialog_.colorFieldRect = SDL_Rect{cursorX, cursorY, panelRect.w - 2 * panelPadding, fieldHeight};
    SDL_Color colorFill = editAppDialog_.colorFocused ? color::Mix(theme_.libraryCardActive, theme_.background, 0.6f)
                                                     : color::Mix(theme_.libraryCard, theme_.background, 0.55f);
    SDL_SetRenderDrawColor(renderer, colorFill.r, colorFill.g, colorFill.b, colorFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, editAppDialog_.colorFieldRect, 12);
    SDL_Color colorBorder = editAppDialog_.colorFocused ? theme_.channelBadge : theme_.border;
    SDL_SetRenderDrawColor(renderer, colorBorder.r, colorBorder.g, colorBorder.b, colorBorder.a);
    colony::drawing::RenderRoundedRect(renderer, editAppDialog_.colorFieldRect, 12);

    const int previewSize = ui::Scale(28);
    SDL_Rect previewRect{
        editAppDialog_.colorFieldRect.x + editAppDialog_.colorFieldRect.w - previewSize - ui::Scale(10),
        editAppDialog_.colorFieldRect.y + (editAppDialog_.colorFieldRect.h - previewSize) / 2,
        previewSize,
        previewSize};

    SDL_Color previewColor = color::ParseHexColor(editAppDialog_.colorInput, theme_.channelBadge);
    SDL_SetRenderDrawColor(renderer, previewColor.r, previewColor.g, previewColor.b, previewColor.a);
    colony::drawing::RenderFilledRoundedRect(renderer, previewRect, 8);
    SDL_SetRenderDrawColor(renderer, theme_.border.r, theme_.border.g, theme_.border.b, theme_.border.a);
    colony::drawing::RenderRoundedRect(renderer, previewRect, 8);

    SDL_Rect colorTextClip{
        editAppDialog_.colorFieldRect.x + ui::Scale(12),
        editAppDialog_.colorFieldRect.y,
        editAppDialog_.colorFieldRect.w - previewSize - ui::Scale(34),
        editAppDialog_.colorFieldRect.h};
    SDL_RenderSetClipRect(renderer, &colorTextClip);

    const bool hasColor = !editAppDialog_.colorInput.empty();
    TextTexture colorValueTexture = hasColor
        ? CreateTextTexture(renderer, fonts_.tileSubtitle.get(), editAppDialog_.colorInput, theme_.heroTitle)
        : CreateTextTexture(renderer, fonts_.tileSubtitle.get(), "#RRGGBB", theme_.muted);
    if (colorValueTexture.texture)
    {
        SDL_Rect colorValueRect{
            colorTextClip.x,
            editAppDialog_.colorFieldRect.y + (editAppDialog_.colorFieldRect.h - colorValueTexture.height) / 2,
            colorValueTexture.width,
            colorValueTexture.height};
        RenderTexture(renderer, colorValueTexture, colorValueRect);
    }

    SDL_RenderSetClipRect(renderer, nullptr);

    if (editAppDialog_.colorFocused)
    {
        const bool caretVisible = std::fmod(timeSeconds, 1.0) < 0.5;
        if (caretVisible)
        {
            const int caretOffset = hasColor ? colorValueTexture.width : 0;
            const int caretX = colorTextClip.x + caretOffset + ui::Scale(2);
            SDL_Rect caretClip{colorTextClip.x, colorTextClip.y + ui::Scale(6), colorTextClip.w, colorTextClip.h - ui::Scale(12)};
            SDL_RenderSetClipRect(renderer, &caretClip);
            SDL_SetRenderDrawColor(renderer, theme_.heroTitle.r, theme_.heroTitle.g, theme_.heroTitle.b, theme_.heroTitle.a);
            SDL_RenderDrawLine(
                renderer,
                caretX,
                editAppDialog_.colorFieldRect.y + ui::Scale(6),
                caretX,
                editAppDialog_.colorFieldRect.y + editAppDialog_.colorFieldRect.h - ui::Scale(6));
            SDL_RenderSetClipRect(renderer, nullptr);
        }
    }

    cursorY += fieldHeight + ui::Scale(12);

    TextTexture hintTexture = CreateTextTexture(
        renderer,
        fonts_.tileSubtitle.get(),
        "Accepts #RGB or #RRGGBB values.",
        theme_.muted);
    if (hintTexture.texture)
    {
        SDL_Rect hintRect{cursorX, cursorY, hintTexture.width, hintTexture.height};
        RenderTexture(renderer, hintTexture, hintRect);
        cursorY += hintRect.h + ui::Scale(8);
    }

    if (!editAppDialog_.errorMessage.empty())
    {
        TextTexture errorTexture = CreateTextTexture(renderer, fonts_.tileSubtitle.get(), editAppDialog_.errorMessage, theme_.channelBadge);
        if (errorTexture.texture)
        {
            SDL_Rect errorRect{cursorX, cursorY, errorTexture.width, errorTexture.height};
            RenderTexture(renderer, errorTexture, errorRect);
            cursorY += errorRect.h + ui::Scale(12);
        }
    }

    const int buttonSpacing = ui::Scale(14);
    const int buttonWidth = ui::Scale(160);
    const int buttonHeight = ui::Scale(46);

    editAppDialog_.saveButtonRect = SDL_Rect{
        panelRect.x + panelRect.w - panelPadding - buttonWidth,
        panelRect.y + panelRect.h - panelPadding - buttonHeight,
        buttonWidth,
        buttonHeight};
    editAppDialog_.cancelButtonRect = SDL_Rect{
        editAppDialog_.saveButtonRect.x - buttonSpacing - buttonWidth,
        editAppDialog_.saveButtonRect.y,
        buttonWidth,
        buttonHeight};

    SDL_Color saveFill = color::Mix(theme_.channelBadge, theme_.libraryCardActive, 0.4f);
    SDL_SetRenderDrawColor(renderer, saveFill.r, saveFill.g, saveFill.b, saveFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, editAppDialog_.saveButtonRect, 14);
    SDL_SetRenderDrawColor(renderer, theme_.border.r, theme_.border.g, theme_.border.b, theme_.border.a);
    colony::drawing::RenderRoundedRect(renderer, editAppDialog_.saveButtonRect, 14);

    SDL_Color cancelFill = color::Mix(theme_.libraryCard, theme_.libraryBackground, 0.6f);
    SDL_SetRenderDrawColor(renderer, cancelFill.r, cancelFill.g, cancelFill.b, cancelFill.a);
    colony::drawing::RenderFilledRoundedRect(renderer, editAppDialog_.cancelButtonRect, 14);
    SDL_SetRenderDrawColor(renderer, theme_.border.r, theme_.border.g, theme_.border.b, theme_.border.a);
    colony::drawing::RenderRoundedRect(renderer, editAppDialog_.cancelButtonRect, 14);

    TextTexture saveLabel = CreateTextTexture(renderer, fonts_.button.get(), "Save changes", theme_.heroTitle);
    if (saveLabel.texture)
    {
        SDL_Rect saveRect{
            editAppDialog_.saveButtonRect.x + (editAppDialog_.saveButtonRect.w - saveLabel.width) / 2,
            editAppDialog_.saveButtonRect.y + (editAppDialog_.saveButtonRect.h - saveLabel.height) / 2,
            saveLabel.width,
            saveLabel.height};
        RenderTexture(renderer, saveLabel, saveRect);
    }

    TextTexture cancelLabel = CreateTextTexture(renderer, fonts_.button.get(), "Cancel", theme_.heroTitle);
    if (cancelLabel.texture)
    {
        SDL_Rect cancelRect{
            editAppDialog_.cancelButtonRect.x + (editAppDialog_.cancelButtonRect.w - cancelLabel.width) / 2,
            editAppDialog_.cancelButtonRect.y + (editAppDialog_.cancelButtonRect.h - cancelLabel.height) / 2,
            cancelLabel.width,
            cancelLabel.height};
        RenderTexture(renderer, cancelLabel, cancelRect);
    }

    SDL_SetRenderDrawBlendMode(renderer, previousBlendMode);
}

bool Application::HandleAddAppDialogMouseClick(int x, int y)
{
    if (!addAppDialog_.visible)
    {
        return false;
    }

    auto closeFilterDropdown = [this]() {
        addAppDialog_.filterDropdownOpen = false;
        addAppDialog_.filterDropdownRect = SDL_Rect{0, 0, 0, 0};
        addAppDialog_.filterDropdownOptionRects.clear();
        addAppDialog_.filterDropdownVisible = false;
        addAppDialog_.filterDropdownOptionHeight = 0;
        addAppDialog_.filterDropdownOptionCount = 0;
    };

    if (!PointInRect(addAppDialog_.panelRect, x, y))
    {
        HideAddAppDialog();
        return true;
    }

    if (PointInRect(addAppDialog_.cancelButtonRect, x, y))
    {
        HideAddAppDialog();
        return true;
    }

    if (addAppDialog_.parentAvailable && PointInRect(addAppDialog_.parentButtonRect, x, y))
    {
        closeFilterDropdown();
        addAppDialog_.currentDirectory = addAppDialog_.currentDirectory.parent_path();
        addAppDialog_.selectedIndex = -1;
        addAppDialog_.scrollOffset = 0;
        RefreshAddAppDialogEntries();
        return true;
    }

    const Uint32 buttonState = SDL_GetMouseState(nullptr, nullptr);
    const bool cycleBackward = (buttonState & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0
        && (buttonState & SDL_BUTTON(SDL_BUTTON_LEFT)) == 0;

    const bool clickedFilterButton = addAppDialog_.filterButtonRect.w > 0
        && addAppDialog_.filterButtonRect.h > 0 && PointInRect(addAppDialog_.filterButtonRect, x, y);

    if (addAppDialog_.filterDropdownOpen)
    {
        if (addAppDialog_.filterDropdownRect.w > 0 && addAppDialog_.filterDropdownRect.h > 0
            && PointInRect(addAppDialog_.filterDropdownRect, x, y))
        {
            const auto& fileFilters = GetAddDialogFileTypeFilters();
            for (std::size_t index = 0; index < addAppDialog_.filterDropdownOptionRects.size(); ++index)
            {
                if (index >= fileFilters.size())
                {
                    break;
                }
                if (PointInRect(addAppDialog_.filterDropdownOptionRects[index], x, y))
                {
                    int previous = addAppDialog_.fileTypeFilterIndex;
                    addAppDialog_.fileTypeFilterIndex = static_cast<int>(index);
                    closeFilterDropdown();
                    if (addAppDialog_.fileTypeFilterIndex != previous)
                    {
                        RefreshAddAppDialogEntries();
                    }
                    return true;
                }
            }
            return true;
        }

        if (!clickedFilterButton)
        {
            closeFilterDropdown();
        }
    }

    if (addAppDialog_.sortButtonRect.w > 0 && addAppDialog_.sortButtonRect.h > 0
        && PointInRect(addAppDialog_.sortButtonRect, x, y))
    {
        closeFilterDropdown();
        const auto& sortOptions = GetAddDialogSortOptions();
        const int optionCount = static_cast<int>(sortOptions.size());
        if (optionCount > 0)
        {
            int previous = addAppDialog_.sortModeIndex;
            if (cycleBackward)
            {
                addAppDialog_.sortModeIndex = (addAppDialog_.sortModeIndex - 1 + optionCount) % optionCount;
            }
            else
            {
                addAppDialog_.sortModeIndex = (addAppDialog_.sortModeIndex + 1) % optionCount;
            }

            if (addAppDialog_.sortModeIndex != previous)
            {
                RefreshAddAppDialogEntries();
            }
        }
        return true;
    }

    if (clickedFilterButton)
    {
        if (addAppDialog_.filterDropdownOpen)
        {
            closeFilterDropdown();
        }
        else
        {
            addAppDialog_.filterDropdownOpen = true;
        }
        return true;
    }

    if (PointInRect(addAppDialog_.searchBoxRect, x, y))
    {
        closeFilterDropdown();
        if (!addAppDialog_.searchFocused)
        {
            addAppDialog_.searchFocused = true;
            UpdateTextInputState();
        }
        return true;
    }
    if (addAppDialog_.searchFocused && PointInRect(addAppDialog_.listViewport, x, y))
    {
        addAppDialog_.searchFocused = false;
        UpdateTextInputState();
    }

    const bool canConfirm = addAppDialog_.selectedIndex >= 0
        && addAppDialog_.selectedIndex < static_cast<int>(addAppDialog_.entries.size())
        && !addAppDialog_.entries[addAppDialog_.selectedIndex].isDirectory;

    if (PointInRect(addAppDialog_.confirmButtonRect, x, y))
    {
        closeFilterDropdown();
        if (canConfirm)
        {
            const auto& entry = addAppDialog_.entries[addAppDialog_.selectedIndex];
            if (AddUserApplication(entry.path))
            {
                HideAddAppDialog();
            }
        }
        return true;
    }

    for (std::size_t index = 0; index < addAppDialog_.entries.size(); ++index)
    {
        if (PointInRect(addAppDialog_.entryRects[index], x, y))
        {
            closeFilterDropdown();
            addAppDialog_.errorMessage.clear();
            if (addAppDialog_.searchFocused)
            {
                addAppDialog_.searchFocused = false;
                UpdateTextInputState();
            }
            const auto& entry = addAppDialog_.entries[index];
            if (entry.isDirectory)
            {
                addAppDialog_.currentDirectory = entry.path;
                addAppDialog_.selectedIndex = -1;
                addAppDialog_.scrollOffset = 0;
                RefreshAddAppDialogEntries();
            }
            else
            {
                addAppDialog_.selectedIndex = static_cast<int>(index);
            }
            return true;
        }
    }

    closeFilterDropdown();
    return true;
}

bool Application::HandleAddAppDialogMouseWheel(const SDL_MouseWheelEvent& wheel)
{
    if (!addAppDialog_.visible)
    {
        return false;
    }

    if (addAppDialog_.listViewport.w <= 0 || addAppDialog_.listViewport.h <= 0)
    {
        return true;
    }

    int mouseX = 0;
    int mouseY = 0;
    SDL_GetMouseState(&mouseX, &mouseY);
    if (addAppDialog_.filterDropdownOpen && PointInRect(addAppDialog_.filterDropdownRect, mouseX, mouseY))
    {
        return true;
    }
    if (!PointInRect(addAppDialog_.listViewport, mouseX, mouseY))
    {
        return true;
    }

    int wheelY = wheel.y;
    if (wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
    {
        wheelY = -wheelY;
    }

    if (wheelY == 0)
    {
        return true;
    }

    const int maxScroll = std::max(0, addAppDialog_.contentHeight - addAppDialog_.listViewport.h);
    if (maxScroll <= 0)
    {
        return true;
    }

    addAppDialog_.scrollOffset = std::clamp(
        addAppDialog_.scrollOffset - wheelY * AddDialogRowHeight(),
        0,
        maxScroll);
    return true;
}

bool Application::HandleAddAppDialogKey(SDL_Keycode key)
{
    if (!addAppDialog_.visible)
    {
        return false;
    }

    if (addAppDialog_.filterDropdownOpen)
    {
        addAppDialog_.filterDropdownOpen = false;
        addAppDialog_.filterDropdownRect = SDL_Rect{0, 0, 0, 0};
        addAppDialog_.filterDropdownOptionRects.clear();
        addAppDialog_.filterDropdownVisible = false;
        addAppDialog_.filterDropdownOptionHeight = 0;
        addAppDialog_.filterDropdownOptionCount = 0;
        if (key == SDLK_ESCAPE)
        {
            return true;
        }
    }

    const auto activateDirectory = [this](const std::filesystem::path& directory) {
        addAppDialog_.currentDirectory = directory;
        addAppDialog_.selectedIndex = -1;
        addAppDialog_.scrollOffset = 0;
        RefreshAddAppDialogEntries();
    };

    switch (key)
    {
    case SDLK_ESCAPE:
        HideAddAppDialog();
        return true;
    case SDLK_BACKSPACE:
        if (addAppDialog_.searchFocused)
        {
            if (!addAppDialog_.searchQuery.empty())
            {
                addAppDialog_.searchQuery.pop_back();
                addAppDialog_.scrollOffset = 0;
                RefreshAddAppDialogEntries();
            }
            else if (addAppDialog_.parentAvailable)
            {
                activateDirectory(addAppDialog_.currentDirectory.parent_path());
            }
        }
        else if (addAppDialog_.parentAvailable)
        {
            activateDirectory(addAppDialog_.currentDirectory.parent_path());
        }
        return true;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        if (addAppDialog_.selectedIndex >= 0 && addAppDialog_.selectedIndex < static_cast<int>(addAppDialog_.entries.size()))
        {
            const auto& entry = addAppDialog_.entries[addAppDialog_.selectedIndex];
            if (entry.isDirectory)
            {
                activateDirectory(entry.path);
            }
            else if (AddUserApplication(entry.path))
            {
                HideAddAppDialog();
            }
        }
        return true;
    case SDLK_TAB:
        addAppDialog_.searchFocused = !addAppDialog_.searchFocused;
        UpdateTextInputState();
        return true;
    case SDLK_UP:
        if (!addAppDialog_.entries.empty())
        {
            if (addAppDialog_.selectedIndex <= 0)
            {
                addAppDialog_.selectedIndex = 0;
            }
            else
            {
                --addAppDialog_.selectedIndex;
            }
            const int rowTop = addAppDialog_.listViewport.y
                + addAppDialog_.selectedIndex * AddDialogRowHeight() - addAppDialog_.scrollOffset;
            if (rowTop < addAppDialog_.listViewport.y)
            {
                addAppDialog_.scrollOffset = std::max(0, addAppDialog_.scrollOffset - (addAppDialog_.listViewport.y - rowTop));
            }
        }
        return true;
    case SDLK_DOWN:
        if (!addAppDialog_.entries.empty())
        {
            if (addAppDialog_.selectedIndex < 0)
            {
                addAppDialog_.selectedIndex = 0;
            }
            else if (addAppDialog_.selectedIndex + 1 < static_cast<int>(addAppDialog_.entries.size()))
            {
                ++addAppDialog_.selectedIndex;
            }
            const int rowBottom = addAppDialog_.listViewport.y
                + (addAppDialog_.selectedIndex + 1) * AddDialogRowHeight() - addAppDialog_.scrollOffset;
            if (rowBottom > addAppDialog_.listViewport.y + addAppDialog_.listViewport.h)
            {
                const int maxScroll = std::max(0, addAppDialog_.contentHeight - addAppDialog_.listViewport.h);
                addAppDialog_.scrollOffset = std::min(
                    maxScroll,
                    addAppDialog_.scrollOffset + (rowBottom - (addAppDialog_.listViewport.y + addAppDialog_.listViewport.h)));
            }
        }
        return true;
    default:
        break;
    }

    return true;
}

bool Application::HandleEditUserAppDialogMouseClick(int x, int y)
{
    if (!editAppDialog_.visible)
    {
        return false;
    }

    if (!PointInRect(editAppDialog_.panelRect, x, y))
    {
        HideEditUserAppDialog();
        return true;
    }

    if (PointInRect(editAppDialog_.cancelButtonRect, x, y))
    {
        HideEditUserAppDialog();
        return true;
    }

    if (PointInRect(editAppDialog_.saveButtonRect, x, y))
    {
        if (ApplyEditUserAppChanges())
        {
            HideEditUserAppDialog();
        }
        return true;
    }

    if (PointInRect(editAppDialog_.nameFieldRect, x, y))
    {
        if (!editAppDialog_.nameFocused)
        {
            editAppDialog_.nameFocused = true;
            editAppDialog_.colorFocused = false;
            editAppDialog_.errorMessage.clear();
            UpdateTextInputState();
        }
        return true;
    }

    if (PointInRect(editAppDialog_.colorFieldRect, x, y))
    {
        if (!editAppDialog_.colorFocused)
        {
            editAppDialog_.colorFocused = true;
            editAppDialog_.nameFocused = false;
            editAppDialog_.errorMessage.clear();
            UpdateTextInputState();
        }
        return true;
    }

    if (editAppDialog_.nameFocused || editAppDialog_.colorFocused)
    {
        editAppDialog_.nameFocused = false;
        editAppDialog_.colorFocused = false;
        UpdateTextInputState();
    }

    return true;
}

bool Application::HandleEditUserAppDialogKey(SDL_Keycode key)
{
    if (!editAppDialog_.visible)
    {
        return false;
    }

    switch (key)
    {
    case SDLK_ESCAPE:
        HideEditUserAppDialog();
        return true;
    case SDLK_TAB:
        if (editAppDialog_.nameFocused)
        {
            editAppDialog_.nameFocused = false;
            editAppDialog_.colorFocused = true;
        }
        else
        {
            editAppDialog_.nameFocused = true;
            editAppDialog_.colorFocused = false;
        }
        editAppDialog_.errorMessage.clear();
        UpdateTextInputState();
        return true;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        if (ApplyEditUserAppChanges())
        {
            HideEditUserAppDialog();
        }
        return true;
    case SDLK_BACKSPACE:
        if (editAppDialog_.nameFocused)
        {
            if (!editAppDialog_.nameInput.empty())
            {
                editAppDialog_.nameInput.pop_back();
            }
        }
        else if (editAppDialog_.colorFocused && !editAppDialog_.colorInput.empty())
        {
            editAppDialog_.colorInput.pop_back();
            if (editAppDialog_.colorInput == "#")
            {
                editAppDialog_.colorInput.clear();
            }
        }
        editAppDialog_.errorMessage.clear();
        return true;
    default:
        return true;
    }
}

bool Application::HandleEditUserAppDialogText(const SDL_TextInputEvent& text)
{
    if (!editAppDialog_.visible)
    {
        return false;
    }

    const char* input = text.text;
    if (input == nullptr || input[0] == '\0')
    {
        return false;
    }

    editAppDialog_.errorMessage.clear();

    if (editAppDialog_.nameFocused)
    {
        constexpr std::size_t kMaxNameLength = 80;
        if (editAppDialog_.nameInput.size() >= kMaxNameLength)
        {
            return true;
        }

        const std::size_t remaining = kMaxNameLength - editAppDialog_.nameInput.size();
        editAppDialog_.nameInput.append(input, std::min<std::size_t>(std::strlen(input), remaining));
        return true;
    }

    if (editAppDialog_.colorFocused)
    {
        constexpr std::size_t kMaxColorLength = 7;
        bool appended = false;
        for (const char* ch = input; *ch != '\0'; ++ch)
        {
            const unsigned char value = static_cast<unsigned char>(*ch);
            if (value == '#')
            {
                if (editAppDialog_.colorInput.empty())
                {
                    editAppDialog_.colorInput.push_back('#');
                    appended = true;
                }
                continue;
            }

            if (!std::isxdigit(value))
            {
                continue;
            }

            if (editAppDialog_.colorInput.empty())
            {
                editAppDialog_.colorInput.push_back('#');
            }

            if (editAppDialog_.colorInput.size() >= kMaxColorLength)
            {
                break;
            }

            editAppDialog_.colorInput.push_back(static_cast<char>(std::toupper(value)));
            appended = true;
        }

        return appended;
    }

    return false;
}

bool Application::ApplyEditUserAppChanges()
{
    if (!editAppDialog_.visible)
    {
        return false;
    }

    auto viewIt = content_.views.find(editAppDialog_.programId);
    if (viewIt == content_.views.end())
    {
        editAppDialog_.errorMessage = "Unable to locate the application.";
        return false;
    }

    std::string trimmedName = TrimString(editAppDialog_.nameInput);
    if (trimmedName.empty())
    {
        editAppDialog_.errorMessage = "Display name cannot be empty.";
        return false;
    }

    std::string colorValue = TrimString(editAppDialog_.colorInput);
    if (!colorValue.empty())
    {
        if (!IsValidHexColor(colorValue))
        {
            editAppDialog_.errorMessage = "Use #RGB or #RRGGBB color codes.";
            return false;
        }
        if (colorValue.front() != '#')
        {
            colorValue.insert(colorValue.begin(), '#');
        }
    }
    else
    {
        colorValue = ColorToHex(theme_.channelBadge);
    }

    std::transform(colorValue.begin(), colorValue.end(), colorValue.begin(), [](unsigned char ch) {
        return ch == '#' ? '#' : static_cast<char>(std::toupper(ch));
    });

    SDL_Color accent = color::ParseHexColor(colorValue, theme_.channelBadge);
    SDL_Color gradientStart = color::Mix(accent, theme_.heroGradientFallbackStart, 0.55f);
    SDL_Color gradientEnd = color::Mix(theme_.heroGradientFallbackEnd, accent, 0.35f);

    auto& view = viewIt->second;
    view.heading = trimmedName;
    view.statusMessage = "Ready to launch " + trimmedName;
    view.accentColor = ColorToHex(accent);
    view.heroGradient = {ColorToHex(gradientStart), ColorToHex(gradientEnd)};

    RebuildProgramVisuals();
    viewRegistry_.BindContent(content_);

    if (activeProgramId_ == editAppDialog_.programId)
    {
        ActivateProgram(editAppDialog_.programId);
        UpdateStatusMessage(view.statusMessage);
    }
    else
    {
        UpdateStatusMessage("Updated " + trimmedName);
    }

    editAppDialog_.errorMessage.clear();
    return true;
}

void Application::UpdateTextInputState()
{
    const bool shouldEnable = hubSearchFocused_
        || libraryFilterFocused_
        || (addAppDialog_.visible && addAppDialog_.searchFocused)
        || (editAppDialog_.visible && (editAppDialog_.nameFocused || editAppDialog_.colorFocused))
        || (customThemeDialog_.visible && customThemeDialog_.focusedIndex >= 0);

    if (shouldEnable && !textInputActive_)
    {
        SDL_StartTextInput();
        textInputActive_ = true;
    }
    else if (!shouldEnable && textInputActive_)
    {
        SDL_StopTextInput();
        textInputActive_ = false;
    }
}

bool Application::IsValidHexColor(const std::string& value)
{
    if (value.empty())
    {
        return false;
    }

    std::string cleaned = value;
    if (cleaned.front() == '#')
    {
        cleaned.erase(cleaned.begin());
    }

    if (cleaned.size() != 3 && cleaned.size() != 6)
    {
        return false;
    }

    return std::all_of(cleaned.begin(), cleaned.end(), [](unsigned char ch) { return std::isxdigit(ch) != 0; });
}

std::string Application::TrimString(std::string value)
{
    auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch); });
    value.erase(value.begin(), first);
    auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();
    value.erase(last, value.end());
    return value;
}

int Application::EnsureLocalAppsChannel()
{
    auto toLower = [](std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return value;
    };

    const std::string localIdLower = toLower(std::string(kLocalAppsChannelId));
    const std::string settingsIdLower = toLower(std::string("settings"));

    auto equalsIgnoreCase = [&](const std::string& lhs, const std::string& rhsLower) {
        return toLower(lhs) == rhsLower;
    };

    auto existingIt = std::find_if(content_.channels.begin(), content_.channels.end(), [&](const Channel& channel) {
        return equalsIgnoreCase(channel.id, localIdLower);
    });

    auto settingsIt = std::find_if(content_.channels.begin(), content_.channels.end(), [&](const Channel& channel) {
        return equalsIgnoreCase(channel.id, settingsIdLower);
    });

    const int desiredIndex = settingsIt != content_.channels.end()
        ? static_cast<int>(std::distance(content_.channels.begin(), settingsIt))
        : static_cast<int>(content_.channels.size());

    if (existingIt == content_.channels.end())
    {
        Channel localChannel;
        localChannel.id = std::string(kLocalAppsChannelId);
        localChannel.label = std::string(kLocalAppsChannelLabel);

        auto insertPos = settingsIt != content_.channels.end() ? settingsIt : content_.channels.end();
        const int index = static_cast<int>(std::distance(content_.channels.begin(), insertPos));
        content_.channels.insert(insertPos, std::move(localChannel));

        if (channelSelections_.empty())
        {
            channelSelections_.assign(content_.channels.size(), 0);
        }
        else
        {
            channelSelections_.insert(channelSelections_.begin() + index, 0);
        }

        SyncNavigationEntries();
        return index;
    }

    const int existingIndex = static_cast<int>(std::distance(content_.channels.begin(), existingIt));

    if ((settingsIt != content_.channels.end() && existingIndex == desiredIndex - 1)
        || (settingsIt == content_.channels.end() && existingIndex == desiredIndex - 1))
    {
        return existingIndex;
    }

    Channel localChannel = std::move(*existingIt);
    int preservedSelection = 0;
    bool hasSelectionEntry = !channelSelections_.empty() && existingIndex < static_cast<int>(channelSelections_.size());
    if (hasSelectionEntry)
    {
        preservedSelection = channelSelections_[existingIndex];
        channelSelections_.erase(channelSelections_.begin() + existingIndex);
    }

    existingIt = content_.channels.erase(existingIt);

    settingsIt = std::find_if(content_.channels.begin(), content_.channels.end(), [&](const Channel& channel) {
        return equalsIgnoreCase(channel.id, settingsIdLower);
    });

    auto insertPos = settingsIt != content_.channels.end() ? settingsIt : content_.channels.end();
    auto insertedIt = content_.channels.insert(insertPos, std::move(localChannel));
    const int index = static_cast<int>(std::distance(content_.channels.begin(), insertedIt));

    if (channelSelections_.empty())
    {
        channelSelections_.assign(content_.channels.size(), 0);
    }
    else
    {
        channelSelections_.insert(channelSelections_.begin() + index, preservedSelection);
    }

    SyncNavigationEntries();
    return index;
}

bool Application::AddUserApplication(const std::filesystem::path& executablePath)
{
    std::error_code ec;
    if (executablePath.empty() || std::filesystem::is_directory(executablePath, ec)
        || !std::filesystem::exists(executablePath, ec))
    {
        addAppDialog_.errorMessage = "Select a valid executable file.";
        return false;
    }

    std::filesystem::path resolvedPath = executablePath;
    std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(executablePath, ec);
    if (!ec && !canonicalPath.empty())
    {
        resolvedPath = canonicalPath;
    }

    const std::string programId = "CUSTOM_APP_" + std::to_string(nextCustomProgramId_++);
    const std::string displayName = MakeDisplayNameFromPath(resolvedPath);

    ViewContent viewContent;
    viewContent.heading = displayName;
    viewContent.tagline = "Launch an external application directly from Colony.";
    viewContent.paragraphs = {
        "Executable path: " + resolvedPath.string(),
        "Launch opens the binary in a separate process."};
    viewContent.heroHighlights = {
        std::string("Manually added to the ") + std::string(kLocalAppsChannelLabel) + " category",
        "Launches without leaving Colony",
        "Remove or update by editing your configuration"};
    viewContent.primaryActionLabel = "Launch";
    viewContent.statusMessage = "Ready to launch " + displayName;
    viewContent.version = resolvedPath.extension().empty() ? "Binary" : "Binary " + resolvedPath.extension().string();
    viewContent.installState = "Manual entry";
    viewContent.availability = "Ready";
    viewContent.lastLaunched = "Never launched";

    SDL_Color accentColor = color::Mix(theme_.channelBadge, theme_.heroTitle, 0.45f);
    SDL_Color gradientStart = color::Mix(accentColor, theme_.heroGradientFallbackStart, 0.55f);
    SDL_Color gradientEnd = color::Mix(theme_.heroGradientFallbackEnd, accentColor, 0.35f);
    viewContent.accentColor = ColorToHex(accentColor);
    viewContent.heroGradient = {ColorToHex(gradientStart), ColorToHex(gradientEnd)};

    content_.views[programId] = viewContent;

    viewRegistry_.Register(viewFactory_.CreateSimpleTextView(programId));
    viewRegistry_.BindContent(content_);

    userAppExecutables_[programId] = resolvedPath;

    const int targetChannelIndex = EnsureLocalAppsChannel();
    if (targetChannelIndex < 0 || targetChannelIndex >= static_cast<int>(content_.channels.size()))
    {
        addAppDialog_.errorMessage = "Unable to locate a channel for the application.";
        return false;
    }

    auto& targetChannel = content_.channels[static_cast<std::size_t>(targetChannelIndex)];
    targetChannel.programs.emplace_back(programId);
    if (static_cast<std::size_t>(targetChannelIndex) >= channelSelections_.size())
    {
        channelSelections_.resize(content_.channels.size(), 0);
    }

    channelSelections_[targetChannelIndex] = static_cast<int>(targetChannel.programs.size()) - 1;

    RebuildProgramVisuals();

    if (targetChannelIndex == activeChannelIndex_)
    {
        ActivateProgramInChannel(channelSelections_[targetChannelIndex]);
    }
    else
    {
        navigationController_.Activate(targetChannelIndex);
    }

    UpdateStatusMessage(viewContent.statusMessage);
    addAppDialog_.errorMessage.clear();
    return true;
}

void Application::LaunchUserApp(const std::filesystem::path& executablePath, const std::string& programId)
{
    std::error_code ec;
    if (!std::filesystem::exists(executablePath, ec))
    {
        UpdateStatusMessage("Executable missing: " + executablePath.string());
        return;
    }

    const auto viewIt = content_.views.find(programId);
    const std::string displayName = viewIt != content_.views.end() ? viewIt->second.heading : executablePath.filename().string();
    UpdateStatusMessage("Launching " + displayName + "...");

#if defined(_WIN32)
    std::string command = "start \"\" \"" + executablePath.string() + "\"";
#else
    std::string command = "\"" + executablePath.string() + "\" &";
#endif

    std::thread launcherThread([command]() { std::system(command.c_str()); });
    launcherThread.detach();

    if (viewIt != content_.views.end())
    {
        auto& view = viewIt->second;
        std::time_t nowTime = std::time(nullptr);
        std::tm local{};
#if defined(_WIN32)
        localtime_s(&local, &nowTime);
#else
        localtime_r(&nowTime, &local);
#endif
        std::ostringstream timeStream;
        timeStream << "Launched " << std::put_time(&local, "%H:%M");
        view.lastLaunched = timeStream.str();
        view.statusMessage = "Launch command sent to " + displayName + ".";
        UpdateStatusMessage(view.statusMessage);
    }

    RebuildProgramVisuals();
}

void Application::ChangeLanguage(const std::string& languageId)
{
    if (languageId.empty() || languageId == settingsService_.ActiveLanguageId())
    {
        return;
    }

    if (!localizationManager_.LoadLanguage(languageId))
    {
        std::cerr << "Unable to load localization for language '" << languageId << "'." << '\n';
        return;
    }

    settingsService_.SetActiveLanguageId(languageId);
    if (!InitializeFonts())
    {
        std::cerr << "Failed to reload fonts for language '" << languageId << "'." << '\n';
        return;
    }
    RebuildTheme();
}

std::filesystem::path Application::ResolveContentPath()
{
    constexpr char kContentFile[] = "assets/content/app_content.json";
    return colony::paths::ResolveAssetPath(kContentFile);
}

std::filesystem::path Application::ResolveLocalizationDirectory()
{
    constexpr char kLocalizationDir[] = "assets/content/i18n";
    return colony::paths::ResolveAssetDirectory(kLocalizationDir);
}

std::filesystem::path Application::ResolveSettingsPath() const
{
    constexpr char kSettingsFileName[] = "settings.json";

    if (char* prefPath = SDL_GetPrefPath("OpenAI", "Colony"); prefPath != nullptr)
    {
        std::filesystem::path base{prefPath};
        SDL_free(prefPath);
        if (!base.empty())
        {
            return base / kSettingsFileName;
        }
    }

    return std::filesystem::path{kSettingsFileName};
}

bool Application::PointInRect(const SDL_Rect& rect, int x, int y) const
{
    if (rect.w <= 0 || rect.h <= 0)
    {
        return false;
    }

    const int maxX = rect.x + rect.w;
    const int maxY = rect.y + rect.h;
    return x >= rect.x && x < maxX && y >= rect.y && y < maxY;
}

std::string Application::GetLocalizedString(std::string_view key) const
{
    return localizationManager_.GetString(key);
}

std::string Application::GetLocalizedString(std::string_view key, std::string_view fallback) const
{
    return localizationManager_.GetStringOrDefault(key, fallback);
}

} // namespace colony
