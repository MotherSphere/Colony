#pragma once

#include <filesystem>
#include <string>

namespace colony::fonts
{

std::filesystem::path GetBundledFontPath();

bool EnsureBundledFontAvailable();

std::string ResolveFontPath();

} // namespace colony::fonts
