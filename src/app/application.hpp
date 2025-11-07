#pragma once

#include "controllers/navigation_controller.hpp"
#include "core/content.hpp"
#include "core/localization_manager.hpp"
#include "core/program_catalog.hpp"
#include "programs/archive/ArchiveApp.hpp"
#include "programs/launch_contract.hpp"
#include "ui/hero_panel.hpp"
#include "ui/library_panel.hpp"
#include "ui/navigation.hpp"
#include "ui/program_visuals.hpp"
#include "ui/settings_panel.hpp"
#include "ui/theme.hpp"
#include "utils/sdl_wrappers.hpp"
#include "utils/preferences.hpp"
#include "views/view_factory.hpp"
#include "views/view_registry.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <functional>
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
    [[nodiscard]] bool InitializeLocalization();
    void InitializeNavigation();
    void InitializeViews();
    void RebuildTheme();
    void RebuildProgramVisuals();
    void RebuildProgramLaunchers();
    programs::LaunchContext BuildLaunchContext();
    void ActivateChannel(int index);
    void ActivateProgram(const std::string& programId);
    void ActivateProgramInChannel(int programIndex);
    [[nodiscard]] std::string GetActiveProgramId() const;
    void HandleEvent(const SDL_Event& event, bool& running);
    void HandleMouseClick(int x, int y);
    void HandleMouseWheel(const SDL_MouseWheelEvent& wheel);
    void HandleKeyDown(SDL_Keycode key);
    void RenderFrame(double deltaSeconds);
    void UpdateStatusMessage(const std::string& statusText);
    void UpdateViewContextAccent();
    void ChangeLanguage(const std::string& languageId);
    void HandlePrimaryActionLaunch();
    void LoadPreferences();
    void ApplyPreferences();
    void SavePreferences();
    void MarkPreferencesDirty();
    void MaybeReloadContent(double deltaSeconds);
    void RefreshCustomProgramRegistrations();
    void HandleAddProgramRequest();
    [[nodiscard]] std::optional<std::filesystem::path> PromptForExecutable() const;
    void LaunchExternalProgram(const std::filesystem::path& path) const;
    colony::ViewContent BuildCustomProgramView(const preferences::CustomProgram& program) const;
    void ActivateCustomProgramChannel(const std::string& programId);

    [[nodiscard]] static std::filesystem::path ResolveContentPath();
    [[nodiscard]] static std::filesystem::path ResolveLocalizationDirectory();
    [[nodiscard]] bool PointInRect(const SDL_Rect& rect, int x, int y) const;
    [[nodiscard]] std::string GetLocalizedString(std::string_view key) const;
    [[nodiscard]] std::string GetLocalizedString(std::string_view key, std::string_view fallback) const;

    sdl::WindowHandle window_;
    sdl::RendererHandle renderer_;
    FontResources fonts_;

    AppContent content_;
    ProgramCatalog programCatalog_{};
    LocalizationManager localizationManager_{};
    ui::ThemeManager themeManager_;
    ui::ThemeColors theme_{};
    ui::ColorScheme::ThemeAnimations themeAnimations_{};

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
    std::optional<SDL_Rect> addProgramButtonRect_;
    ui::SettingsPanel::RenderResult settingsRenderResult_{};
    int settingsScrollOffset_ = 0;
    std::string activeLanguageId_ = "en";
    std::unordered_map<std::string, bool> basicToggleStates_{
        {"notifications", true},
        {"sound", true},
        {"auto_updates", true},
        {"reduced_motion", false},
    };

    NavigationController navigationController_;
    ViewRegistry viewRegistry_;
    ViewFactory viewFactory_;
    RenderContext viewContext_{};
    std::string statusBuffer_;

    double animationTimeSeconds_ = 0.0;
    Uint64 lastFrameCounter_ = 0;

    std::filesystem::path contentRoot_;
    std::filesystem::file_time_type lastContentWriteTime_{};
    double hotReloadAccumulatorSeconds_ = 0.0;
    preferences::Preferences preferences_{};
    std::filesystem::path preferencesPath_{};
    bool preferencesDirty_ = false;

    programs::archive::ArchiveApp archiveApp_{};
    std::unordered_map<std::string, std::function<void()>> programLaunchers_{};

    static constexpr int kWindowWidth = 1600;
    static constexpr int kWindowHeight = 900;
    static constexpr char kSettingsProgramId[] = "SETTINGS";
    static constexpr char kCustomProgramsChannelId[] = "custom_programs";
};

} // namespace colony
