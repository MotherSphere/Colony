#pragma once

#include "controllers/navigation_controller.hpp"
#include "core/content.hpp"
#include "core/localization_manager.hpp"
#include "ui/hero_panel.hpp"
#include "ui/library_panel.hpp"
#include "ui/navigation.hpp"
#include "ui/program_visuals.hpp"
#include "ui/settings_panel.hpp"
#include "ui/theme.hpp"
#include "utils/sdl_wrappers.hpp"
#include "utils/text.hpp"
#include "views/view_factory.hpp"
#include "views/view_registry.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
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
    void ActivateChannel(int index);
    void ActivateProgram(const std::string& programId);
    void ActivateProgramInChannel(int programIndex);
    [[nodiscard]] std::string GetActiveProgramId() const;
    void HandleEvent(const SDL_Event& event, bool& running);
    void HandleMouseClick(int x, int y);
    void HandleMouseWheel(const SDL_MouseWheelEvent& wheel);
    void HandleKeyDown(SDL_Keycode key);
    void HandleMouseRightClick(int x, int y);
    bool HandleTextInput(const SDL_TextInputEvent& event);
    void RenderFrame(double deltaSeconds);
    void UpdateStatusMessage(const std::string& statusText);
    void UpdateViewContextAccent();
    void ChangeLanguage(const std::string& languageId);
    void LaunchArcadeApp();

    [[nodiscard]] static std::filesystem::path ResolveContentPath();
    [[nodiscard]] static std::filesystem::path ResolveLocalizationDirectory();
    [[nodiscard]] bool PointInRect(const SDL_Rect& rect, int x, int y) const;
    [[nodiscard]] std::string GetLocalizedString(std::string_view key) const;
    [[nodiscard]] std::string GetLocalizedString(std::string_view key, std::string_view fallback) const;
    void ShowAddAppDialog();
    void HideAddAppDialog();
    void RefreshAddAppDialogEntries();
    void RenderAddAppDialog(double timeSeconds);
    bool HandleAddAppDialogMouseClick(int x, int y);
    bool HandleAddAppDialogMouseWheel(const SDL_MouseWheelEvent& wheel);
    bool HandleAddAppDialogKey(SDL_Keycode key);
    void RenderEditUserAppDialog(double timeSeconds);
    bool HandleEditUserAppDialogMouseClick(int x, int y);
    bool HandleEditUserAppDialogKey(SDL_Keycode key);
    bool HandleEditUserAppDialogText(const SDL_TextInputEvent& text);
    void ShowEditUserAppDialog(const std::string& programId);
    void HideEditUserAppDialog();
    bool ApplyEditUserAppChanges();
    void UpdateTextInputState();
    bool AddUserApplication(const std::filesystem::path& executablePath);
    void LaunchUserApp(const std::filesystem::path& executablePath, const std::string& programId);
    static std::string ColorToHex(SDL_Color color);
    static std::string MakeDisplayNameFromPath(const std::filesystem::path& path);
    static bool IsValidHexColor(const std::string& value);
    static std::string TrimString(std::string value);

    sdl::WindowHandle window_;
    sdl::RendererHandle renderer_;
    FontResources fonts_;

    AppContent content_;
    LocalizationManager localizationManager_{};
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
    std::optional<SDL_Rect> addAppButtonRect_;
    std::optional<SDL_Rect> heroActionRect_;
    ui::SettingsPanel::RenderResult settingsRenderResult_{};
    int settingsScrollOffset_ = 0;
    std::string activeLanguageId_ = "en";
    std::unordered_map<std::string, bool> basicToggleStates_{
        {"notifications", true},
        {"sound", true},
        {"auto_updates", true},
        {"reduced_motion", false},
    };

    struct AddAppDialogState
    {
        struct Entry
        {
            std::filesystem::path path;
            bool isDirectory = false;
            colony::TextTexture label;
        };

        bool visible = false;
        std::filesystem::path currentDirectory;
        std::vector<Entry> entries;
        std::vector<SDL_Rect> entryRects;
        SDL_Rect panelRect{0, 0, 0, 0};
        SDL_Rect listViewport{0, 0, 0, 0};
        SDL_Rect confirmButtonRect{0, 0, 0, 0};
        SDL_Rect cancelButtonRect{0, 0, 0, 0};
        SDL_Rect parentButtonRect{0, 0, 0, 0};
        bool parentAvailable = false;
        SDL_Rect searchBoxRect{0, 0, 0, 0};
        std::string searchQuery;
        bool searchFocused = true;
        int selectedIndex = -1;
        int scrollOffset = 0;
        int contentHeight = 0;
        std::string errorMessage;
    } addAppDialog_{};

    struct EditAppDialogState
    {
        bool visible = false;
        std::string programId;
        std::string nameInput;
        std::string colorInput;
        std::string errorMessage;
        SDL_Rect panelRect{0, 0, 0, 0};
        SDL_Rect nameFieldRect{0, 0, 0, 0};
        SDL_Rect colorFieldRect{0, 0, 0, 0};
        SDL_Rect saveButtonRect{0, 0, 0, 0};
        SDL_Rect cancelButtonRect{0, 0, 0, 0};
        bool nameFocused = false;
        bool colorFocused = false;
    } editAppDialog_{};

    NavigationController navigationController_;
    ViewRegistry viewRegistry_;
    ViewFactory viewFactory_;
    RenderContext viewContext_{};
    std::string statusBuffer_;

    double animationTimeSeconds_ = 0.0;
    Uint64 lastFrameCounter_ = 0;

    std::vector<std::string> programTileProgramIds_;
    bool textInputActive_ = false;

    std::unordered_map<std::string, std::filesystem::path> userAppExecutables_;
    int nextCustomProgramId_ = 1;

    static constexpr int kWindowWidth = 1600;
    static constexpr int kWindowHeight = 900;
    static constexpr int kStatusBarHeight = 52;
    static constexpr char kSettingsProgramId[] = "SETTINGS";
    static constexpr char kOrbitalArcadeProgramId[] = "ORBITAL_ARCADE";

};

} // namespace colony
