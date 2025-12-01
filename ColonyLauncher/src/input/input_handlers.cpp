#include "input/input_handlers.hpp"

#include "app/application.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace
{
void RemoveLastUtf8CodepointInPlace(std::string& value)
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
    const int knobSize = colony::ui::Scale(28);
    const int knobTravel = std::max(1, rect.w - knobSize);
    const int relative = std::clamp(mouseX - rect.x - knobSize / 2, 0, knobTravel);
    if (knobTravel <= 0)
    {
        return 0.0f;
    }

    return static_cast<float>(relative) / static_cast<float>(knobTravel);
}
} // namespace

namespace colony::input
{

HubInputHandler::HubInputHandler(Application& app) : app_(app) {}

void HubInputHandler::Register(InputRouter& router)
{
    router.RegisterHandler(SDL_MOUSEBUTTONDOWN, [this](const SDL_Event& event, bool& running) {
        return HandleMouseButtonDown(event, running);
    });
    router.RegisterHandler(SDL_MOUSEMOTION, [this](const SDL_Event& event, bool& running) {
        return HandleMouseMotion(event, running);
    });
    router.RegisterHandler(SDL_MOUSEWHEEL, [this](const SDL_Event& event, bool& running) {
        return HandleMouseWheel(event, running);
    });
    router.RegisterHandler(SDL_KEYDOWN, [this](const SDL_Event& event, bool& running) {
        return HandleKeyDown(event, running);
    });
    router.RegisterHandler(SDL_TEXTINPUT, [this](const SDL_Event& event, bool& running) {
        return HandleTextInput(event, running);
    });
}

bool HubInputHandler::HandleMouseButtonDown(const SDL_Event& event, bool& running)
{
    (void)running;
    if (app_.interfaceState_ != Application::InterfaceState::Hub)
    {
        return false;
    }

    if (event.button.button == SDL_BUTTON_LEFT)
    {
        app_.HandleHubMouseClick(event.button.x, event.button.y);
    }

    return true;
}

bool HubInputHandler::HandleMouseMotion(const SDL_Event& event, bool& running)
{
    (void)running;
    if (app_.interfaceState_ != Application::InterfaceState::Hub)
    {
        return false;
    }

    app_.HandleHubMouseMotion(event.motion);
    return true;
}

bool HubInputHandler::HandleMouseWheel(const SDL_Event& event, bool& running)
{
    (void)running;
    if (app_.interfaceState_ != Application::InterfaceState::Hub)
    {
        return false;
    }

    app_.HandleHubMouseWheel(event.wheel);
    return true;
}

bool HubInputHandler::HandleKeyDown(const SDL_Event& event, bool& running)
{
    (void)running;
    if (app_.interfaceState_ != Application::InterfaceState::Hub)
    {
        return false;
    }

    app_.HandleHubKeyDown(event.key.keysym.sym);
    return true;
}

bool HubInputHandler::HandleTextInput(const SDL_Event& event, bool& running)
{
    (void)running;
    if (app_.interfaceState_ != Application::InterfaceState::Hub)
    {
        return false;
    }

    if (app_.hubSearchFocused_)
    {
        constexpr std::size_t kMaxSearchLength = 120;
        const std::size_t currentLength = app_.hubSearchQuery_.size();
        const std::size_t incomingLength = std::strlen(event.text.text);
        if (incomingLength > 0 && currentLength < kMaxSearchLength)
        {
            app_.hubSearchQuery_.append(event.text.text, std::min(incomingLength, kMaxSearchLength - currentLength));
            app_.hubScrollOffset_ = 0;
            app_.BuildHubPanel();
        }
        return true;
    }

    return false;
}

DialogInputHandler::DialogInputHandler(Application& app) : app_(app) {}

void DialogInputHandler::Register(InputRouter& router)
{
    router.RegisterHandler(SDL_MOUSEBUTTONDOWN, [this](const SDL_Event& event, bool& running) {
        return HandleMouseButtonDown(event, running);
    });
    router.RegisterHandler(SDL_MOUSEWHEEL, [this](const SDL_Event& event, bool& running) {
        return HandleMouseWheel(event, running);
    });
    router.RegisterHandler(SDL_KEYDOWN, [this](const SDL_Event& event, bool& running) {
        return HandleKeyDown(event, running);
    });
    router.RegisterHandler(SDL_TEXTINPUT, [this](const SDL_Event& event, bool& running) {
        return HandleTextInput(event, running);
    });
}

bool DialogInputHandler::HandleMouseButtonDown(const SDL_Event& event, bool& running)
{
    (void)running;
    const bool rightClick = event.button.button == SDL_BUTTON_RIGHT;
    const bool leftClick = event.button.button == SDL_BUTTON_LEFT;
    if (!rightClick && !leftClick)
    {
        return false;
    }

    bool anyDialogVisible = app_.customThemeDialog_.visible || app_.editAppDialog_.visible || app_.addAppDialog_.visible;
    bool handled = false;
    if (app_.customThemeDialog_.visible)
    {
        handled = app_.HandleCustomThemeDialogMouseClick(event.button.x, event.button.y) || handled;
    }

    if (app_.editAppDialog_.visible)
    {
        handled = app_.HandleEditUserAppDialogMouseClick(event.button.x, event.button.y) || handled;
    }

    if (app_.addAppDialog_.visible)
    {
        handled = app_.HandleAddAppDialogMouseClick(event.button.x, event.button.y) || handled;
    }

    return handled || anyDialogVisible;
}

bool DialogInputHandler::HandleMouseWheel(const SDL_Event& event, bool& running)
{
    (void)running;
    if (app_.interfaceState_ == Application::InterfaceState::Hub)
    {
        return false;
    }

    if (app_.HandleCustomThemeDialogMouseWheel(event.wheel))
    {
        return true;
    }

    if (app_.editAppDialog_.visible)
    {
        return true;
    }

    if (app_.addAppDialog_.visible && app_.HandleAddAppDialogMouseWheel(event.wheel))
    {
        return true;
    }

    return false;
}

bool DialogInputHandler::HandleKeyDown(const SDL_Event& event, bool& running)
{
    (void)running;
    const SDL_Keycode key = event.key.keysym.sym;
    if (app_.customThemeDialog_.visible && app_.HandleCustomThemeDialogKey(key))
    {
        return true;
    }

    if (app_.editAppDialog_.visible && app_.HandleEditUserAppDialogKey(key))
    {
        return true;
    }

    if (app_.addAppDialog_.visible && app_.HandleAddAppDialogKey(key))
    {
        return true;
    }

    return false;
}

bool DialogInputHandler::HandleTextInput(const SDL_Event& event, bool& running)
{
    (void)running;
    if (app_.customThemeDialog_.visible && app_.HandleCustomThemeDialogText(event.text))
    {
        return true;
    }

    if (app_.editAppDialog_.visible && app_.HandleEditUserAppDialogText(event.text))
    {
        return true;
    }

    if (app_.addAppDialog_.visible && app_.addAppDialog_.searchFocused)
    {
        constexpr std::size_t kMaxSearchLength = 120;
        const std::size_t currentLength = app_.addAppDialog_.searchQuery.size();
        const std::size_t incomingLength = std::strlen(event.text.text);
        if (currentLength < kMaxSearchLength && incomingLength > 0)
        {
            app_.addAppDialog_.searchQuery.append(event.text.text, std::min(incomingLength, kMaxSearchLength - currentLength));
            app_.addAppDialog_.scrollOffset = 0;
            app_.RefreshAddAppDialogEntries();
        }
        return true;
    }

    return false;
}

NavigationInputHandler::NavigationInputHandler(Application& app) : app_(app) {}

void NavigationInputHandler::Register(InputRouter& router)
{
    router.RegisterHandler(SDL_QUIT, [this](const SDL_Event& event, bool& running) {
        return HandleQuit(event, running);
    });
    router.RegisterHandler(SDL_MOUSEBUTTONDOWN, [this](const SDL_Event& event, bool& running) {
        return HandleMouseButtonDown(event, running);
    });
    router.RegisterHandler(SDL_MOUSEBUTTONUP, [this](const SDL_Event& event, bool& running) {
        return HandleMouseButtonUp(event, running);
    });
    router.RegisterHandler(SDL_MOUSEMOTION, [this](const SDL_Event& event, bool& running) {
        return HandleMouseMotion(event, running);
    });
    router.RegisterHandler(SDL_KEYDOWN, [this](const SDL_Event& event, bool& running) {
        return HandleKeyDown(event, running);
    });
}

bool NavigationInputHandler::HandleMouseButtonDown(const SDL_Event& event, bool& running)
{
    (void)running;
    if (event.button.button != SDL_BUTTON_LEFT || app_.interfaceState_ == Application::InterfaceState::Hub)
    {
        return false;
    }

    app_.activeCustomizationDragId_.reset();

    if (app_.resizeState_.target != Application::ResizeState::Target::None)
    {
        return true;
    }

    if (!app_.editAppDialog_.visible && !app_.addAppDialog_.visible)
    {
        if (app_.PointInRect(app_.navResizeHandleRect_, event.button.x, event.button.y))
        {
            app_.BeginResizeDrag(event.button.x, event.button.y, true);
            return true;
        }

        if (app_.PointInRect(app_.libraryResizeHandleRect_, event.button.x, event.button.y))
        {
            app_.BeginResizeDrag(event.button.x, event.button.y, false);
            return true;
        }
    }

    if (app_.hubButtonRect_.has_value() && app_.PointInRect(*app_.hubButtonRect_, event.button.x, event.button.y))
    {
        app_.ShowHub();
        return true;
    }

    if (app_.libraryFilterInputRect_.has_value() && app_.PointInRect(*app_.libraryFilterInputRect_, event.button.x, event.button.y))
    {
        if (!app_.libraryFilterFocused_)
        {
            app_.libraryFilterFocused_ = true;
            app_.UpdateTextInputState();
        }
        return true;
    }

    if (app_.libraryFilterFocused_)
    {
        app_.libraryFilterFocused_ = false;
        app_.UpdateTextInputState();
        const double nowSeconds = static_cast<double>(SDL_GetTicks64()) / 1000.0;
        app_.libraryFilterDebouncer_.Flush(nowSeconds);
    }

    for (std::size_t i = 0; i < app_.channelButtonRects_.size(); ++i)
    {
        if (app_.PointInRect(app_.channelButtonRects_[i], event.button.x, event.button.y))
        {
            app_.navigationController_.Activate(static_cast<int>(i));
            return true;
        }
    }

    return false;
}

bool NavigationInputHandler::HandleMouseButtonUp(const SDL_Event& event, bool& running)
{
    (void)running;
    if (event.button.button != SDL_BUTTON_LEFT || app_.interfaceState_ == Application::InterfaceState::Hub)
    {
        return false;
    }

    app_.activeCustomizationDragId_.reset();

    if (app_.resizeState_.target != Application::ResizeState::Target::None)
    {
        app_.EndResizeDrag();
        return true;
    }

    return false;
}

bool NavigationInputHandler::HandleMouseMotion(const SDL_Event& event, bool& running)
{
    (void)running;
    if (app_.interfaceState_ == Application::InterfaceState::Hub)
    {
        return false;
    }

    if (app_.resizeState_.target != Application::ResizeState::Target::None)
    {
        app_.UpdateResizeDrag(event.motion.x);
        return true;
    }

    return false;
}

bool NavigationInputHandler::HandleKeyDown(const SDL_Event& event, bool& running)
{
    (void)running;
    if (app_.interfaceState_ == Application::InterfaceState::Hub)
    {
        return false;
    }

    const SDL_Keycode key = event.key.keysym.sym;

    if (app_.libraryFilterFocused_)
    {
        switch (key)
        {
        case SDLK_BACKSPACE:
            if (!app_.libraryFilterDraft_.empty())
            {
                RemoveLastUtf8CodepointInPlace(app_.libraryFilterDraft_);
                app_.QueueLibraryFilterUpdate();
            }
            return true;
        case SDLK_ESCAPE:
            app_.libraryFilterFocused_ = false;
            app_.UpdateTextInputState();
            app_.libraryFilterDebouncer_.Flush(static_cast<double>(SDL_GetTicks64()) / 1000.0);
            return true;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            app_.libraryFilterDebouncer_.Flush(static_cast<double>(SDL_GetTicks64()) / 1000.0);
            return true;
        default:
            break;
        }
    }

    switch (key)
    {
    case SDLK_UP:
        app_.ActivateProgramInChannel(app_.channelSelections_[app_.activeChannelIndex_] - 1);
        return true;
    case SDLK_DOWN:
        app_.ActivateProgramInChannel(app_.channelSelections_[app_.activeChannelIndex_] + 1);
        return true;
    case SDLK_LEFT:
        app_.navigationController_.Activate(app_.activeChannelIndex_ - 1);
        return true;
    case SDLK_RIGHT:
        app_.navigationController_.Activate(app_.activeChannelIndex_ + 1);
        return true;
    case SDLK_h:
    case SDLK_HOME:
        if (!app_.libraryFilterFocused_)
        {
            app_.ShowHub();
        }
        return true;
    default:
        break;
    }

    return false;
}

bool NavigationInputHandler::HandleQuit(const SDL_Event& event, bool& running)
{
    (void)event;
    running = false;
    return true;
}

LibraryInputHandler::LibraryInputHandler(Application& app) : app_(app) {}

void LibraryInputHandler::Register(InputRouter& router)
{
    router.RegisterHandler(SDL_MOUSEBUTTONDOWN, [this](const SDL_Event& event, bool& running) {
        return HandleMouseButtonDown(event, running);
    });
    router.RegisterHandler(SDL_MOUSEBUTTONUP, [this](const SDL_Event& event, bool& running) {
        return HandleMouseButtonUp(event, running);
    });
    router.RegisterHandler(SDL_MOUSEMOTION, [this](const SDL_Event& event, bool& running) {
        return HandleMouseMotion(event, running);
    });
    router.RegisterHandler(SDL_MOUSEWHEEL, [this](const SDL_Event& event, bool& running) {
        return HandleMouseWheel(event, running);
    });
    router.RegisterHandler(SDL_KEYDOWN, [this](const SDL_Event& event, bool& running) {
        return HandleKeyDown(event, running);
    });
    router.RegisterHandler(SDL_TEXTINPUT, [this](const SDL_Event& event, bool& running) {
        return HandleTextInput(event, running);
    });
}

bool LibraryInputHandler::HandleMouseButtonDown(const SDL_Event& event, bool& running)
{
    (void)running;
    const bool leftClick = event.button.button == SDL_BUTTON_LEFT;
    const bool rightClick = event.button.button == SDL_BUTTON_RIGHT;
    if (!leftClick && !rightClick)
    {
        return false;
    }

    if (app_.interfaceState_ == Application::InterfaceState::Hub)
    {
        return false;
    }

    if (rightClick)
    {
        if (app_.customThemeDialog_.visible)
        {
            app_.HandleCustomThemeDialogMouseClick(event.button.x, event.button.y);
            return true;
        }

        if (app_.editAppDialog_.visible)
        {
            app_.HandleEditUserAppDialogMouseClick(event.button.x, event.button.y);
            return true;
        }

        if (app_.addAppDialog_.visible)
        {
            app_.HandleAddAppDialogMouseClick(event.button.x, event.button.y);
            return true;
        }

        for (std::size_t i = 0; i < app_.programTileRects_.size(); ++i)
        {
            if (!app_.PointInRect(app_.programTileRects_[i], event.button.x, event.button.y))
            {
                continue;
            }

            if (i >= app_.programTileProgramIds_.size())
            {
                return true;
            }

            const std::string& programId = app_.programTileProgramIds_[i];
            if (app_.userAppExecutables_.find(programId) != app_.userAppExecutables_.end())
            {
                app_.ShowEditUserAppDialog(programId);
            }
            return true;
        }

        return false;
    }

    if (app_.addAppDialog_.visible)
    {
        if (app_.HandleAddAppDialogMouseClick(event.button.x, event.button.y))
        {
            return true;
        }
    }

    if (app_.resizeState_.target != Application::ResizeState::Target::None)
    {
        return true;
    }

    if (app_.addAppButtonRect_.has_value() && app_.activeChannelIndex_ >= 0
        && app_.activeChannelIndex_ < static_cast<int>(app_.content_.channels.size()))
    {
        auto toLower = [](std::string value) {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            return value;
        };

        const std::string channelIdLower = toLower(app_.content_.channels[app_.activeChannelIndex_].id);
        const std::string localIdLower = toLower(std::string(Application::kLocalAppsChannelId));
        if (channelIdLower == localIdLower && app_.PointInRect(*app_.addAppButtonRect_, event.button.x, event.button.y))
        {
            app_.ShowAddAppDialog();
            return true;
        }
    }

    for (const auto& chip : app_.librarySortChipHitboxes_)
    {
        if (app_.PointInRect(chip.rect, event.button.x, event.button.y))
        {
            const double nowSeconds = static_cast<double>(SDL_GetTicks64()) / 1000.0;
            app_.libraryFilterDebouncer_.Flush(nowSeconds);
            app_.libraryViewModel_.SetSortOption(chip.option);
            return true;
        }
    }

    for (std::size_t i = 0; i < app_.programTileRects_.size(); ++i)
    {
        if (app_.PointInRect(app_.programTileRects_[i], event.button.x, event.button.y))
        {
            if (i < app_.programTileProgramIds_.size())
            {
                const std::string& programId = app_.programTileProgramIds_[i];
                if (app_.activeChannelIndex_ >= 0 && app_.activeChannelIndex_ < static_cast<int>(app_.content_.channels.size()))
                {
                    const auto& channel = app_.content_.channels[app_.activeChannelIndex_];
                    auto it = std::find(channel.programs.begin(), channel.programs.end(), programId);
                    if (it != channel.programs.end())
                    {
                        const int index = static_cast<int>(std::distance(channel.programs.begin(), it));
                        app_.ActivateProgramInChannel(index);
                        return true;
                    }
                }

                app_.ActivateProgram(programId);
            }
            return true;
        }
    }

    if (Application::IsSettingsProgramId(app_.activeProgramId_))
    {
        for (const auto& region : app_.settingsRenderResult_.interactiveRegions)
        {
            if (!app_.PointInRect(region.rect, event.button.x, event.button.y))
            {
                continue;
            }

            switch (region.type)
            {
            case ui::SettingsPanel::RenderResult::InteractionType::ThemeSelection:
                if (app_.themeManager_.SetActiveScheme(region.id))
                {
                    app_.RebuildTheme();
                }
                break;
            case ui::SettingsPanel::RenderResult::InteractionType::ThemeCreation:
                app_.ShowCustomThemeDialog();
                break;
            case ui::SettingsPanel::RenderResult::InteractionType::LanguageSelection:
                app_.ChangeLanguage(region.id);
                break;
            case ui::SettingsPanel::RenderResult::InteractionType::Toggle:
            {
                auto& toggleStates = app_.settingsService_.ToggleStates();
                if (auto it = toggleStates.find(region.id); it != toggleStates.end())
                {
                    it->second = !it->second;
                }
                break;
            }
            case ui::SettingsPanel::RenderResult::InteractionType::Customization:
            {
                const float newValue = ComputeCustomizationSliderValue(region.rect, event.button.x);
                if (app_.SetAppearanceCustomizationValue(region.id, newValue))
                {
                    app_.RebuildTheme();
                }
                app_.activeCustomizationDragId_ = region.id;
                break;
            }
            case ui::SettingsPanel::RenderResult::InteractionType::SectionToggle:
                if (region.id == ui::SettingsPanel::kAppearanceSectionId)
                {
                    app_.settingsSectionStates_.appearanceExpanded = !app_.settingsSectionStates_.appearanceExpanded;
                }
                else if (region.id == ui::SettingsPanel::kLanguageSectionId)
                {
                    app_.settingsSectionStates_.languageExpanded = !app_.settingsSectionStates_.languageExpanded;
                }
                else if (region.id == ui::SettingsPanel::kGeneralSectionId)
                {
                    app_.settingsSectionStates_.generalExpanded = !app_.settingsSectionStates_.generalExpanded;
                }
                break;
            }
            return true;
        }
    }
    else if (app_.heroActionRect_.has_value() && app_.PointInRect(*app_.heroActionRect_, event.button.x, event.button.y))
    {
        app_.viewRegistry_.TriggerPrimaryAction(app_.statusBuffer_);
        app_.UpdateStatusMessage(app_.statusBuffer_);
        if (app_.activeProgramId_ == Application::kNexusProgramId)
        {
            app_.LaunchNexusApp();
        }
        else if (const auto appIt = app_.userApplications_.find(app_.activeProgramId_);
                 appIt != app_.userApplications_.end())
        {
            app_.LaunchUserApp(appIt->second, app_.activeProgramId_);
        }
        return true;
    }

    return false;
}

bool LibraryInputHandler::HandleMouseButtonUp(const SDL_Event& event, bool& running)
{
    (void)running;
    (void)event;
    return false;
}

bool LibraryInputHandler::HandleMouseMotion(const SDL_Event& event, bool& running)
{
    (void)running;
    if (app_.interfaceState_ == Application::InterfaceState::Hub)
    {
        return false;
    }

    if (app_.activeCustomizationDragId_)
    {
        if ((event.motion.state & SDL_BUTTON_LMASK) == 0)
        {
            app_.activeCustomizationDragId_.reset();
        }
        else
        {
            app_.UpdateCustomizationValueFromPosition(*app_.activeCustomizationDragId_, event.motion.x);
        }
    }

    if (app_.resizeState_.target == Application::ResizeState::Target::None)
    {
        return true;
    }

    return false;
}

bool LibraryInputHandler::HandleMouseWheel(const SDL_Event& event, bool& running)
{
    (void)running;
    if (app_.interfaceState_ == Application::InterfaceState::Hub)
    {
        return false;
    }

    if (SDL_Rect viewport = app_.settingsRenderResult_.viewport; viewport.w > 0 && viewport.h > 0
        && Application::IsSettingsProgramId(app_.activeProgramId_))
    {
        int mouseX = 0;
        int mouseY = 0;
        SDL_GetMouseState(&mouseX, &mouseY);
        if (app_.PointInRect(viewport, mouseX, mouseY))
        {
            int wheelY = event.wheel.y;
            if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
            {
                wheelY = -wheelY;
            }

            if (wheelY == 0)
            {
                return true;
            }

            const int maxScroll = std::max(0, app_.settingsRenderResult_.contentHeight - app_.settingsRenderResult_.viewport.h);
            if (maxScroll <= 0)
            {
                return true;
            }

            constexpr int kScrollStep = 48;
            const int delta = -wheelY * kScrollStep;
            app_.settingsScrollOffset_ = std::clamp(app_.settingsScrollOffset_ + delta, 0, maxScroll);
            return true;
        }
    }

    auto visualsIt = app_.programVisuals_.find(app_.activeProgramId_);
    if (visualsIt == app_.programVisuals_.end())
    {
        return false;
    }

    auto& visuals = visualsIt->second;
    if (visuals.sectionsViewport.w <= 0 || visuals.sectionsViewport.h <= 0)
    {
        return false;
    }

    int mouseX = 0;
    int mouseY = 0;
    SDL_GetMouseState(&mouseX, &mouseY);
    if (!app_.PointInRect(visuals.sectionsViewport, mouseX, mouseY))
    {
        return false;
    }

    int wheelY = event.wheel.y;
    if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
    {
        wheelY = -wheelY;
    }

    if (wheelY == 0)
    {
        return false;
    }

    const int viewportHeight = std::max(0, visuals.sectionsViewportContentHeight);
    const int maxScroll = std::max(0, visuals.sectionsContentHeight - viewportHeight);
    if (maxScroll <= 0)
    {
        return false;
    }

    const int scrollStep = ui::Scale(40);
    const int delta = -wheelY * scrollStep;
    visuals.sectionsScrollOffset = std::clamp(visuals.sectionsScrollOffset + delta, 0, maxScroll);
    return true;
}

bool LibraryInputHandler::HandleMouseRightClick(const SDL_Event& event, bool& running)
{
    (void)running;
    (void)event;
    return false;
}

bool LibraryInputHandler::HandleKeyDown(const SDL_Event& event, bool& running)
{
    (void)running;
    (void)event;
    return false;
}

bool LibraryInputHandler::HandleTextInput(const SDL_Event& event, bool& running)
{
    (void)running;
    if (app_.interfaceState_ == Application::InterfaceState::Hub)
    {
        return false;
    }

    if (app_.libraryFilterFocused_)
    {
        constexpr std::size_t kMaxFilterLength = 120;
        const std::size_t currentLength = app_.libraryFilterDraft_.size();
        const std::size_t incomingLength = std::strlen(event.text.text);
        if (incomingLength > 0 && currentLength < kMaxFilterLength)
        {
            app_.libraryFilterDraft_.append(event.text.text, std::min(incomingLength, kMaxFilterLength - currentLength));
            app_.QueueLibraryFilterUpdate();
        }
        return true;
    }

    return false;
}

} // namespace colony::input
