#include "content_loader.hpp"

#include "json.hpp"

#include <fstream>
#include <stdexcept>

namespace colony
{

namespace
{
ViewContent ParseViewContent(const std::string& viewId, const nlohmann::json& json)
{
    if (!json.contains("heading") || !json["heading"].is_string() || json["heading"].get<std::string>().empty())
    {
        throw std::runtime_error("View \"" + viewId + "\" requires a non-empty heading.");
    }
    if (!json.contains("primaryActionLabel") || !json["primaryActionLabel"].is_string()
        || json["primaryActionLabel"].get<std::string>().empty())
    {
        throw std::runtime_error("View \"" + viewId + "\" requires a non-empty primaryActionLabel.");
    }

    if (json.contains("statusMessage") && !json["statusMessage"].is_string())
    {
        throw std::runtime_error("View \"" + viewId + "\" must declare statusMessage as a string.");
    }

    ViewContent content;
    content.heading = json["heading"].get<std::string>();
    content.primaryActionLabel = json["primaryActionLabel"].get<std::string>();
    content.statusMessage = json.value("statusMessage", "");

    if (json.contains("paragraphs"))
    {
        if (!json["paragraphs"].is_array())
        {
            throw std::runtime_error("View \"" + viewId + "\" must declare paragraphs as an array.");
        }
        for (const auto& paragraph : json["paragraphs"])
        {
            if (!paragraph.is_string())
            {
                throw std::runtime_error("View \"" + viewId + "\" contains a non-string paragraph entry.");
            }
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

    if (!document.contains("navigation") || !document["navigation"].is_array())
    {
        throw std::runtime_error("Content file missing \"navigation\" array.");
    }
    for (const auto& item : document["navigation"])
    {
        if (!item.is_string() || item.get<std::string>().empty())
        {
            throw std::runtime_error("Navigation entries must be non-empty strings.");
        }
        content.navigation.emplace_back(item.get<std::string>());
    }
    if (content.navigation.empty())
    {
        throw std::runtime_error("Content file must declare at least one navigation entry.");
    }

    if (!document.contains("views") || !document["views"].is_object())
    {
        throw std::runtime_error("Content file missing \"views\" object.");
    }
    if (document["views"].empty())
    {
        throw std::runtime_error("Content file must declare at least one view.");
    }
    for (const auto& [id, value] : document["views"].items())
    {
        if (!value.is_object())
        {
            throw std::runtime_error("View \"" + id + "\" must be a JSON object.");
        }
        content.views.emplace(id, ParseViewContent(id, value));
    }

    return content;
}

} // namespace colony
