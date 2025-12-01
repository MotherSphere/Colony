#pragma once

#include "ui/theme.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

namespace colony::services
{

class SettingsService
{
  public:
    SettingsService();

    [[nodiscard]] const std::string& ActiveLanguageId() const noexcept { return activeLanguageId_; }
    void SetActiveLanguageId(std::string languageId);

    [[nodiscard]] const std::unordered_map<std::string, bool>& ToggleStates() const noexcept { return basicToggleStates_; }
    [[nodiscard]] std::unordered_map<std::string, bool>& ToggleStates() noexcept { return basicToggleStates_; }

    [[nodiscard]] const std::unordered_map<std::string, float>& AppearanceCustomizationValues() const noexcept
    {
        return appearanceCustomizationValues_;
    }

    [[nodiscard]] float GetAppearanceCustomizationValue(std::string_view id) const;
    bool SetAppearanceCustomizationValue(const std::string& id, float value);

    [[nodiscard]] const std::string& PythonInterpreterPath() const noexcept { return pythonInterpreterPath_; }
    void SetPythonInterpreterPath(std::string interpreter);
    [[nodiscard]] std::string ResolvedPythonInterpreter() const;

    void Load(const std::filesystem::path& settingsPath, ui::ThemeManager& themeManager);
    void Save(const std::filesystem::path& settingsPath, const ui::ThemeManager& themeManager) const;

  private:
    static std::string DefaultPythonInterpreter();

    std::string activeLanguageId_ = "en";
    std::unordered_map<std::string, bool> basicToggleStates_;
    std::unordered_map<std::string, float> appearanceCustomizationValues_;
    std::string pythonInterpreterPath_;
};

} // namespace colony::services
