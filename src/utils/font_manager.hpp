#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

namespace colony::fonts
{

std::filesystem::path GetBundledFontPath();

bool EnsureBundledFontAvailable();

struct FontConfiguration
{
    std::string primaryFontPath;
    std::unordered_map<std::string, std::string> nativeLanguageFonts;
};

FontConfiguration BuildFontConfiguration(std::string_view activeLanguageId);

} // namespace colony::fonts
