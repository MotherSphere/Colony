#include "ui/theme.hpp"

#include "utils/color.hpp"

#include <algorithm>
#include <cmath>

namespace colony::ui
{
namespace
{
ThemeColors MakeTheme(
    std::string_view background,
    std::string_view nav,
    std::string_view library,
    std::string_view card,
    std::string_view cardHover,
    std::string_view cardActive,
    std::string_view navText,
    std::string_view heroTitle,
    std::string_view heroBody,
    std::string_view muted,
    std::string_view border,
    std::string_view statusBar,
    std::string_view statusBarText,
    std::string_view channelBadge,
    std::string_view gradientStart,
    std::string_view gradientEnd)
{
    ThemeColors colors;
    colors.background = colony::color::ParseHexColor(background);
    colors.navRail = colony::color::ParseHexColor(nav);
    colors.libraryBackground = colony::color::ParseHexColor(library);
    colors.libraryCard = colony::color::ParseHexColor(card);
    colors.libraryCardHover = colony::color::ParseHexColor(cardHover);
    colors.libraryCardActive = colony::color::ParseHexColor(cardActive);
    colors.navText = colony::color::ParseHexColor(navText);
    colors.heroTitle = colony::color::ParseHexColor(heroTitle);
    colors.heroBody = colony::color::ParseHexColor(heroBody);
    colors.muted = colony::color::ParseHexColor(muted);
    colors.border = colony::color::ParseHexColor(border);
    colors.statusBar = colony::color::ParseHexColor(statusBar);
    colors.statusBarText = colony::color::ParseHexColor(statusBarText);
    colors.channelBadge = colony::color::ParseHexColor(channelBadge);
    colors.heroGradientFallbackStart = colony::color::ParseHexColor(gradientStart);
    colors.heroGradientFallbackEnd = colony::color::ParseHexColor(gradientEnd);
    return colors;
}

ColorScheme::ThemeAnimations MakeAnimations(
    ColorScheme::ThemeAnimations::Easing easing,
    float pulsePeriod,
    float fadeDuration)
{
    ColorScheme::ThemeAnimations animations;
    animations.heroPulseEasing = easing;
    animations.heroPulsePeriod = pulsePeriod;
    animations.heroFadeDuration = fadeDuration;
    return animations;
}

void AddCatppuccin(std::vector<ColorScheme>& schemes)
{
    schemes.push_back(ColorScheme{
        .id = "catppuccin_latte",
        .name = "Catppuccin Latte",
        .colors = MakeTheme(
            "#eff1f5",
            "#dce0e8",
            "#e6e9ef",
            "#ccd0da",
            "#bcc0cc",
            "#acb0be",
            "#4c4f69",
            "#1e66f5",
            "#5c5f77",
            "#6c6f85",
            "#acb0be",
            "#dce0e8",
            "#4c4f69",
            "#ccd0da",
            "#e5e9f3",
            "#dce1ec"),
        .animations = MakeAnimations(ColorScheme::ThemeAnimations::Easing::EaseInOut, 6.0f, 0.45f)});

    schemes.push_back(ColorScheme{
        .id = "catppuccin_frappe",
        .name = "Catppuccin Frappé",
        .colors = MakeTheme(
            "#303446",
            "#292c3c",
            "#414559",
            "#51576d",
            "#626880",
            "#737994",
            "#c6d0f5",
            "#c6d0f5",
            "#a5adce",
            "#949cbb",
            "#51576d",
            "#232634",
            "#c6d0f5",
            "#414559",
            "#2f3144",
            "#1f2233"),
        .animations = MakeAnimations(ColorScheme::ThemeAnimations::Easing::EaseOut, 7.25f, 0.5f)});

    schemes.push_back(ColorScheme{
        .id = "catppuccin_macchiato",
        .name = "Catppuccin Macchiato",
        .colors = MakeTheme(
            "#24273a",
            "#1e2030",
            "#2f3349",
            "#363a4f",
            "#494d64",
            "#5b6078",
            "#cad3f5",
            "#cad3f5",
            "#a5adcb",
            "#939ab7",
            "#494d64",
            "#181926",
            "#cad3f5",
            "#2f3349",
            "#1a1d2c",
            "#111320"),
        .animations = MakeAnimations(ColorScheme::ThemeAnimations::Easing::EaseInOut, 6.5f, 0.42f)});

    schemes.push_back(ColorScheme{
        .id = "catppuccin_mocha",
        .name = "Catppuccin Mocha",
        .colors = MakeTheme(
            "#11111b",
            "#181825",
            "#1e1e2e",
            "#313244",
            "#45475a",
            "#585b70",
            "#cdd6f4",
            "#cdd6f4",
            "#a6adc8",
            "#9399b2",
            "#45475a",
            "#181825",
            "#cdd6f4",
            "#2c2f40",
            "#161925",
            "#0f111a"),
        .animations = MakeAnimations(ColorScheme::ThemeAnimations::Easing::EaseOut, 7.0f, 0.5f)});
}

void AddClassicSchemes(std::vector<ColorScheme>& schemes)
{
    schemes.push_back(ColorScheme{
        .id = "gruvbox_dark",
        .name = "Gruvbox Dark",
        .colors = MakeTheme(
            "#1d2021",
            "#282828",
            "#32302f",
            "#3c3836",
            "#504945",
            "#665c54",
            "#ebdbb2",
            "#fbf1c7",
            "#d5c4a1",
            "#bdae93",
            "#504945",
            "#282828",
            "#ebdbb2",
            "#3c3836",
            "#252525",
            "#1a1a1a"),
        .animations = MakeAnimations(ColorScheme::ThemeAnimations::Easing::EaseIn, 5.5f, 0.38f)});

    schemes.push_back(ColorScheme{
        .id = "solarized_dark",
        .name = "Solarized Dark",
        .colors = MakeTheme(
            "#002b36",
            "#073642",
            "#0b3948",
            "#114857",
            "#1b5b6a",
            "#256d7d",
            "#93a1a1",
            "#eee8d5",
            "#93a1a1",
            "#839496",
            "#1b5b6a",
            "#002b36",
            "#93a1a1",
            "#114857",
            "#0a2f3a",
            "#041f27"),
        .animations = MakeAnimations(ColorScheme::ThemeAnimations::Easing::EaseInOut, 6.8f, 0.44f)});

    schemes.push_back(ColorScheme{
        .id = "everblush",
        .name = "Everblush",
        .colors = MakeTheme(
            "#141b1e",
            "#0f1417",
            "#1b2225",
            "#232a2d",
            "#2d3437",
            "#383f42",
            "#e5c76b",
            "#e5c76b",
            "#b3cfa7",
            "#9da9ad",
            "#2d3437",
            "#0f1417",
            "#e5c76b",
            "#20272a",
            "#141b1e",
            "#0c1214"),
        .animations = MakeAnimations(ColorScheme::ThemeAnimations::Easing::EaseOut, 6.2f, 0.41f)});

    schemes.push_back(ColorScheme{
        .id = "cyberdream",
        .name = "Cyberdream",
        .colors = MakeTheme(
            "#05060a",
            "#0a0c12",
            "#10131c",
            "#181c28",
            "#202534",
            "#2a3040",
            "#f8f8ff",
            "#9aafff",
            "#c3c9ff",
            "#a1a8d9",
            "#2a3040",
            "#0a0c12",
            "#f8f8ff",
            "#1c2130",
            "#10131c",
            "#080a10"),
        .animations = MakeAnimations(ColorScheme::ThemeAnimations::Easing::EaseInOut, 5.8f, 0.4f)});

    schemes.push_back(ColorScheme{
        .id = "onedark",
        .name = "One Dark",
        .colors = MakeTheme(
            "#0f1419",
            "#171b24",
            "#1f2430",
            "#232834",
            "#303643",
            "#3b4252",
            "#abb2bf",
            "#e5c07b",
            "#abb2bf",
            "#9da5b4",
            "#303643",
            "#171b24",
            "#d0d7e2",
            "#262c39",
            "#151a21",
            "#0e1117"),
        .animations = MakeAnimations(ColorScheme::ThemeAnimations::Easing::EaseInOut, 6.0f, 0.45f)});
}

} // namespace

ThemeManager::ThemeManager()
{
    schemes_.reserve(10);
    AddCatppuccin(schemes_);
    AddClassicSchemes(schemes_);
    if (!schemes_.empty())
    {
        active_ = &schemes_.front();
    }
}

bool ThemeManager::SetActiveScheme(std::string_view id)
{
    const auto it = std::find_if(schemes_.begin(), schemes_.end(), [&](const ColorScheme& scheme) {
        return scheme.id == id;
    });
    if (it == schemes_.end())
    {
        return false;
    }
    active_ = &*it;
    return true;
}

float EvaluateEasing(ColorScheme::ThemeAnimations::Easing easing, float t) noexcept
{
    t = std::clamp(t, 0.0f, 1.0f);
    switch (easing)
    {
    case ColorScheme::ThemeAnimations::Easing::Linear:
        return t;
    case ColorScheme::ThemeAnimations::Easing::EaseIn:
        return t * t;
    case ColorScheme::ThemeAnimations::Easing::EaseOut:
        return 1.0f - std::pow(1.0f - t, 2.0f);
    case ColorScheme::ThemeAnimations::Easing::EaseInOut:
    default:
        return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
    }
}

} // namespace colony::ui
