#pragma once

#include <filesystem>
#include <string_view>

namespace colony::paths
{
std::filesystem::path ResolveAssetPath(std::string_view relativePath);
std::filesystem::path ResolveAssetDirectory(std::string_view relativePath);
} // namespace colony::paths

