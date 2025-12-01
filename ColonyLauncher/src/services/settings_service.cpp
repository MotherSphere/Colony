#include "services/settings_service.hpp"

#include "json.hpp"
#include "services/theme_service.hpp"
#include "utils/color.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

namespace colony::services
{
namespace
{
std::unordered_map<std::string, bool> BuildDefaultToggleStates()
{
    return std::unordered_map<std::string, bool>{
        {"notifications", true},
        {"sound", true},
        {"auto_updates", true},
        {"reduced_motion", false},
    };
}

std::unordered_map<std::string, float> BuildDefaultAppearanceValues()
{
    return std::unordered_map<std::string, float>{
        {"accent_intensity", 0.5f},
        {"background_depth", 0.5f},
        {"interface_density", 0.5f},
    };
}

} // namespace

SettingsService::SettingsService()
    : basicToggleStates_(BuildDefaultToggleStates())
    , appearanceCustomizationValues_(BuildDefaultAppearanceValues())
    , pythonInterpreterPath_(DefaultPythonInterpreter())
{}

std::string SettingsService::DefaultPythonInterpreter()
{
#if defined(_WIN32)
    return "py -3";
#else
    return "python3";
#endif
}

void SettingsService::SetActiveLanguageId(std::string languageId)
{
    if (!languageId.empty())
    {
        activeLanguageId_ = std::move(languageId);
    }
}

float SettingsService::GetAppearanceCustomizationValue(std::string_view id) const
{
    const auto it = appearanceCustomizationValues_.find(std::string{id});
    if (it != appearanceCustomizationValues_.end())
    {
        return it->second;
    }

    return 0.5f;
}

bool SettingsService::SetAppearanceCustomizationValue(const std::string& id, float value)
{
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    auto [it, inserted] = appearanceCustomizationValues_.try_emplace(id, clamped);
    if (!inserted)
    {
        if (std::abs(it->second - clamped) < 0.001f)
        {
            return false;
        }
        it->second = clamped;
        return true;
    }

    return true;
}

void SettingsService::SetPythonInterpreterPath(std::string interpreter)
{
    pythonInterpreterPath_ = std::move(interpreter);
}

std::string SettingsService::ResolvedPythonInterpreter() const
{
    if (!pythonInterpreterPath_.empty())
    {
        return pythonInterpreterPath_;
    }

    return DefaultPythonInterpreter();
}

void SettingsService::Load(const std::filesystem::path& settingsPath, ui::ThemeManager& themeManager)
{
    if (settingsPath.empty())
    {
        return;
    }

    std::error_code error;
    if (!std::filesystem::exists(settingsPath, error) || error)
    {
        return;
    }

    std::ifstream input{settingsPath};
    if (!input.is_open())
    {
        std::cerr << "Unable to open settings file: " << settingsPath << '\n';
        return;
    }

    try
    {
        const nlohmann::json document = nlohmann::json::parse(input);

        if (document.contains("theme") && document["theme"].is_string())
        {
            themeManager.SetActiveScheme(document["theme"].get<std::string>());
        }

        if (document.contains("language") && document["language"].is_string())
        {
            activeLanguageId_ = document["language"].get<std::string>();
        }

        if (document.contains("toggles") && document["toggles"].is_object())
        {
            for (const auto& [key, value] : document["toggles"].items())
            {
                if (!value.is_boolean())
                {
                    continue;
                }

                if (auto it = basicToggleStates_.find(key); it != basicToggleStates_.end())
                {
                    it->second = value.get<bool>();
                }
                else
                {
                    basicToggleStates_.emplace(key, value.get<bool>());
                }
            }
        }

        if (document.contains("customThemes") && document["customThemes"].is_array())
        {
            for (const auto& entry : document["customThemes"])
            {
                if (!entry.is_object())
                {
                    continue;
                }

                ui::ColorScheme scheme;
                scheme.isCustom = true;
                scheme.id = entry.value("id", std::string{});
                scheme.name = entry.value("name", scheme.id);

                if (!entry.contains("colors") || !entry["colors"].is_object() || scheme.id.empty())
                {
                    continue;
                }

                const auto& colorsObject = entry["colors"];
                bool missing = false;
                for (const auto& field : CustomThemeFields())
                {
                    if (!colorsObject.contains(field.id) || !colorsObject[field.id].is_string())
                    {
                        missing = true;
                        break;
                    }

                    const std::string colorValue = colorsObject[field.id].get<std::string>();
                    scheme.colors.*(field.member) = color::ParseHexColor(colorValue, scheme.colors.*(field.member));
                }

                if (missing)
                {
                    continue;
                }

                themeManager.AddCustomScheme(std::move(scheme));
            }
        }

        if (document.contains("appearance") && document["appearance"].is_object())
        {
            for (const auto& [key, value] : document["appearance"].items())
            {
                if (!value.is_number())
                {
                    continue;
                }

                SetAppearanceCustomizationValue(key, static_cast<float>(value.get<double>()));
            }
        }

        if (document.contains("pythonInterpreter") && document["pythonInterpreter"].is_string())
        {
            pythonInterpreterPath_ = document["pythonInterpreter"].get<std::string>();
        }
        else if (!document.contains("pythonInterpreter"))
        {
            pythonInterpreterPath_ = DefaultPythonInterpreter();
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to load settings: " << ex.what() << '\n';
    }
}

void SettingsService::Save(const std::filesystem::path& settingsPath, const ui::ThemeManager& themeManager) const
{
    if (settingsPath.empty())
    {
        return;
    }

    const std::filesystem::path directory = settingsPath.parent_path();
    std::error_code error;
    if (!directory.empty() && !std::filesystem::exists(directory, error))
    {
        std::filesystem::create_directories(directory, error);
        if (error)
        {
            std::cerr << "Unable to create settings directory: " << directory << '\n';
            return;
        }
    }

    nlohmann::json document;
    document["theme"] = themeManager.ActiveScheme().id;
    document["language"] = activeLanguageId_;
    nlohmann::json toggles = nlohmann::json::object();
    for (const auto& [key, value] : basicToggleStates_)
    {
        toggles[key] = value;
    }
    document["toggles"] = std::move(toggles);

    nlohmann::json appearance = nlohmann::json::object();
    for (const auto& [key, value] : appearanceCustomizationValues_)
    {
        appearance[key] = value;
    }
    document["appearance"] = std::move(appearance);

    document["pythonInterpreter"] = pythonInterpreterPath_;

    nlohmann::json customThemes = nlohmann::json::array();
    for (const auto& scheme : themeManager.Schemes())
    {
        if (!scheme.isCustom)
        {
            continue;
        }

        nlohmann::json colors = nlohmann::json::object();
        for (const auto& field : CustomThemeFields())
        {
            colors[field.id] = color::ToHexString(scheme.colors.*(field.member));
        }

        nlohmann::json entry = nlohmann::json::object();
        entry["id"] = scheme.id;
        entry["name"] = scheme.name;
        entry["colors"] = std::move(colors);
        customThemes.push_back(std::move(entry));
    }

    if (!customThemes.empty())
    {
        document["customThemes"] = std::move(customThemes);
    }

    std::ofstream output{settingsPath};
    if (!output.is_open())
    {
        std::cerr << "Unable to write settings file: " << settingsPath << '\n';
        return;
    }

    output << document.dump(2) << '\n';
}

} // namespace colony::services
