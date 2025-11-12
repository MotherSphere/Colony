#pragma once

#include "core/content.hpp"
#include "json.hpp"

#include <string>

namespace colony
{

class ContentValidator
{
  public:
    AppContent LoadFromFile(const std::string& filePath) const;

  private:
    AppContent ParseDocument(const nlohmann::json& document) const;
    void ParseUserSection(const nlohmann::json& document, AppContent& content) const;
    void ParseViewsSection(const nlohmann::json& document, AppContent& content) const;
    ViewContent ParseViewContent(const std::string& viewId, const nlohmann::json& json) const;
    void ParseChannelsSection(const nlohmann::json& document, AppContent& content) const;
    void ParseHubSection(const nlohmann::json& document, AppContent& content) const;
    HubBranch ParseHubBranch(const nlohmann::json& json) const;
};

AppContent LoadContentFromFile(const std::string& filePath);

} // namespace colony
