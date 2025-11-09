#include "app/application.hpp"

#include "core/content_loader.hpp"
#include "ui/layout.hpp"
#include "ui/theme.hpp"
#include "utils/color.hpp"
#include "utils/font_manager.hpp"
#include "utils/text.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <system_error>

namespace colony
{
namespace
{
bool SDLCallSucceeded(int result)
{
    if (result < 0)
    {
        std::cerr << "SDL error: " << SDL_GetError() << '\n';
        return false;
    }
    return true;
}

} // namespace

Application::Application() = default;

int Application::Run()
{
    if (!InitializeSDL())
    {
        return EXIT_FAILURE;
    }

    struct TtfGuard
    {
        ~TtfGuard() { TTF_Quit(); }
    } ttfGuard;

    if (!CreateWindowAndRenderer())
    {
        SDL_Quit();
        return EXIT_FAILURE;
    }

    if (!InitializeFonts())
    {
        SDL_Quit();
        return EXIT_FAILURE;
    }

    if (!LoadContent())
    {
        SDL_Quit();
        return EXIT_FAILURE;
    }

    if (!InitializeLocalization())
    {
        SDL_Quit();
        return EXIT_FAILURE;
    }

    InitializeNavigation();
    InitializeViews();
    RebuildTheme();

    channelButtonRects_.assign(content_.channels.size(), SDL_Rect{});

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

        const auto reduceMotionIt = basicToggleStates_.find("reduced_motion");
        const bool reduceMotion = reduceMotionIt != basicToggleStates_.end() && reduceMotionIt->second;
        if (!reduceMotion)
        {
            animationTimeSeconds_ += deltaSeconds;
        }

        while (SDL_PollEvent(&event))
        {
            HandleEvent(event, running);
        }

        RenderFrame(reduceMotion ? 0.0 : deltaSeconds);
    }

    SDL_Quit();
    return EXIT_SUCCESS;
}

bool Application::InitializeSDL()
{
    if (!SDLCallSucceeded(SDL_Init(SDL_INIT_VIDEO)))
    {
        return false;
    }

    if (TTF_Init() == -1)
    {
        std::cerr << "Failed to initialize SDL_ttf: " << TTF_GetError() << '\n';
        return false;
    }

    return true;
}

bool Application::CreateWindowAndRenderer()
{
    window_ = sdl::WindowHandle{SDL_CreateWindow(
        "Colony Launcher",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        kWindowWidth,
        kWindowHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE)};
    if (!window_)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << '\n';
        return false;
    }

    renderer_ = sdl::RendererHandle{SDL_CreateRenderer(
        window_.get(),
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE)};
    if (!renderer_)
    {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << '\n';
        return false;
    }

    return true;
}

