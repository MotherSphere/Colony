#pragma once

#include "core/content.hpp"
#include "json.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace colony
{

struct ContentLoadDiagnostic
{
    std::string filePath;
    std::string message;
    bool isError = false;
};

struct ContentCatalogLoadResult
{
    AppContent content;
    std::vector<ContentLoadDiagnostic> diagnostics;

    [[nodiscard]] bool HasErrors() const
    {
        for (const auto& diagnostic : diagnostics)
        {
            if (diagnostic.isError)
            {
                return true;
            }
        }
        return false;
    }
};

class ContentValidator
{
  public:
    AppContent LoadFromFile(const std::string& filePath) const;
    AppContent ParseDocument(const nlohmann::json& document) const;

  private:
    void ParseUserSection(const nlohmann::json& document, AppContent& content) const;
    void ParseViewsSection(const nlohmann::json& document, AppContent& content) const;
    ViewContent ParseViewContent(const std::string& viewId, const nlohmann::json& json) const;
    void ParseChannelsSection(const nlohmann::json& document, AppContent& content) const;
};

AppContent LoadContentFromFile(const std::string& filePath);
ContentCatalogLoadResult LoadContentCatalog(const std::filesystem::path& directory);

[[nodiscard]] std::filesystem::file_time_type QueryLatestWriteTime(const std::filesystem::path& directory);

} // namespace colony
