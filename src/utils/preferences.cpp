#include "utils/preferences.hpp"

#include "json.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace colony::preferences
{
namespace
{
constexpr char kPreferencesDir[] = ".colony";
constexpr char kPreferencesFile[] = "config.json";

nlohmann::json Serialize(const Preferences& preferences)
{
    nlohmann::json json;
    json["themeId"] = preferences.themeId;
    json["languageId"] = preferences.languageId;
    json["lastProgramId"] = preferences.lastProgramId;
    json["lastChannelIndex"] = preferences.lastChannelIndex;
    json["toggleStates"] = preferences.toggleStates;
    return json;
}

Preferences Deserialize(const nlohmann::json& json)
{
    Preferences preferences;
    preferences.themeId = json.value("themeId", std::string{});
    preferences.languageId = json.value("languageId", std::string{});
    preferences.lastProgramId = json.value("lastProgramId", std::string{});
    preferences.lastChannelIndex = json.value("lastChannelIndex", 0);
    if (json.contains("toggleStates") && json["toggleStates"].is_object())
    {
        for (auto it = json["toggleStates"].begin(); it != json["toggleStates"].end(); ++it)
        {
            if (it.value().is_boolean())
            {
                preferences.toggleStates[it.key()] = it.value().get<bool>();
            }
        }
    }
    return preferences;
}

} // namespace

Preferences Load(const std::filesystem::path& path)
{
    Preferences preferences;
    std::error_code error;
    if (!std::filesystem::exists(path, error))
    {
        return preferences;
    }

    std::ifstream input{path};
    if (!input.is_open())
    {
        return preferences;
    }

    try
    {
        const nlohmann::json document = nlohmann::json::parse(input, nullptr, true, true);
        preferences = Deserialize(document);
    }
    catch (const std::exception&)
    {
        return preferences;
    }

    return preferences;
}

void Save(const Preferences& preferences, const std::filesystem::path& path)
{
    std::error_code error;
    const auto directory = path.parent_path();
    if (!directory.empty() && !std::filesystem::exists(directory, error))
    {
        std::filesystem::create_directories(directory, error);
    }

    std::ofstream output{path};
    if (!output.is_open())
    {
        return;
    }

    const auto json = Serialize(preferences);
    output << json.dump(2);
}

std::filesystem::path DefaultPath()
{
    std::filesystem::path base = std::filesystem::path{std::getenv("HOME") ? std::getenv("HOME") : ""};
#ifdef _WIN32
    if (base.empty())
    {
        if (const char* userProfile = std::getenv("USERPROFILE"))
        {
            base = userProfile;
        }
    }
#endif
    if (base.empty())
    {
        base = std::filesystem::current_path();
    }
    return base / kPreferencesDir / kPreferencesFile;
}

} // namespace colony::preferences
