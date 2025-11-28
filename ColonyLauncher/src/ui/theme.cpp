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
    ThemeColors colors{};

    const SDL_Color backgroundColor = colony::color::ParseHexColor(background);
    const SDL_Color navColor = colony::color::ParseHexColor(nav);
    const SDL_Color libraryColor = colony::color::ParseHexColor(library);
    const SDL_Color cardColor = colony::color::ParseHexColor(card);
    const SDL_Color cardHoverColor = colony::color::ParseHexColor(cardHover);
    const SDL_Color cardActiveColor = colony::color::ParseHexColor(cardActive);
    const SDL_Color navTextColor = colony::color::ParseHexColor(navText);
    const SDL_Color heroTitleColor = colony::color::ParseHexColor(heroTitle);
    const SDL_Color heroBodyColor = colony::color::ParseHexColor(heroBody);
    const SDL_Color mutedColor = colony::color::ParseHexColor(muted);
    const SDL_Color borderColor = colony::color::ParseHexColor(border);
    const SDL_Color statusBarColor = colony::color::ParseHexColor(statusBar);
    const SDL_Color statusBarTextColor = colony::color::ParseHexColor(statusBarText);
    const SDL_Color channelBadgeColor = colony::color::ParseHexColor(channelBadge);
    const SDL_Color gradientStartColor = colony::color::ParseHexColor(gradientStart);
    const SDL_Color gradientEndColor = colony::color::ParseHexColor(gradientEnd);

    colors.background = backgroundColor;
    colors.navRail = navColor;
    colors.surface = colony::color::Mix(libraryColor, backgroundColor, 0.55f);
    colors.surfaceAlt = colony::color::Mix(navColor, backgroundColor, 0.4f);
    colors.elevatedSurface = colony::color::Mix(cardActiveColor, backgroundColor, 0.35f);
    colors.card = cardColor;
    colors.cardHover = cardHoverColor;
    colors.cardActive = cardActiveColor;
    colors.divider = colony::color::Mix(borderColor, backgroundColor, 0.45f);
    colors.navText = navTextColor;
    colors.navTextMuted = colony::color::Mix(navTextColor, mutedColor, 0.6f);
    colors.heroTitle = heroTitleColor;
    colors.heroBody = heroBodyColor;
    colors.muted = mutedColor;
    colors.border = borderColor;
    colors.statusBar = statusBarColor;
    colors.statusBarText = statusBarTextColor;
    colors.channelBadge = channelBadgeColor;
    colors.chipBackground = colony::color::Mix(cardHoverColor, backgroundColor, 0.65f);
    colors.inputBackground = colony::color::Mix(cardColor, libraryColor, 0.6f);
    colors.inputBorder = colony::color::Mix(borderColor, cardActiveColor, 0.45f);
    colors.inputPlaceholder = colony::color::Mix(mutedColor, heroBodyColor, 0.55f);
    colors.buttonPrimary = colony::color::Mix(channelBadgeColor, heroTitleColor, 0.1f);
    colors.buttonPrimaryHover = colony::color::Mix(colors.buttonPrimary, heroTitleColor, 0.28f);
    colors.buttonPrimaryActive = colony::color::Mix(colors.buttonPrimary, heroBodyColor, 0.35f);
    colors.buttonGhost = colony::color::Mix(cardColor, backgroundColor, 0.38f);
    colors.buttonGhostHover = colony::color::Mix(cardHoverColor, backgroundColor, 0.38f);
    colors.focusRing = colony::color::Mix(channelBadgeColor, heroTitleColor, 0.28f);
    colors.success = colony::color::Mix(channelBadgeColor, SDL_Color{68, 214, 163, SDL_ALPHA_OPAQUE}, 0.55f);
    colors.warning = colony::color::Mix(channelBadgeColor, SDL_Color{255, 190, 118, SDL_ALPHA_OPAQUE}, 0.65f);
    colors.info = colony::color::Mix(channelBadgeColor, SDL_Color{109, 207, 246, SDL_ALPHA_OPAQUE}, 0.45f);
    colors.overlay = colony::color::Mix(backgroundColor, heroTitleColor, 0.82f);
    colors.overlay.a = 208;

    colors.libraryBackground = libraryColor;
    colors.libraryCard = cardColor;
    colors.libraryCardHover = cardHoverColor;
    colors.libraryCardActive = cardActiveColor;
    colors.heroGradientFallbackStart = gradientStartColor;
    colors.heroGradientFallbackEnd = gradientEndColor;

    return colors;
}

