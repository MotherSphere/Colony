#pragma once

#include "core/content.hpp"

#include <string>

namespace nlohmann
{
class json;
}

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
};

AppContent LoadContentFromFile(const std::string& filePath);

} // namespace colony
