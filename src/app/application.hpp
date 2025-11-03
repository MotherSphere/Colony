#pragma once

#include "controllers/navigation_controller.hpp"
#include "core/content.hpp"
#include "ui/hero_panel.hpp"
#include "ui/library_panel.hpp"
#include "ui/navigation.hpp"
#include "ui/program_visuals.hpp"
#include "ui/settings_panel.hpp"
#include "ui/theme.hpp"
#include "utils/sdl_wrappers.hpp"
#include "views/view_factory.hpp"
#include "views/view_registry.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace colony
{

class Application
{
  public:
    Application();
    int Run();

  private:
    struct FontResources
    {
        sdl::FontHandle brand;
        sdl::FontHandle navigation;
        sdl::FontHandle channel;
        sdl::FontHandle tileTitle;
        sdl::FontHandle tileSubtitle;
        sdl::FontHandle tileMeta;
        sdl::FontHandle heroTitle;
        sdl::FontHandle heroSubtitle;
        sdl::FontHandle heroBody;
        sdl::FontHandle patchTitle;
        sdl::FontHandle patchBody;
        sdl::FontHandle button;
        sdl::FontHandle status;
    };

    [[nodiscard]] bool InitializeSDL();
    [[nodiscard]] bool CreateWindowAndRenderer();
    [[nodiscard]] bool InitializeFonts();
    [[nodiscard]] bool LoadContent();
    void InitializeNavigation();
    void InitializeViews();
    void RebuildTheme();
    void RebuildProgramVisuals();
    void ActivateChannel(int index);
    void ActivateProgram(const std::string& programId);
    void ActivateProgramInChannel(int programIndex);
    [[nodiscard]] std::string GetActiveProgramId() const;
    void HandleEvent(const SDL_Event& event, bool& running);
    void HandleMouseClick(int x, int y);
    void HandleKeyDown(SDL_Keycode key);
    void RenderFrame();
    void UpdateStatusMessage(const std::string& statusText);
    void UpdateViewContextAccent();

    [[nodiscard]] static std::filesystem::path ResolveContentPath();
    [[nodiscard]] bool PointInRect(const SDL_Rect& rect, int x, int y) const;

    sdl::WindowHandle window_;
    sdl::RendererHandle renderer_;
    FontResources fonts_;

    AppContent content_;
    ui::ThemeManager themeManager_;
    ui::ThemeColors theme_{};

    ui::NavigationRail navigationRail_;
    ui::LibraryPanelRenderer libraryPanel_;
    ui::HeroPanelRenderer heroPanel_;
    ui::SettingsPanel settingsPanel_;

    std::unordered_map<std::string, ui::ProgramVisuals> programVisuals_;
    std::vector<int> channelSelections_;
    int activeChannelIndex_ = 0;
    std::string activeProgramId_;

    std::vector<SDL_Rect> channelButtonRects_;
    std::vector<SDL_Rect> programTileRects_;
    std::optional<SDL_Rect> heroActionRect_;
    ui::SettingsPanel::RenderResult settingsRenderResult_{};

    NavigationController navigationController_;
    ViewRegistry viewRegistry_;
    ViewFactory viewFactory_;
    RenderContext viewContext_{};
    std::string statusBuffer_;

    static constexpr int kWindowWidth = 1280;
    static constexpr int kWindowHeight = 768;
    static constexpr int kStatusBarHeight = 52;
    static constexpr char kSettingsProgramId[] = "SETTINGS";
};

} // namespace colony
