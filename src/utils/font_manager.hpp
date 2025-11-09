#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace colony::fonts
{

std::filesystem::path GetBundledFontPath();

bool EnsureBundledFontAvailable();

std::string ResolveFontPath();

std::string ResolveFontPathForLanguage(std::string_view languageId);

} // namespace colony::fonts
