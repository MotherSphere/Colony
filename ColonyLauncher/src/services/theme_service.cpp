#include "services/theme_service.hpp"

#include "utils/color.hpp"
#include "ui/layout.hpp"

#include <algorithm>
#include <cmath>

namespace colony::services
{
namespace
{
const std::array<CustomThemeFieldDefinition, kCustomThemeFieldCount> kFields = {{
    {"background", &ui::ThemeColors::background, "settings.appearance.custom_theme.dialog.fields.background"},
    {"navRail", &ui::ThemeColors::navRail, "settings.appearance.custom_theme.dialog.fields.navRail"},
    {"libraryBackground", &ui::ThemeColors::libraryBackground, "settings.appearance.custom_theme.dialog.fields.libraryBackground"},
    {"libraryCard", &ui::ThemeColors::libraryCard, "settings.appearance.custom_theme.dialog.fields.libraryCard"},
    {"libraryCardHover", &ui::ThemeColors::libraryCardHover, "settings.appearance.custom_theme.dialog.fields.libraryCardHover"},
    {"libraryCardActive", &ui::ThemeColors::libraryCardActive, "settings.appearance.custom_theme.dialog.fields.libraryCardActive"},
    {"navText", &ui::ThemeColors::navText, "settings.appearance.custom_theme.dialog.fields.navText"},
    {"heroTitle", &ui::ThemeColors::heroTitle, "settings.appearance.custom_theme.dialog.fields.heroTitle"},
    {"heroBody", &ui::ThemeColors::heroBody, "settings.appearance.custom_theme.dialog.fields.heroBody"},
    {"muted", &ui::ThemeColors::muted, "settings.appearance.custom_theme.dialog.fields.muted"},
    {"border", &ui::ThemeColors::border, "settings.appearance.custom_theme.dialog.fields.border"},
    {"statusBar", &ui::ThemeColors::statusBar, "settings.appearance.custom_theme.dialog.fields.statusBar"},
    {"statusBarText", &ui::ThemeColors::statusBarText, "settings.appearance.custom_theme.dialog.fields.statusBarText"},
    {"channelBadge", &ui::ThemeColors::channelBadge, "settings.appearance.custom_theme.dialog.fields.channelBadge"},
    {"heroGradientFallbackStart", &ui::ThemeColors::heroGradientFallbackStart,
     "settings.appearance.custom_theme.dialog.fields.heroGradientFallbackStart"},
    {"heroGradientFallbackEnd", &ui::ThemeColors::heroGradientFallbackEnd,
     "settings.appearance.custom_theme.dialog.fields.heroGradientFallbackEnd"},
}};
} // namespace

const std::array<CustomThemeFieldDefinition, kCustomThemeFieldCount>& CustomThemeFields()
{
    return kFields;
}

ThemeService::ThemeService(ui::ThemeManager& themeManager)
    : themeManager_(themeManager)
{}

ThemeService::ThemeBuildResult ThemeService::BuildTheme(const SettingsService& settingsService) const
{
    ThemeBuildResult result;
    const ui::ColorScheme& activeScheme = themeManager_.ActiveScheme();
    result.theme = activeScheme.colors;
    result.typography = activeScheme.typography;
    result.interactions = activeScheme.interactions;
    result.motion = activeScheme.motion;

    ApplyInterfaceDensity(settingsService);
    ApplyAppearanceCustomizations(result.theme, settingsService);
    RebuildInteractionPalette(result.theme, result.interactions);

    return result;
}

void ThemeService::ApplyInterfaceDensity(const SettingsService& settingsService) const
{
    const float density = std::clamp(settingsService.GetAppearanceCustomizationValue("interface_density"), 0.0f, 1.0f);
    constexpr float kMinScale = 0.74f;
    constexpr float kMaxScale = 0.9f;
    const float scale = kMinScale + (kMaxScale - kMinScale) * density;
    ui::SetUiScale(scale);
}

void ThemeService::ApplyAppearanceCustomizations(ui::ThemeColors& theme, const SettingsService& settingsService) const
{
    const float accentValue = std::clamp(settingsService.GetAppearanceCustomizationValue("accent_intensity"), 0.0f, 1.0f);
    const float backgroundValue = std::clamp(settingsService.GetAppearanceCustomizationValue("background_depth"), 0.0f, 1.0f);

    const float accentDelta = accentValue - 0.5f;
    if (accentDelta > 0.0f)
    {
        theme.channelBadge = color::Mix(theme.channelBadge, theme.heroTitle, accentDelta * 0.6f);
        theme.libraryCardActive = color::Mix(theme.libraryCardActive, theme.heroTitle, accentDelta * 0.45f);
        theme.statusBar = color::Mix(theme.statusBar, theme.heroTitle, accentDelta * 0.35f);
    }
    else if (accentDelta < 0.0f)
    {
        const float factor = -accentDelta;
        theme.channelBadge = color::Mix(theme.channelBadge, theme.muted, factor * 0.6f);
        theme.libraryCardActive = color::Mix(theme.libraryCardActive, theme.muted, factor * 0.45f);
        theme.statusBar = color::Mix(theme.statusBar, theme.muted, factor * 0.35f);
    }

    const float depthDelta = backgroundValue - 0.5f;
    if (depthDelta != 0.0f)
    {
        const float depthAmount = std::abs(depthDelta) * 0.45f;
        const SDL_Color darkTarget{0, 0, 0, SDL_ALPHA_OPAQUE};
        const SDL_Color lightTarget{255, 255, 255, SDL_ALPHA_OPAQUE};
        const SDL_Color target = depthDelta > 0.0f ? darkTarget : lightTarget;

        auto adjust = [&](SDL_Color color) {
            return color::Mix(color, target, depthAmount);
        };

        theme.background = adjust(theme.background);
        theme.libraryBackground = adjust(theme.libraryBackground);
        theme.navRail = adjust(theme.navRail);
        theme.libraryCard = adjust(theme.libraryCard);
        theme.libraryCardHover = adjust(theme.libraryCardHover);
        theme.libraryCardActive = adjust(theme.libraryCardActive);
        theme.heroGradientFallbackStart = adjust(theme.heroGradientFallbackStart);
        theme.heroGradientFallbackEnd = adjust(theme.heroGradientFallbackEnd);
    }
}

void ThemeService::RebuildInteractionPalette(const ui::ThemeColors& theme, ui::InteractionColors& interactions) const
{
    interactions.hover = color::Mix(theme.libraryCardHover, theme.libraryCard, 0.5f);
    interactions.active = color::Mix(theme.libraryCardActive, theme.libraryCardHover, 0.55f);
    interactions.focus = theme.focusRing;
    interactions.subtleGlow = color::Mix(theme.channelBadge, theme.buttonGhost, 0.35f);
    interactions.subtleGlow.a = 90;
}

} // namespace colony::services
