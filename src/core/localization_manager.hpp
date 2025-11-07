#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace colony
{

class LocalizationManager
{
  public:
    using StringsMap = std::unordered_map<std::string, std::string>;

    void SetResourceDirectory(std::filesystem::path directory);
    void SetFallbackLanguage(std::string languageId);

    [[nodiscard]] const std::filesystem::path& ResourceDirectory() const noexcept { return resourceDirectory_; }
    [[nodiscard]] const std::string& ActiveLanguage() const noexcept { return activeLanguageId_; }
    [[nodiscard]] const std::string& FallbackLanguage() const noexcept { return fallbackLanguageId_; }

    bool LoadLanguage(const std::string& languageId);

    [[nodiscard]] std::string GetString(std::string_view key) const;
    [[nodiscard]] std::string GetStringOrDefault(std::string_view key, std::string_view fallback) const;

  private:
    using StringPair = std::pair<std::string, std::string>;

    [[nodiscard]] std::filesystem::path ResolveLanguageFile(const std::string& languageId) const;
    bool LoadFromFile(const std::filesystem::path& path, StringsMap& outStrings) const;
    bool LoadJson(std::istream& stream, StringsMap& outStrings) const;
    bool LoadYaml(std::istream& stream, StringsMap& outStrings) const;
    bool EnsureFallbackLoaded();

    std::filesystem::path resourceDirectory_{};
    std::string activeLanguageId_{};
    std::string fallbackLanguageId_{"en"};
    StringsMap activeStrings_{};
    StringsMap fallbackStrings_{};
};

} // namespace colony
