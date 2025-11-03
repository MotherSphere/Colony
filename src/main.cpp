#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <algorithm>
#include <filesystem>
#include <system_error>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "content_loader.hpp"
#include "ui/hero_panel.hpp"
#include "ui/library_panel.hpp"
#include "ui/navigation.hpp"
#include "ui/program_visuals.hpp"
#include "ui/settings_panel.hpp"
#include "ui/theme.hpp"
#include "utils/color.hpp"
#include "utils/font_manager.hpp"
#include "utils/sdl_wrappers.hpp"
#include "utils/text.hpp"

namespace
{
constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 768;
constexpr int kStatusBarHeight = 52;
constexpr char kSettingsProgramId[] = "SETTINGS";
constexpr char kContentFile[] = "assets/content/app_content.json";

std::filesystem::path ResolveContentPath()
{
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

bool PointInRect(const SDL_Rect& rect, int x, int y)
{
    return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
}

} // namespace

int main(int argc, char** argv)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "Failed to initialize SDL2: " << SDL_GetError() << '\n';
        return EXIT_FAILURE;
    }

    colony::sdl::WindowHandle window{SDL_CreateWindow(
        "Colony Launcher",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        kWindowWidth,
        kWindowHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE)};

    if (!window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << '\n';
        SDL_Quit();
        return EXIT_FAILURE;
    }

    colony::sdl::RendererHandle renderer{SDL_CreateRenderer(
        window.get(),
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE)};
    if (!renderer)
    {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << '\n';
        SDL_Quit();
        return EXIT_FAILURE;
    }

    if (TTF_Init() == -1)
    {
        std::cerr << "Failed to initialize SDL_ttf: " << TTF_GetError() << '\n';
        SDL_Quit();
        return EXIT_FAILURE;
    }

    struct TtfGuard
    {
        ~TtfGuard() { TTF_Quit(); }
    } ttfGuard;

    const std::string fontPath = colony::fonts::ResolveFontPath();
    if (fontPath.empty())
    {
        std::cerr << "Unable to locate a usable font file. Provide DejaVuSans.ttf in assets/fonts or set COLONY_FONT_PATH." << '\n';
        SDL_Quit();
        return EXIT_FAILURE;
    }

    auto openFont = [&](int size) { return colony::sdl::FontHandle{TTF_OpenFont(fontPath.c_str(), size)}; };

    colony::sdl::FontHandle brandFont = openFont(32);
    colony::sdl::FontHandle navFont = openFont(18);
    colony::sdl::FontHandle channelFont = openFont(22);
    colony::sdl::FontHandle tileTitleFont = openFont(22);
    colony::sdl::FontHandle tileMetaFont = openFont(16);
    colony::sdl::FontHandle heroTitleFont = openFont(46);
    colony::sdl::FontHandle heroSubtitleFont = openFont(22);
    colony::sdl::FontHandle heroBodyFont = openFont(18);
    colony::sdl::FontHandle patchTitleFont = openFont(18);
    colony::sdl::FontHandle patchBodyFont = openFont(16);
    colony::sdl::FontHandle buttonFont = openFont(24);
    colony::sdl::FontHandle statusFont = openFont(16);

    if (!brandFont || !navFont || !channelFont || !tileTitleFont || !tileMetaFont || !heroTitleFont || !heroSubtitleFont
        || !heroBodyFont || !patchTitleFont || !patchBodyFont || !buttonFont || !statusFont)
    {
        std::cerr << "Failed to load required fonts from " << fontPath << ": " << TTF_GetError() << '\n';
        SDL_Quit();
        return EXIT_FAILURE;
    }

    colony::AppContent content;
    try
    {
        content = colony::LoadContentFromFile(ResolveContentPath().string());
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << '\n';
        SDL_Quit();
        return EXIT_FAILURE;
    }

    if (content.channels.empty())
    {
        std::cerr << "No channels defined in content file." << '\n';
        SDL_Quit();
        return EXIT_FAILURE;
    }

    colony::ui::ThemeManager themeManager;
    colony::ui::ThemeColors theme = themeManager.ActiveScheme().colors;

    std::unordered_map<std::string, colony::ui::ProgramVisuals> programVisuals;
    programVisuals.reserve(content.views.size());

    colony::ui::NavigationChrome navigationChrome;
    colony::ui::LibraryChrome libraryChrome;
    colony::ui::HeroChrome heroChrome;
    colony::ui::SettingsPanel settingsPanel;

    auto rebuildProgramVisuals = [&]() {
        programVisuals.clear();
        const SDL_Color heroSubtitleColor = colony::color::Mix(theme.heroBody, theme.heroTitle, 0.35f);
        for (const auto& [id, view] : content.views)
        {
            programVisuals.emplace(
                id,
                colony::ui::BuildProgramVisuals(
                    view,
                    renderer.get(),
                    heroTitleFont.get(),
                    heroSubtitleFont.get(),
                    heroBodyFont.get(),
                    buttonFont.get(),
                    tileTitleFont.get(),
                    tileMetaFont.get(),
                    tileMetaFont.get(),
                    patchTitleFont.get(),
                    patchBodyFont.get(),
                    statusFont.get(),
                    theme.heroTitle,
                    theme.heroBody,
                    heroSubtitleColor,
                    theme.muted,
                    theme.statusBarText,
                    theme.heroGradientFallbackStart,
                    theme.heroGradientFallbackEnd));
        }
    };

    auto rebuildChrome = [&]() {
        theme = themeManager.ActiveScheme().colors;
        navigationChrome = colony::ui::BuildNavigationChrome(
            renderer.get(),
            brandFont.get(),
            navFont.get(),
            tileMetaFont.get(),
            content,
            theme);
        libraryChrome = colony::ui::BuildLibraryChrome(renderer.get(), tileMetaFont.get(), theme);
        heroChrome = colony::ui::BuildHeroChrome(renderer.get(), tileMetaFont.get(), theme);
        settingsPanel.Build(renderer.get(), heroTitleFont.get(), heroBodyFont.get(), theme.heroTitle, theme.heroBody, themeManager);
        rebuildProgramVisuals();
    };

    rebuildChrome();

    std::vector<int> channelSelections(content.channels.size(), 0);
    auto clampSelection = [&](int channelIndex) {
        const auto& channel = content.channels[channelIndex];
        if (channel.programs.empty())
        {
            channelSelections[channelIndex] = 0;
            return;
        }
        channelSelections[channelIndex] = std::clamp(channelSelections[channelIndex], 0, static_cast<int>(channel.programs.size()) - 1);
    };
    for (std::size_t index = 0; index < content.channels.size(); ++index)
    {
        clampSelection(static_cast<int>(index));
    }

    int activeChannelIndex = 0;
    auto getActiveProgramId = [&]() -> std::string {
        const auto& channel = content.channels[activeChannelIndex];
        if (channel.programs.empty())
        {
            return {};
        }
        clampSelection(activeChannelIndex);
        return channel.programs[channelSelections[activeChannelIndex]];
    };

    auto activeProgramId = getActiveProgramId();

    std::vector<SDL_Rect> channelButtonRects(content.channels.size());
    std::vector<SDL_Rect> programTileRects;
    colony::ui::SettingsPanel::RenderResult settingsRenderResult{};
    std::optional<SDL_Rect> actionButtonRect;

    bool running = true;
    SDL_Event event;

    auto activateProgram = [&](const std::string& programId) {
        if (programVisuals.find(programId) == programVisuals.end() && programId != kSettingsProgramId)
        {
            return;
        }
        activeProgramId = programId;
    };

    auto activateChannel = [&](int index) {
        if (index < 0 || index >= static_cast<int>(content.channels.size()))
        {
            return;
        }
        activeChannelIndex = index;
        clampSelection(activeChannelIndex);
        activateProgram(getActiveProgramId());
    };

    auto activateProgramInChannel = [&](int programIndex) {
        clampSelection(activeChannelIndex);
        const auto& channel = content.channels[activeChannelIndex];
        if (channel.programs.empty())
        {
            return;
        }
        const int clamped = std::clamp(programIndex, 0, static_cast<int>(channel.programs.size()) - 1);
        channelSelections[activeChannelIndex] = clamped;
        activateProgram(channel.programs[clamped]);
    };

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                const int mouseX = event.button.x;
                const int mouseY = event.button.y;

                for (std::size_t i = 0; i < channelButtonRects.size(); ++i)
                {
                    if (PointInRect(channelButtonRects[i], mouseX, mouseY))
                    {
                        activateChannel(static_cast<int>(i));
                        break;
                    }
                }

                for (std::size_t i = 0; i < programTileRects.size(); ++i)
                {
                    if (PointInRect(programTileRects[i], mouseX, mouseY))
                    {
                        activateProgramInChannel(static_cast<int>(i));
                        break;
                    }
                }

                if (activeProgramId == kSettingsProgramId)
                {
                    for (const auto& [schemeId, rect] : settingsRenderResult.optionRects)
                    {
                        if (PointInRect(rect, mouseX, mouseY))
                        {
                            if (themeManager.SetActiveScheme(schemeId))
                            {
                                rebuildChrome();
                            }
                            break;
                        }
                    }
                }
                else if (actionButtonRect.has_value() && PointInRect(*actionButtonRect, mouseX, mouseY))
                {
                    // Placeholder for future action handling.
                }
            }
            else if (event.type == SDL_KEYDOWN)
            {
                const SDL_Keycode key = event.key.keysym.sym;
                switch (key)
                {
                case SDLK_UP:
                    activateProgramInChannel(channelSelections[activeChannelIndex] - 1);
                    break;
                case SDLK_DOWN:
                    activateProgramInChannel(channelSelections[activeChannelIndex] + 1);
                    break;
                case SDLK_LEFT:
                    activateChannel(activeChannelIndex - 1);
                    break;
                case SDLK_RIGHT:
                    activateChannel(activeChannelIndex + 1);
                    break;
                default:
                    break;
                }
            }
        }

        int outputWidth = 0;
        int outputHeight = 0;
        SDL_GetRendererOutputSize(renderer.get(), &outputWidth, &outputHeight);

        SDL_SetRenderDrawColor(renderer.get(), theme.background.r, theme.background.g, theme.background.b, theme.background.a);
        SDL_RenderClear(renderer.get());

        const int navRailWidth = 96;
        const int libraryWidth = std::clamp(outputWidth / 4, 320, 360);
        SDL_Rect navRailRect{0, 0, navRailWidth, outputHeight};
        SDL_SetRenderDrawColor(renderer.get(), theme.navRail.r, theme.navRail.g, theme.navRail.b, theme.navRail.a);
        SDL_RenderFillRect(renderer.get(), &navRailRect);

        SDL_Rect libraryRect{navRailWidth, 0, libraryWidth, outputHeight};
        SDL_SetRenderDrawColor(renderer.get(), theme.libraryBackground.r, theme.libraryBackground.g, theme.libraryBackground.b, theme.libraryBackground.a);
        SDL_RenderFillRect(renderer.get(), &libraryRect);

        const SDL_Rect heroRect{navRailWidth + libraryWidth, 0, outputWidth - navRailWidth - libraryWidth, outputHeight};
        const auto visualsIt = programVisuals.find(activeProgramId);
        const colony::ui::ProgramVisuals* activeVisuals = visualsIt != programVisuals.end() ? &visualsIt->second : nullptr;
        if (activeVisuals != nullptr)
        {
            colony::color::RenderVerticalGradient(renderer.get(), heroRect, activeVisuals->gradientStart, activeVisuals->gradientEnd);
        }
        else
        {
            colony::color::RenderVerticalGradient(renderer.get(), heroRect, theme.heroGradientFallbackStart, theme.heroGradientFallbackEnd);
        }

        SDL_Rect navDivider{navRailRect.x + navRailRect.w - 2, 0, 2, outputHeight};
        SDL_SetRenderDrawColor(renderer.get(), theme.border.r, theme.border.g, theme.border.b, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer.get(), &navDivider);

        channelButtonRects = colony::ui::RenderNavigationRail(
            renderer.get(),
            theme,
            navRailRect,
            kStatusBarHeight,
            navigationChrome,
            content,
            channelSelections,
            activeChannelIndex,
            programVisuals);

        const auto libraryResult = colony::ui::RenderLibraryPanel(
            renderer.get(),
            theme,
            libraryRect,
            libraryChrome,
            content,
            activeChannelIndex,
            channelSelections,
            programVisuals,
            channelFont.get());
        programTileRects = libraryResult.tileRects;

        actionButtonRect.reset();
        settingsRenderResult.optionRects.clear();
        settingsRenderResult.contentHeight = 0;

        if (activeProgramId == kSettingsProgramId)
        {
            colony::ui::RenderSettingsPanel(renderer.get(), theme, heroRect, settingsPanel, themeManager.ActiveScheme().id, settingsRenderResult);
        }
        else if (activeVisuals != nullptr)
        {
            const auto heroResult = colony::ui::RenderHeroPanel(
                renderer.get(),
                theme,
                heroRect,
                *visualsIt,
                heroChrome,
                heroBodyFont.get(),
                patchTitleFont.get(),
                patchBodyFont.get());
            actionButtonRect = heroResult.actionButtonRect;
        }

        colony::ui::RenderStatusBar(renderer.get(), theme, heroRect, kStatusBarHeight, activeVisuals);

        SDL_RenderPresent(renderer.get());
    }

    SDL_Quit();
    return EXIT_SUCCESS;
}
