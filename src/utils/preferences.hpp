#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace colony::preferences
{

struct CustomProgram
{
    std::string id;
    std::string name;
    std::filesystem::path executable;
};

struct Preferences
{
    std::string themeId;
    std::string languageId;
    std::string lastProgramId;
    int lastChannelIndex = 0;
    std::unordered_map<std::string, bool> toggleStates;
    std::vector<CustomProgram> customPrograms;
};

[[nodiscard]] Preferences Load(const std::filesystem::path& path);
void Save(const Preferences& preferences, const std::filesystem::path& path);
[[nodiscard]] std::filesystem::path DefaultPath();

} // namespace colony::preferences
