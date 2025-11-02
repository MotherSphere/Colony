#include "content_loader.hpp"

#include "third_party/json.hpp"

#include <fstream>
#include <stdexcept>

namespace colony
{

namespace
{
ViewContent ParseViewContent(const nlohmann::json& json)
{
    ViewContent content;
    content.heading = json.value("heading", "");
    content.primaryActionLabel = json.value("primaryActionLabel", "");
    content.statusMessage = json.value("statusMessage", "");

    if (json.contains("paragraphs") && json["paragraphs"].is_array())
    {
        for (const auto& paragraph : json["paragraphs"])
        {
            content.paragraphs.emplace_back(paragraph.get<std::string>());
        }
    }

    return content;
}
} // namespace

AppContent LoadContentFromFile(const std::string& filePath)
{
    std::ifstream input{filePath};
    if (!input.is_open())
    {
        throw std::runtime_error("Failed to open content file: " + filePath);
    }

    nlohmann::json document = nlohmann::json::parse(input);

    AppContent content;
    content.brandName = document.value("brand", "COLONY");

    if (document.contains("navigation") && document["navigation"].is_array())
    {
        for (const auto& item : document["navigation"])
        {
            content.navigation.emplace_back(item.get<std::string>());
        }
    }

    if (document.contains("views") && document["views"].is_object())
    {
        for (const auto& [id, value] : document["views"].items())
        {
            content.views.emplace(id, ParseViewContent(value));
        }
    }

    return content;
}

} // namespace colony