InteractionColors BuildDefaultInteractions(const ThemeColors& colors)
{
    InteractionColors interactions{};
    interactions.hover = colony::color::Mix(colors.cardHover, colors.card, 0.5f);
    interactions.active = colony::color::Mix(colors.cardActive, colors.cardHover, 0.55f);
    interactions.focus = colors.focusRing;
    interactions.subtleGlow = colony::color::Mix(colors.channelBadge, colors.buttonGhost, 0.35f);
    interactions.subtleGlow.a = 90;
    return interactions;
}

Typography BuildDefaultTypography(const ThemeColors& colors)
{
    Typography typography{};
    typography.display = TypographyStyle{48, 1.16f, colors.heroTitle};
    typography.headline = TypographyStyle{34, 1.18f, colors.heroTitle};
    typography.title = TypographyStyle{26, 1.2f, colors.heroTitle};
    typography.subtitle = TypographyStyle{20, 1.24f, colors.heroBody};
    typography.body = TypographyStyle{17, 1.32f, colors.heroBody};
    typography.label = TypographyStyle{15, 1.28f, colors.navText};
    typography.caption = TypographyStyle{13, 1.32f, colors.muted};
    return typography;
}

MotionTimings BuildDefaultMotionTimings()
{
    MotionTimings motion{};
    motion.fast = 0.14f;
    motion.standard = 0.26f;
    motion.slow = 0.45f;
    motion.hoverLift = 6.5f;
    motion.focusScale = 1.04f;
    return motion;
}

void ApplyDefaults(ColorScheme& scheme)
{
    if (scheme.interactions.hover.a == 0 && scheme.interactions.active.a == 0 && scheme.interactions.focus.a == 0)
    {
        scheme.interactions = BuildDefaultInteractions(scheme.colors);
    }

    if (scheme.typography.display.size == 0)
    {
        scheme.typography = BuildDefaultTypography(scheme.colors);
    }

    if (scheme.motion.standard <= 0.0f)
    {
        scheme.motion = BuildDefaultMotionTimings();
    }
}

void AddColonySignature(std::vector<ColorScheme>& schemes)
{
    ColorScheme scheme{
        .id = "colony_aurora",
        .name = "Colony Aurora",
        .colors = MakeTheme(
            "#1E1E2A",
            "#171824",
            "#222437",
            "#282B40",
            "#2D3248",
            "#353B55",
            "#F6F7FF",
            "#FFFFFF",
            "#D5D7E6",
            "#9CA2C6",
            "#31364A",
            "#181B2A",
            "#F1F4FF",
            "#FF9F43",
            "#242A40",
            "#181D30")};

    scheme.colors.buttonPrimary = colony::color::ParseHexColor("#FF9F43");
    scheme.colors.buttonPrimaryHover = colony::color::ParseHexColor("#FFB15E");
    scheme.colors.buttonPrimaryActive = colony::color::ParseHexColor("#E9831F");
    scheme.colors.channelBadge = scheme.colors.buttonPrimary;
    scheme.colors.focusRing = colony::color::ParseHexColor("#FFB56A");
    scheme.colors.success = colony::color::ParseHexColor("#53DFAD");
    scheme.colors.warning = colony::color::ParseHexColor("#FFC878");
    scheme.colors.info = colony::color::ParseHexColor("#6DCFF6");

    scheme.typography.display = TypographyStyle{48, 1.16f, scheme.colors.heroTitle};
    scheme.typography.headline = TypographyStyle{34, 1.18f, scheme.colors.heroTitle};
    scheme.typography.title = TypographyStyle{26, 1.22f, scheme.colors.heroTitle};
    scheme.typography.subtitle = TypographyStyle{20, 1.26f, scheme.colors.heroBody};
    scheme.typography.body = TypographyStyle{17, 1.34f, scheme.colors.heroBody};
    scheme.typography.label = TypographyStyle{15, 1.28f, scheme.colors.navText};
    scheme.typography.caption = TypographyStyle{13, 1.32f, scheme.colors.muted};

    scheme.motion = BuildDefaultMotionTimings();
    scheme.interactions = BuildDefaultInteractions(scheme.colors);

    schemes.push_back(std::move(scheme));
}

