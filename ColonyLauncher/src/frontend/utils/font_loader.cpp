#include "frontend/utils/font_loader.hpp"

#include "ui/layout.hpp"
#include "utils/asset_paths.hpp"

#include <SDL2/SDL.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <initializer_list>
#include <unordered_set>
#include <vector>

namespace colony::frontend::fonts
{
namespace
{
std::filesystem::path GetOverridePath(const char* variable)
{
    if (!variable)
    {
        return {};
    }

    if (const char* value = std::getenv(variable); value != nullptr && *value != '\0')
    {
        return std::filesystem::path{value};
    }

    return {};
}

void AppendIfExists(std::vector<std::filesystem::path>& candidates, const std::filesystem::path& path)
{
    if (path.empty())
    {
        return;
    }

    std::error_code error;
    if (!std::filesystem::exists(path, error))
    {
        return;
    }

    if (std::find(candidates.begin(), candidates.end(), path) == candidates.end())
    {
        candidates.push_back(path);
    }
}

std::vector<std::filesystem::path> CandidateFilesForRole(FontRole role, const LoadFontSetParams& params)
{
    std::vector<std::filesystem::path> candidates;
    candidates.reserve(8);

    const std::filesystem::path overrideDisplay = GetOverridePath("COLONY_DISPLAY_FONT");
    const std::filesystem::path overrideHeadline = GetOverridePath("COLONY_HEADLINE_FONT");
    const std::filesystem::path overrideBody = GetOverridePath("COLONY_BODY_FONT");
    const std::filesystem::path overrideGeneral = GetOverridePath("COLONY_UI_FONT");

    auto appendOverride = [&](FontRole targetRole, const std::filesystem::path& candidate) {
        if (role == targetRole && !candidate.empty())
        {
            AppendIfExists(candidates, candidate);
        }
    };

    appendOverride(FontRole::Display, overrideDisplay);
    appendOverride(FontRole::Headline, overrideHeadline);
    appendOverride(FontRole::Body, overrideBody);
    appendOverride(FontRole::Subtitle, overrideBody);
    appendOverride(FontRole::Title, overrideHeadline);
    appendOverride(FontRole::Label, overrideBody);
    appendOverride(FontRole::Caption, overrideBody);

    if (!overrideGeneral.empty())
    {
        AppendIfExists(candidates, overrideGeneral);
    }

    const std::filesystem::path baseInterDir = colony::paths::ResolveAssetDirectory("assets/fonts/Inter");
    const std::filesystem::path basePoppinsDir = colony::paths::ResolveAssetDirectory("assets/fonts/Poppins");

    const auto appendFontFamily = [&](const std::filesystem::path& root, std::initializer_list<const char*> names) {
        for (const char* name : names)
        {
            if (!name)
            {
                continue;
            }
            AppendIfExists(candidates, root / name);
            AppendIfExists(candidates, root / "static" / name);
            AppendIfExists(candidates, root / "ttf" / name);
        }
    };

    switch (role)
    {
    case FontRole::Display:
        appendFontFamily(baseInterDir, {"Inter-ExtraBold.ttf", "Inter-SemiBold.ttf", "Inter-Bold.ttf"});
        appendFontFamily(basePoppinsDir, {"Poppins-SemiBold.ttf", "Poppins-Bold.ttf"});
        break;
    case FontRole::Headline:
        appendFontFamily(baseInterDir, {"Inter-SemiBold.ttf", "Inter-Medium.ttf"});
        appendFontFamily(basePoppinsDir, {"Poppins-Medium.ttf", "Poppins-SemiBold.ttf"});
        break;
    case FontRole::Title:
        appendFontFamily(baseInterDir, {"Inter-Medium.ttf", "Inter-Regular.ttf"});
        appendFontFamily(basePoppinsDir, {"Poppins-Medium.ttf", "Poppins-Regular.ttf"});
        break;
    case FontRole::Subtitle:
        appendFontFamily(baseInterDir, {"Inter-Regular.ttf"});
        appendFontFamily(basePoppinsDir, {"Poppins-Regular.ttf"});
        break;
    case FontRole::Body:
        appendFontFamily(baseInterDir, {"Inter-Regular.ttf", "Inter-Light.ttf"});
        appendFontFamily(basePoppinsDir, {"Poppins-Regular.ttf", "Poppins-Light.ttf"});
        break;
    case FontRole::Label:
        appendFontFamily(baseInterDir, {"Inter-Medium.ttf", "Inter-Regular.ttf"});
        appendFontFamily(basePoppinsDir, {"Poppins-Medium.ttf", "Poppins-Regular.ttf"});
        break;
    case FontRole::Caption:
        appendFontFamily(baseInterDir, {"Inter-Regular.ttf", "Inter-Light.ttf"});
        appendFontFamily(basePoppinsDir, {"Poppins-Regular.ttf", "Poppins-Light.ttf"});
        break;
    }

    AppendIfExists(candidates, params.configuration.primaryFontPath);

    if (const std::filesystem::path bundled = colony::fonts::GetBundledFontPath(); !bundled.empty())
    {
        AppendIfExists(candidates, bundled);
    }

    return candidates;
}

sdl::FontHandle OpenFontForRole(FontRole role, int size, const LoadFontSetParams& params)
{
    if (size <= 0)
    {
        return {};
    }

    const std::vector<std::filesystem::path> candidates = CandidateFilesForRole(role, params);
    for (const auto& candidate : candidates)
    {
        if (candidate.empty())
        {
            continue;
        }

        sdl::FontHandle font{TTF_OpenFont(candidate.string().c_str(), colony::ui::ScaleDynamic(size))};
        if (font)
        {
            return font;
        }
    }

    if (!candidates.empty())
    {
        std::cerr << "Failed to load font for role. Candidates:";
        for (const auto& candidate : candidates)
        {
            std::cerr << ' ' << candidate;
        }
        std::cerr << "\n";
    }

    return {};
}

} // namespace

FontSet LoadFontSet(const LoadFontSetParams& params)
{
    FontSet fonts{};
    fonts.display = OpenFontForRole(FontRole::Display, params.typography.display.size, params);
    fonts.headline = OpenFontForRole(FontRole::Headline, params.typography.headline.size, params);
    fonts.title = OpenFontForRole(FontRole::Title, params.typography.title.size, params);
    fonts.subtitle = OpenFontForRole(FontRole::Subtitle, params.typography.subtitle.size, params);
    fonts.body = OpenFontForRole(FontRole::Body, params.typography.body.size, params);
    fonts.label = OpenFontForRole(FontRole::Label, params.typography.label.size, params);
    fonts.caption = OpenFontForRole(FontRole::Caption, params.typography.caption.size, params);
    return fonts;
}

std::filesystem::path ResolveFontForRole(FontRole role, const LoadFontSetParams& params)
{
    const std::vector<std::filesystem::path> candidates = CandidateFilesForRole(role, params);
    if (candidates.empty())
    {
        return {};
    }
    return candidates.front();
}

} // namespace colony::frontend::fonts