bool Application::InitializeFonts()
{
    const std::string fontPath = fonts::ResolveFontPath();
    if (fontPath.empty())
    {
        std::cerr << "Unable to locate a usable font file. Provide DejaVuSans.ttf in assets/fonts or set COLONY_FONT_PATH." << '\n';
        return false;
    }

    auto openFont = [&](int size) { return sdl::FontHandle{TTF_OpenFont(fontPath.c_str(), ui::ScaleDynamic(size))}; };

    fonts_.brand = openFont(30);
    fonts_.navigation = openFont(16);
    fonts_.channel = openFont(20);
    fonts_.tileTitle = openFont(20);
    fonts_.tileSubtitle = openFont(14);
    fonts_.tileMeta = openFont(14);
    fonts_.heroTitle = openFont(40);
    fonts_.heroSubtitle = openFont(20);
    fonts_.heroBody = openFont(16);
    fonts_.patchTitle = openFont(16);
    fonts_.patchBody = openFont(14);
    fonts_.button = openFont(20);
    fonts_.status = openFont(14);

    if (!fonts_.brand || !fonts_.navigation || !fonts_.channel || !fonts_.tileTitle || !fonts_.tileSubtitle || !fonts_.tileMeta
        || !fonts_.heroTitle || !fonts_.heroSubtitle || !fonts_.heroBody || !fonts_.patchTitle || !fonts_.patchBody
        || !fonts_.button || !fonts_.status)
    {
        std::cerr << "Failed to load required fonts from " << fontPath << ": " << TTF_GetError() << '\n';
        return false;
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
    return true;
}

bool Application::InitializeLocalization()
{
    localizationManager_.SetResourceDirectory(ResolveLocalizationDirectory());
    localizationManager_.SetFallbackLanguage("en");

    if (!localizationManager_.LoadLanguage(activeLanguageId_))
    {
        std::cerr << "Failed to load localization for language '" << activeLanguageId_ << "'." << '\n';
        if (activeLanguageId_ != localizationManager_.FallbackLanguage()
            && localizationManager_.LoadLanguage(localizationManager_.FallbackLanguage()))
        {
            activeLanguageId_ = localizationManager_.FallbackLanguage();
        }
        else
        {
            return false;
        }
    }

    return true;
}

void Application::InitializeNavigation()
{
    std::vector<std::string> entries;
    entries.reserve(content_.channels.size());
    for (const auto& channel : content_.channels)
    {
        entries.emplace_back(channel.id);
    }

    navigationController_.SetEntries(std::move(entries));
    navigationController_.OnSelectionChanged([this](int index) { ActivateChannel(index); });
    ActivateChannel(navigationController_.ActiveIndex());
}

void Application::InitializeViews()
{
    for (const auto& [id, view] : content_.views)
    {
        if (id == kSettingsProgramId)
        {
            continue;
        }
        viewRegistry_.Register(viewFactory_.CreateSimpleTextView(id));
    }
    viewRegistry_.BindContent(content_);
}

void Application::RebuildTheme()
{
    theme_ = themeManager_.ActiveScheme().colors;

    const auto localize = [this](std::string_view key) { return GetLocalizedString(key); };

    navigationRail_.Build(
        renderer_.get(),
        fonts_.brand.get(),
        fonts_.navigation.get(),
        fonts_.tileMeta.get(),
        content_,
        theme_);

    libraryPanel_.Build(renderer_.get(), fonts_.tileMeta.get(), theme_, localize);
    heroPanel_.Build(renderer_.get(), fonts_.tileMeta.get(), theme_, localize);
    settingsPanel_.Build(
        renderer_.get(),
        fonts_.heroTitle.get(),
        fonts_.heroBody.get(),
        theme_.heroTitle,
        theme_.heroBody,
        themeManager_,
        localize);
    settingsScrollOffset_ = 0;

    RebuildProgramVisuals();
    UpdateStatusMessage(statusBuffer_.empty() && !activeProgramId_.empty()
            ? content_.views.at(activeProgramId_).statusMessage
            : statusBuffer_);

    viewContext_.renderer = renderer_.get();
    viewContext_.headingFont = fonts_.heroTitle.get();
    viewContext_.paragraphFont = fonts_.heroBody.get();
    viewContext_.buttonFont = fonts_.button.get();
    viewContext_.primaryColor = theme_.heroTitle;
    viewContext_.mutedColor = theme_.heroBody;
    UpdateViewContextAccent();

    if (arcadeOverlay_.active)
    {
        BuildArcadeOverlayTextures();
    }

    if (!activeProgramId_.empty())
    {
        viewRegistry_.Activate(activeProgramId_, viewContext_);
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
                renderer_.get(),
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
    HideArcadeOverlay();

    if (programId.empty())
    {
        activeProgramId_.clear();
        heroActionRect_.reset();
        viewRegistry_.DeactivateActive();
        return;
    }

    activeProgramId_ = programId;

    if (activeProgramId_ == kSettingsProgramId)
    {
        settingsScrollOffset_ = 0;
        viewRegistry_.DeactivateActive();
        UpdateStatusMessage(content_.views[activeProgramId_].statusMessage);
        UpdateViewContextAccent();
        return;
    }

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

void Application::HandleEvent(const SDL_Event& event, bool& running)
{
    if (event.type == SDL_QUIT)
    {
        running = false;
        return;
    }

    if (arcadeOverlay_.active)
    {
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
        {
            HideArcadeOverlay();
            if (auto it = content_.views.find(activeProgramId_); it != content_.views.end())
            {
                UpdateStatusMessage(it->second.statusMessage);
            }
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
        {
            HideArcadeOverlay();
            if (auto it = content_.views.find(activeProgramId_); it != content_.views.end())
            {
                UpdateStatusMessage(it->second.statusMessage);
            }
        }
        return;
    }

    switch (event.type)
    {
    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT)
        {
            HandleMouseClick(event.button.x, event.button.y);
        }
        break;
    case SDL_MOUSEWHEEL:
        HandleMouseWheel(event.wheel);
        break;
    case SDL_KEYDOWN:
        HandleKeyDown(event.key.keysym.sym);
        break;
    default:
        break;
    }
}

void Application::HandleMouseClick(int x, int y)
{
    for (std::size_t i = 0; i < channelButtonRects_.size(); ++i)
    {
        if (PointInRect(channelButtonRects_[i], x, y))
        {
            navigationController_.Activate(static_cast<int>(i));
            return;
        }
    }

    for (std::size_t i = 0; i < programTileRects_.size(); ++i)
    {
        if (PointInRect(programTileRects_[i], x, y))
        {
            ActivateProgramInChannel(static_cast<int>(i));
            return;
        }
    }

    if (activeProgramId_ == kSettingsProgramId)
    {
        for (const auto& region : settingsRenderResult_.interactiveRegions)
        {
            if (!PointInRect(region.rect, x, y))
            {
                continue;
            }

            switch (region.type)
            {
            case ui::SettingsPanel::RenderResult::InteractionType::ThemeSelection:
                if (themeManager_.SetActiveScheme(region.id))
                {
                    RebuildTheme();
                }
                break;
            case ui::SettingsPanel::RenderResult::InteractionType::LanguageSelection:
                ChangeLanguage(region.id);
                break;
            case ui::SettingsPanel::RenderResult::InteractionType::Toggle:
                if (auto it = basicToggleStates_.find(region.id); it != basicToggleStates_.end())
                {
                    it->second = !it->second;
                }
                break;
            }
            return;
        }
    }
    else if (heroActionRect_.has_value() && PointInRect(*heroActionRect_, x, y))
    {
        viewRegistry_.TriggerPrimaryAction(statusBuffer_);
        UpdateStatusMessage(statusBuffer_);
        if (activeProgramId_ == kOrbitalArcadeProgramId)
        {
            ShowArcadeOverlay();
        }
    }
}

void Application::HandleMouseWheel(const SDL_MouseWheelEvent& wheel)
{
    if (activeProgramId_ == kSettingsProgramId)
    {
        if (settingsRenderResult_.viewport.w <= 0 || settingsRenderResult_.viewport.h <= 0)
        {
            return;
        }

        int mouseX = 0;
        int mouseY = 0;
        SDL_GetMouseState(&mouseX, &mouseY);
        if (!PointInRect(settingsRenderResult_.viewport, mouseX, mouseY))
        {
            return;
        }

        int wheelY = wheel.y;
        if (wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
        {
            wheelY = -wheelY;
        }

        if (wheelY == 0)
        {
            return;
        }

        const int maxScroll = std::max(0, settingsRenderResult_.contentHeight - settingsRenderResult_.viewport.h);
        if (maxScroll <= 0)
        {
            return;
        }

        constexpr int kScrollStep = 48;
        const int delta = -wheelY * kScrollStep;
        settingsScrollOffset_ = std::clamp(settingsScrollOffset_ + delta, 0, maxScroll);
        return;
    }

    auto visualsIt = programVisuals_.find(activeProgramId_);
    if (visualsIt == programVisuals_.end())
    {
        return;
    }

    auto& visuals = visualsIt->second;
    if (visuals.sectionsViewport.w <= 0 || visuals.sectionsViewport.h <= 0)
    {
        return;
    }

    int mouseX = 0;
    int mouseY = 0;
    SDL_GetMouseState(&mouseX, &mouseY);
    if (!PointInRect(visuals.sectionsViewport, mouseX, mouseY))
    {
        return;
    }

    int wheelY = wheel.y;
    if (wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
    {
        wheelY = -wheelY;
    }

    if (wheelY == 0)
    {
        return;
    }

    const int viewportHeight = std::max(0, visuals.sectionsViewportContentHeight);
    const int maxScroll = std::max(0, visuals.sectionsContentHeight - viewportHeight);
    if (maxScroll <= 0)
    {
        return;
    }

    constexpr int kScrollStep = ui::Scale(40);
    const int delta = -wheelY * kScrollStep;
    visuals.sectionsScrollOffset = std::clamp(visuals.sectionsScrollOffset + delta, 0, maxScroll);
}

void Application::HandleKeyDown(SDL_Keycode key)
{
    switch (key)
    {
    case SDLK_UP:
        ActivateProgramInChannel(channelSelections_[activeChannelIndex_] - 1);
        break;
    case SDLK_DOWN:
        ActivateProgramInChannel(channelSelections_[activeChannelIndex_] + 1);
        break;
    case SDLK_LEFT:
        navigationController_.Activate(activeChannelIndex_ - 1);
        break;
    case SDLK_RIGHT:
        navigationController_.Activate(activeChannelIndex_ + 1);
        break;
    default:
        break;
    }
}

void Application::RenderFrame(double deltaSeconds)
{
    if (!renderer_)
    {
        return;
    }

    int outputWidth = 0;
    int outputHeight = 0;
    SDL_GetRendererOutputSize(renderer_.get(), &outputWidth, &outputHeight);

    SDL_SetRenderDrawColor(renderer_.get(), theme_.background.r, theme_.background.g, theme_.background.b, theme_.background.a);
    SDL_RenderClear(renderer_.get());

    const double timeSeconds = animationTimeSeconds_;

    const int navRailWidth = ui::Scale(88);
    const int libraryWidth = std::clamp(outputWidth / 4, ui::Scale(280), ui::Scale(320));
    SDL_Rect navRailRect{0, 0, navRailWidth, outputHeight};
    SDL_SetRenderDrawColor(renderer_.get(), theme_.navRail.r, theme_.navRail.g, theme_.navRail.b, theme_.navRail.a);
    SDL_RenderFillRect(renderer_.get(), &navRailRect);

    SDL_Rect libraryRect{navRailWidth, 0, libraryWidth, outputHeight};
    SDL_SetRenderDrawColor(
        renderer_.get(),
        theme_.libraryBackground.r,
        theme_.libraryBackground.g,
        theme_.libraryBackground.b,
        theme_.libraryBackground.a);
    SDL_RenderFillRect(renderer_.get(), &libraryRect);

    const SDL_Rect heroRect{navRailWidth + libraryWidth, 0, outputWidth - navRailWidth - libraryWidth, outputHeight};
    const auto visualsIt = programVisuals_.find(activeProgramId_);
    const ui::ProgramVisuals* activeVisuals = visualsIt != programVisuals_.end() ? &visualsIt->second : nullptr;
    if (activeVisuals != nullptr)
    {
        const float gradientPulse = static_cast<float>(0.5 + 0.5 * std::sin(timeSeconds * 0.6));
        SDL_Color gradientStart = color::Mix(activeVisuals->gradientStart, activeVisuals->accent, 0.15f + 0.1f * gradientPulse);
        SDL_Color gradientEnd = color::Mix(activeVisuals->gradientEnd, theme_.heroGradientFallbackEnd, 0.2f * gradientPulse);
        color::RenderVerticalGradient(renderer_.get(), heroRect, gradientStart, gradientEnd);
    }
    else
    {
        const float gradientPulse = static_cast<float>(0.5 + 0.5 * std::sin(timeSeconds * 0.8));
        SDL_Color gradientStart = color::Mix(
            theme_.heroGradientFallbackStart,
            theme_.channelBadge,
            0.1f + 0.15f * gradientPulse);
        SDL_Color gradientEnd = color::Mix(
            theme_.heroGradientFallbackEnd,
            theme_.border,
            0.1f * static_cast<float>(std::cos(timeSeconds * 0.6) * 0.5 + 0.5));
        color::RenderVerticalGradient(renderer_.get(), heroRect, gradientStart, gradientEnd);
    }

    SDL_Rect navDivider{navRailRect.x + navRailRect.w - 2, 0, 2, outputHeight};
    SDL_SetRenderDrawColor(renderer_.get(), theme_.border.r, theme_.border.g, theme_.border.b, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer_.get(), &navDivider);

    const int statusBarHeight = ui::Scale(kStatusBarHeight);

    channelButtonRects_ = navigationRail_.Render(
        renderer_.get(),
        theme_,
        navRailRect,
        statusBarHeight,
        content_,
        channelSelections_,
        activeChannelIndex_,
        programVisuals_,
        timeSeconds);

    const auto libraryResult = libraryPanel_.Render(
        renderer_.get(),
        theme_,
        libraryRect,
        content_,
        activeChannelIndex_,
        channelSelections_,
        programVisuals_,
        fonts_.channel.get(),
        timeSeconds,
        deltaSeconds);
    programTileRects_ = libraryResult.tileRects;

    heroActionRect_.reset();
    SDL_Rect previousSettingsViewport = settingsRenderResult_.viewport;
    const int previousSettingsContentHeight = settingsRenderResult_.contentHeight;

    settingsRenderResult_.interactiveRegions.clear();
    settingsRenderResult_.contentHeight = 0;
    settingsRenderResult_.viewport = SDL_Rect{0, 0, 0, 0};

    if (activeProgramId_ == kSettingsProgramId)
    {
        const int previousViewportHeight = previousSettingsViewport.h;
        const int previousMaxScroll = std::max(0, previousSettingsContentHeight - previousViewportHeight);
        settingsScrollOffset_ = std::clamp(settingsScrollOffset_, 0, previousMaxScroll);

        heroPanel_.RenderSettings(
            renderer_.get(),
            theme_,
            heroRect,
            settingsPanel_,
            settingsScrollOffset_,
            themeManager_.ActiveScheme().id,
            activeLanguageId_,
            basicToggleStates_,
            settingsRenderResult_,
            timeSeconds);

        if (settingsRenderResult_.viewport.w > 0 && settingsRenderResult_.viewport.h > 0)
        {
            const int maxScroll = std::max(0, settingsRenderResult_.contentHeight - settingsRenderResult_.viewport.h);
            settingsScrollOffset_ = std::clamp(settingsScrollOffset_, 0, maxScroll);
        }
    }
    else if (activeVisuals != nullptr)
    {
        const auto heroResult = heroPanel_.RenderHero(
            renderer_.get(),
            theme_,
            heroRect,
            visualsIt->second,
            fonts_.heroBody.get(),
            fonts_.patchTitle.get(),
            fonts_.patchBody.get(),
            timeSeconds,
            deltaSeconds);
        heroActionRect_ = heroResult.actionButtonRect;
    }

    if (activeProgramId_ != kSettingsProgramId)
    {
        settingsScrollOffset_ = 0;
    }

    heroPanel_.RenderStatusBar(renderer_.get(), theme_, heroRect, statusBarHeight, activeVisuals, timeSeconds);

    RenderArcadeOverlay(outputWidth, outputHeight);

    SDL_RenderPresent(renderer_.get());
}

void Application::ShowArcadeOverlay()
{
    if (!renderer_)
    {
        return;
    }

    arcadeOverlay_.active = true;
    BuildArcadeOverlayTextures();

    statusBuffer_ = "Orbital Arcade placeholder environment active.";
    UpdateStatusMessage(statusBuffer_);
}

void Application::HideArcadeOverlay()
{
    if (!arcadeOverlay_.active)
    {
        return;
    }

    arcadeOverlay_.active = false;
    arcadeOverlay_.title = {};
    arcadeOverlay_.message = {};
    arcadeOverlay_.instructions = {};
}

void Application::BuildArcadeOverlayTextures()
{
    if (!arcadeOverlay_.active || !renderer_)
    {
        return;
    }

    arcadeOverlay_.title = {};
    arcadeOverlay_.message = {};
    arcadeOverlay_.instructions = {};

    const SDL_Color titleColor{255, 255, 255, SDL_ALPHA_OPAQUE};
    const SDL_Color messageColor{theme_.heroBody.r, theme_.heroBody.g, theme_.heroBody.b, SDL_ALPHA_OPAQUE};
    const SDL_Color instructionsColor{theme_.muted.r, theme_.muted.g, theme_.muted.b, SDL_ALPHA_OPAQUE};

    if (fonts_.heroTitle)
    {
        arcadeOverlay_.title = CreateTextTexture(renderer_.get(), fonts_.heroTitle.get(), "Orbital Arcade", titleColor);
    }

    if (fonts_.heroBody)
    {
        arcadeOverlay_.message = CreateTextTexture(
            renderer_.get(),
            fonts_.heroBody.get(),
            "Simulation core placeholder ready for co-op development.",
            messageColor);
    }

    if (fonts_.tileMeta)
    {
        arcadeOverlay_.instructions = CreateTextTexture(
            renderer_.get(),
            fonts_.tileMeta.get(),
            "Press ESC or click to return to the launcher.",
            instructionsColor);
    }

    const auto mixComponent = [](Uint8 accentComponent, int base) {
        return static_cast<Uint8>(std::clamp(base + static_cast<int>(accentComponent) / 4, 0, 255));
    };

    const SDL_Color accent = viewContext_.accentColor;
    arcadeOverlay_.background = SDL_Color{
        mixComponent(accent.r, 12),
        mixComponent(accent.g, 8),
        mixComponent(accent.b, 28),
        235};
}

void Application::RenderArcadeOverlay(int outputWidth, int outputHeight)
{
    if (!arcadeOverlay_.active || !renderer_)
    {
        return;
    }

    SDL_BlendMode previousBlendMode = SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(renderer_.get(), &previousBlendMode);
    SDL_SetRenderDrawBlendMode(renderer_.get(), SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(
        renderer_.get(),
        arcadeOverlay_.background.r,
        arcadeOverlay_.background.g,
        arcadeOverlay_.background.b,
        arcadeOverlay_.background.a);

    SDL_Rect overlayRect{0, 0, outputWidth, outputHeight};
    SDL_RenderFillRect(renderer_.get(), &overlayRect);

    SDL_SetRenderDrawBlendMode(renderer_.get(), previousBlendMode);

    const int centerX = outputWidth / 2;
    int currentY = outputHeight / 3;

    if (arcadeOverlay_.title.texture)
    {
        SDL_Rect titleRect{
            centerX - arcadeOverlay_.title.width / 2,
            currentY,
            arcadeOverlay_.title.width,
            arcadeOverlay_.title.height};
        RenderTexture(renderer_.get(), arcadeOverlay_.title, titleRect);
        currentY = titleRect.y + titleRect.h + ui::Scale(28);
    }

    if (arcadeOverlay_.message.texture)
    {
        SDL_Rect messageRect{
            centerX - arcadeOverlay_.message.width / 2,
            currentY,
            arcadeOverlay_.message.width,
            arcadeOverlay_.message.height};
        RenderTexture(renderer_.get(), arcadeOverlay_.message, messageRect);
        currentY = messageRect.y + messageRect.h + ui::Scale(18);
    }

    if (arcadeOverlay_.instructions.texture)
    {
        SDL_Rect instructionsRect{
            centerX - arcadeOverlay_.instructions.width / 2,
            currentY,
            arcadeOverlay_.instructions.width,
            arcadeOverlay_.instructions.height};
        RenderTexture(renderer_.get(), arcadeOverlay_.instructions, instructionsRect);
    }

    const SDL_Color frameColor = viewContext_.accentColor;
    SDL_SetRenderDrawBlendMode(renderer_.get(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_.get(), frameColor.r, frameColor.g, frameColor.b, 180);
    const int frameMargin = ui::Scale(64);
    SDL_Rect frameRect{frameMargin, frameMargin, outputWidth - 2 * frameMargin, outputHeight - 2 * frameMargin};
    SDL_RenderDrawRect(renderer_.get(), &frameRect);
    SDL_SetRenderDrawBlendMode(renderer_.get(), previousBlendMode);
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
            renderer_.get(),
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

void Application::ChangeLanguage(const std::string& languageId)
{
    if (languageId.empty() || languageId == activeLanguageId_)
    {
        return;
    }

    if (!localizationManager_.LoadLanguage(languageId))
    {
        std::cerr << "Unable to load localization for language '" << languageId << "'." << '\n';
        return;
    }

    activeLanguageId_ = languageId;
    RebuildTheme();
}

std::filesystem::path Application::ResolveContentPath()
{
    constexpr char kContentFile[] = "assets/content/app_content.json";

    std::filesystem::path candidate{kContentFile};
    std::error_code error;
    if (std::filesystem::exists(candidate, error))
    {
        return candidate;
    }

    if (char* basePath = SDL_GetBasePath(); basePath != nullptr)
    {
        std::filesystem::path base{basePath};
        SDL_free(basePath);
        std::filesystem::path baseCandidate = base / kContentFile;
        if (std::filesystem::exists(baseCandidate, error))
        {
            return baseCandidate;
        }
    }

    return candidate;
}

std::filesystem::path Application::ResolveLocalizationDirectory()
{
    constexpr char kLocalizationDir[] = "assets/content/i18n";

    std::filesystem::path candidate{kLocalizationDir};
    std::error_code error;
    if (std::filesystem::is_directory(candidate, error))
    {
        return candidate;
    }

    if (char* basePath = SDL_GetBasePath(); basePath != nullptr)
    {
        std::filesystem::path base{basePath};
        SDL_free(basePath);
        std::filesystem::path baseCandidate = base / kLocalizationDir;
        if (std::filesystem::is_directory(baseCandidate, error))
        {
            return baseCandidate;
        }
    }

    return candidate;
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