void AddCatppuccin(std::vector<ColorScheme>& schemes)
{
    {
        ColorScheme scheme{
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
                "#dce1ec")};
        ApplyDefaults(scheme);
        schemes.push_back(std::move(scheme));
    }

    {
        ColorScheme scheme{
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
                "#1f2233")};
        ApplyDefaults(scheme);
        schemes.push_back(std::move(scheme));
    }

    {
        ColorScheme scheme{
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
                "#111320")};
        ApplyDefaults(scheme);
        schemes.push_back(std::move(scheme));
    }

    {
        ColorScheme scheme{
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
                "#0f111a")};
        ApplyDefaults(scheme);
        schemes.push_back(std::move(scheme));
    }
}

void AddClassicSchemes(std::vector<ColorScheme>& schemes)
{
    {
        ColorScheme scheme{
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
                "#1a1a1a")};
        ApplyDefaults(scheme);
        schemes.push_back(std::move(scheme));
    }

    {
        ColorScheme scheme{
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
                "#041f27")};
        ApplyDefaults(scheme);
        schemes.push_back(std::move(scheme));
    }

    {
        ColorScheme scheme{
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
                "#0c1214")};
        ApplyDefaults(scheme);
        schemes.push_back(std::move(scheme));
    }

    {
        ColorScheme scheme{
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
                "#080a10")};
        ApplyDefaults(scheme);
        schemes.push_back(std::move(scheme));
    }

    {
        ColorScheme scheme{
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
                "#0e1117")};
        ApplyDefaults(scheme);
        schemes.push_back(std::move(scheme));
    }
}

} // namespace

ThemeManager::ThemeManager()
{
    schemes_.reserve(10);
    AddColonySignature(schemes_);
    AddCatppuccin(schemes_);
    AddClassicSchemes(schemes_);
    if (!schemes_.empty())
    {
        active_ = &schemes_.front();
    }
}

const ColorScheme& ThemeManager::AddCustomScheme(ColorScheme scheme, bool makeActive)
{
    scheme.isCustom = true;
    ApplyDefaults(scheme);

    const auto findById = [&](std::string_view id) {
        return std::find_if(schemes_.begin(), schemes_.end(), [&](const ColorScheme& candidate) {
            return candidate.id == id;
        });
    };

    const std::string currentActiveId = active_ ? active_->id : std::string{};
    auto existingIt = findById(scheme.id);
    if (existingIt != schemes_.end())
    {
        const bool wasActive = active_ == &*existingIt;
        *existingIt = std::move(scheme);
        if (makeActive || wasActive)
        {
            active_ = &*existingIt;
        }
        else if (!currentActiveId.empty())
        {
            auto activeIt = findById(currentActiveId);
            if (activeIt != schemes_.end())
            {
                active_ = &*activeIt;
            }
        }
        return *existingIt;
    }

    schemes_.push_back(std::move(scheme));
    const std::string desiredActiveId = makeActive ? schemes_.back().id : currentActiveId;
    if (!desiredActiveId.empty())
    {
        auto activeIt = findById(desiredActiveId);
        if (activeIt != schemes_.end())
        {
            active_ = &*activeIt;
        }
        else
        {
            active_ = &schemes_.back();
        }
    }
    else
    {
        active_ = &schemes_.back();
    }

    return schemes_.back();
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

} // namespace colony::ui
