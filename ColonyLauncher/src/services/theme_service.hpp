#pragma once

#include "services/settings_service.hpp"
#include "ui/theme.hpp"

#include <array>

namespace colony::services
{

struct CustomThemeFieldDefinition
{
    const char* id;
    SDL_Color ui::ThemeColors::*member;
    const char* localizationKey;
};

inline constexpr std::size_t kCustomThemeFieldCount = 16;

const std::array<CustomThemeFieldDefinition, kCustomThemeFieldCount>& CustomThemeFields();

class ThemeService
{
  public:
    explicit ThemeService(ui::ThemeManager& themeManager);

    struct ThemeBuildResult
    {
        ui::ThemeColors theme;
        ui::Typography typography;
        ui::InteractionColors interactions;
        ui::MotionTimings motion;
    };

    ThemeBuildResult BuildTheme(const SettingsService& settingsService) const;

  private:
    void ApplyInterfaceDensity(const SettingsService& settingsService) const;
    void ApplyAppearanceCustomizations(ui::ThemeColors& theme, const SettingsService& settingsService) const;
    void RebuildInteractionPalette(const ui::ThemeColors& theme, ui::InteractionColors& interactions) const;

    ui::ThemeManager& themeManager_;
};

} // namespace colony::services
