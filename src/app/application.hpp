#pragma once

#include "controllers/navigation_controller.hpp"
#include "core/content.hpp"
#include "core/localization_manager.hpp"
#include "frontend/models/library_view_model.hpp"
#include "frontend/utils/debounce.hpp"
#include "ui/hero_panel.hpp"
#include "ui/hub_panel.hpp"
#include "ui/layout.hpp"
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

#include <array>
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
    void ShowHub();
    void EnterMainInterface();

  private:
    enum class InterfaceState
    {
        Hub,
        MainInterface
    };

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
    void RebuildInteractionPalette();
    void ActivateChannel(int index);
    void ActivateProgram(const std::string& programId);
    void ActivateProgramInChannel(int programIndex);
    void SyncNavigationEntries();
    int EnsureLocalAppsChannel();
    [[nodiscard]] std::string GetActiveProgramId() const;
    void HandleEvent(const SDL_Event& event, bool& running);
    void HandleMouseClick(int x, int y);
    void HandleMouseButtonUp(int x, int y);
    void HandleMouseMotion(const SDL_MouseMotionEvent& motion);
    void HandleMouseWheel(const SDL_MouseWheelEvent& wheel);
    void HandleKeyDown(SDL_Keycode key);
    void HandleMouseRightClick(int x, int y);
    bool HandleTextInput(const SDL_TextInputEvent& event);
    bool UpdateCustomizationValueFromPosition(const std::string& id, int mouseX);
    void RenderFrame(double deltaSeconds);
    void RenderHubFrame(double deltaSeconds);
    void RenderMainInterfaceFrame(double deltaSeconds);
    void UpdateStatusMessage(const std::string& statusText);
    void UpdateViewContextAccent();
    void ChangeLanguage(const std::string& languageId);
    void LaunchArcadeApp();
    void LoadSettings();
    void SaveSettings() const;
    bool SetAppearanceCustomizationValue(const std::string& id, float value);
    [[nodiscard]] float GetAppearanceCustomizationValue(std::string_view id) const;
    void ApplyInterfaceDensity() const;
    void ApplyAppearanceCustomizations();
    void QueueLibraryFilterUpdate();
    void BuildHubPanel();
    void HandleHubMouseClick(int x, int y);
    void HandleHubMouseMotion(const SDL_MouseMotionEvent& motion);
    void HandleHubMouseWheel(const SDL_MouseWheelEvent& wheel);
    bool HandleHubKeyDown(SDL_Keycode key);
    void ActivateHubBranch(const std::string& branchId);
    void ActivateHubBranchByIndex(int index);
    [[nodiscard]] int FindHubBranchIndexById(const std::string& branchId) const;
    void ResetHubInteractionState();
    [[nodiscard]] std::string NormalizeHubSearchString(std::string_view value) const;
    [[nodiscard]] std::vector<std::string> TokenizeHubSearch(std::string_view value) const;
    void EnsureHubScrollWithinBounds();
    void FocusHubSearch();
    void ClearHubSearchQuery();
    void SyncFocusedHubBranch();

    void UpdateTopBarTitle();
    [[nodiscard]] std::string ResolveTopBarTitle() const;

    [[nodiscard]] static std::filesystem::path ResolveContentPath();
    [[nodiscard]] static std::filesystem::path ResolveLocalizationDirectory();
    [[nodiscard]] std::filesystem::path ResolveSettingsPath() const;
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
    void ShowCustomThemeDialog();
    void HideCustomThemeDialog();
    void RenderCustomThemeDialog(double timeSeconds);
    bool HandleCustomThemeDialogMouseClick(int x, int y);
    bool HandleCustomThemeDialogMouseWheel(const SDL_MouseWheelEvent& wheel);
    bool HandleCustomThemeDialogKey(SDL_Keycode key);
    bool HandleCustomThemeDialogText(const SDL_TextInputEvent& text);
    bool ApplyCustomThemeDialog();
    void EnsureCustomThemeFieldVisible(int focusIndex);
    void UpdateTextInputState();
    bool AddUserApplication(const std::filesystem::path& executablePath);
    void LaunchUserApp(const std::filesystem::path& executablePath, const std::string& programId);
    static std::string ColorToHex(SDL_Color color);
    static std::string MakeDisplayNameFromPath(const std::filesystem::path& path);
    static bool IsValidHexColor(const std::string& value);
    static std::string TrimString(std::string value);

    void UpdateLayoutForOutputWidth(int outputWidth);
    void BeginResizeDrag(int x, int y, bool adjustNavRail);
    void EndResizeDrag();
    void UpdateResizeDrag(int x);

    static constexpr std::string_view kLocalAppsChannelId = "local_apps";
    static constexpr std::string_view kLocalAppsChannelLabel = "Local Apps";

    sdl::WindowHandle window_;
    sdl::RendererHandle renderer_;
    FontResources fonts_;
    std::unordered_map<std::string, sdl::FontHandle> languageFonts_;

    AppContent content_;
    LocalizationManager localizationManager_{};
    ui::ThemeManager themeManager_;
    ui::ThemeColors theme_{};
    ui::Typography typography_{};
    ui::InteractionColors interactions_{};
    ui::MotionTimings motion_{};

    ui::NavigationRail navigationRail_;
    ui::TopBar topBar_;
    ui::LibraryPanelRenderer libraryPanel_;
    ui::HeroPanelRenderer heroPanel_;
    ui::HubPanelRenderer hubPanel_;
    ui::SettingsPanel settingsPanel_;

    std::unordered_map<std::string, ui::ProgramVisuals> programVisuals_;
    frontend::models::LibraryViewModel libraryViewModel_{};
    std::vector<int> channelSelections_;
    int activeChannelIndex_ = 0;
    std::string activeProgramId_;
    InterfaceState interfaceState_ = InterfaceState::Hub;

    int navRailWidth_ = 0;
    int libraryWidth_ = 0;
    bool layoutSizesInitialized_ = false;
    SDL_Rect navRailRect_{0, 0, 0, 0};
    SDL_Rect libraryRect_{0, 0, 0, 0};
    SDL_Rect heroRect_{0, 0, 0, 0};
    SDL_Rect navResizeHandleRect_{0, 0, 0, 0};
    SDL_Rect libraryResizeHandleRect_{0, 0, 0, 0};

    struct ResizeState
    {
        enum class Target
        {
            None,
            NavRail,
            Library
        } target = Target::None;
        int startX = 0;
        int initialNavWidth = 0;
        int initialLibraryWidth = 0;
    } resizeState_{};

    std::vector<SDL_Rect> channelButtonRects_;
    std::vector<SDL_Rect> programTileRects_;
    std::optional<SDL_Rect> addAppButtonRect_;
    std::optional<SDL_Rect> libraryFilterInputRect_;
    std::vector<ui::LibraryRenderResult::SortChipHitbox> librarySortChipHitboxes_;
    std::optional<SDL_Rect> heroActionRect_;
    std::optional<SDL_Rect> hubButtonRect_;
    std::vector<ui::HubRenderResult::BranchHitbox> hubBranchHitboxes_;
    int hoveredHubBranchIndex_ = -1;
    int focusedHubBranchIndex_ = -1;
    std::vector<std::string> hubRenderedBranchIds_;
    std::vector<std::string> hubSearchTokens_;
    int hubScrollOffset_ = 0;
    int hubScrollMaxOffset_ = 0;
    SDL_Rect hubScrollViewport_{0, 0, 0, 0};
    bool hubScrollViewportValid_ = false;
    bool hubSearchFocused_ = false;
    std::string hubSearchQuery_;
    bool isHubHeroCollapsed_ = false;
    int hubWidgetPage_ = 0;
    int hubWidgetsPerPage_ = 2;
    int hubWidgetPageCount_ = 0;
    std::optional<SDL_Rect> hubSearchInputRect_;
    std::optional<SDL_Rect> hubSearchClearRect_;
    std::optional<SDL_Rect> hubHeroToggleRect_;
    std::optional<SDL_Rect> hubDetailActionRect_;
    std::vector<ui::HubRenderResult::WidgetPagerHitbox> hubWidgetPagerHitboxes_;
    ui::SettingsPanel::RenderResult settingsRenderResult_{};
    int settingsScrollOffset_ = 0;
    ui::SettingsPanel::SectionStates settingsSectionStates_{};
    std::optional<std::string> pendingSettingsSectionId_{};
    std::optional<std::string> activeCustomizationDragId_{};
    std::string activeLanguageId_ = "en";
    std::unordered_map<std::string, bool> basicToggleStates_{
        {"notifications", true},
        {"sound", true},
        {"auto_updates", true},
        {"reduced_motion", false},
    };
    std::unordered_map<std::string, float> appearanceCustomizationValues_{
        {"accent_intensity", 0.5f},
        {"background_depth", 0.5f},
        {"interface_density", 0.5f},
    };

    std::string libraryFilterDraft_;
    bool libraryFilterFocused_ = false;
    frontend::utils::Debouncer libraryFilterDebouncer_{0.2};

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
        SDL_Rect sortButtonRect{0, 0, 0, 0};
        SDL_Rect filterButtonRect{0, 0, 0, 0};
        SDL_Rect filterDropdownRect{0, 0, 0, 0};
        bool parentAvailable = false;
        SDL_Rect searchBoxRect{0, 0, 0, 0};
        std::string searchQuery;
        bool searchFocused = true;
        int selectedIndex = -1;
        int scrollOffset = 0;
        int contentHeight = 0;
        std::string errorMessage;
        int sortModeIndex = 0;
        int fileTypeFilterIndex = 0;
        bool filterDropdownOpen = false;
        bool filterDropdownVisible = false;
        int filterDropdownOptionHeight = 0;
        int filterDropdownOptionCount = 0;
        std::vector<SDL_Rect> filterDropdownOptionRects;
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

    struct CustomThemeDialogState
    {
        static constexpr int kColorFieldCount = 16;

        bool visible = false;
        std::string nameInput;
        std::array<std::string, kColorFieldCount> colorInputs{};
        int focusedIndex = -1; // 0 = name, 1..kColorFieldCount = color fields
        std::string errorMessage;
        SDL_Rect panelRect{0, 0, 0, 0};
        SDL_Rect nameFieldRect{0, 0, 0, 0};
        std::array<SDL_Rect, kColorFieldCount> colorFieldRects{};
        std::array<int, kColorFieldCount> colorFieldContentOffsets{};
        SDL_Rect colorFieldViewport{0, 0, 0, 0};
        int scrollOffset = 0;
        int colorFieldContentHeight = 0;
        SDL_Rect saveButtonRect{0, 0, 0, 0};
        SDL_Rect cancelButtonRect{0, 0, 0, 0};
    } customThemeDialog_{};

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
    static constexpr std::string_view kSettingsAppearanceProgramId = "SETTINGS_APPEARANCE";
    static constexpr std::string_view kSettingsLanguageProgramId = "SETTINGS_LANGUAGE";
    static constexpr std::string_view kSettingsGeneralProgramId = "SETTINGS_GENERAL";

    [[nodiscard]] static bool IsSettingsProgramId(std::string_view programId);
    [[nodiscard]] static std::string_view SettingsSectionForProgram(std::string_view programId);
    static constexpr char kOrbitalArcadeProgramId[] = "ORBITAL_ARCADE";

};

} // namespace colony
