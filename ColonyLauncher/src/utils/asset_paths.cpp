#include "utils/asset_paths.hpp"

#include <SDL2/SDL.h>

#include <system_error>
#include <vector>

namespace colony::paths
{
namespace
{
std::vector<std::filesystem::path> BuildAssetCandidates(std::string_view relativePath)
{
    std::vector<std::filesystem::path> candidates;
    const std::filesystem::path relative{relativePath};
    const std::filesystem::path launcherRelative = std::filesystem::path{"ColonyLauncher"} / relative;

    candidates.emplace_back(relative);
    candidates.emplace_back(launcherRelative);

    if (char* rawBasePath = SDL_GetBasePath(); rawBasePath != nullptr)
    {
        std::filesystem::path basePath{rawBasePath};
        SDL_free(rawBasePath);

        candidates.emplace_back(basePath / relative);
        candidates.emplace_back(basePath / launcherRelative);

        const std::filesystem::path parent = basePath.parent_path();
        if (!parent.empty())
        {
            candidates.emplace_back(parent / relative);
            candidates.emplace_back(parent / launcherRelative);
        }
    }

    return candidates;
}
} // namespace

std::filesystem::path ResolveAssetPath(std::string_view relativePath)
{
    std::error_code error;
    for (const auto& candidate : BuildAssetCandidates(relativePath))
    {
        if (std::filesystem::exists(candidate, error))
        {
            return candidate;
        }
    }

    return std::filesystem::path{relativePath};
}

std::filesystem::path ResolveAssetDirectory(std::string_view relativePath)
{
    std::error_code error;
    for (const auto& candidate : BuildAssetCandidates(relativePath))
    {
        if (std::filesystem::is_directory(candidate, error))
        {
            return candidate;
        }
    }

    return std::filesystem::path{relativePath};
}

} // namespace colony::paths

