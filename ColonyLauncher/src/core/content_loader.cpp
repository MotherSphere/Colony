#include "core/content_loader.hpp"

#include "json.hpp"

#include <fstream>
#include <stdexcept>
#include <utility>

namespace colony
{
namespace
{
const char* kViewsKey = "views";
const char* kChannelsKey = "channels";
const char* kUserKey = "user";
const char* kBrandKey = "brand";
const char* kHubKey = "hub";

std::ifstream OpenFile(const std::string& filePath)
{
    std::ifstream input{filePath};
    if (!input.is_open())
    {
        throw std::runtime_error("Failed to open content file: " + filePath);
    }
    return input;
}

} // namespace

AppContent ContentValidator::LoadFromFile(const std::string& filePath) const
{
    auto input = OpenFile(filePath);
    const nlohmann::json document = nlohmann::json::parse(input);
    return ParseDocument(document);
}

AppContent ContentValidator::ParseDocument(const nlohmann::json& document) const
{
    AppContent content;
    content.brandName = document.value(kBrandKey, "COLONY");

    ParseUserSection(document, content);
    ParseViewsSection(document, content);
    ParseChannelsSection(document, content);
    ParseHubSection(document, content);

    return content;
}

void ContentValidator::ParseHubSection(const nlohmann::json& document, AppContent& content) const
{
    content.hub = {};

    if (!document.contains(kHubKey))
    {
        return;
    }

    const auto& hubJson = document[kHubKey];
    if (!hubJson.is_object())
    {
        throw std::runtime_error("Content file field \"hub\" must be an object.");
    }

    auto requireString = [](const nlohmann::json& node, const char* field) -> std::string {
        if (!node.contains(field) || !node[field].is_string())
        {
            throw std::runtime_error(std::string{"Hub field \""} + field + "\" must be a string.");
        }
        const std::string value = node[field].get<std::string>();
        if (value.empty())
        {
            throw std::runtime_error(std::string{"Hub field \""} + field + "\" must not be empty.");
        }
        return value;
    };

    content.hub.headlineLocalizationKey = requireString(hubJson, "headlineKey");
    content.hub.descriptionLocalizationKey = requireString(hubJson, "descriptionKey");
    content.hub.primaryActionLocalizationKey = hubJson.value("primaryActionKey", "");
    content.hub.primaryActionDescriptionLocalizationKey = hubJson.value("primaryActionDescriptionKey", "");

    content.hub.highlightLocalizationKeys.clear();
    if (hubJson.contains("highlights"))
    {
        const auto& highlightsJson = hubJson["highlights"];
        if (!highlightsJson.is_array())
        {
            throw std::runtime_error("Hub highlights must be an array of strings.");
        }
        for (const auto& highlight : highlightsJson)
        {
            if (!highlight.is_string())
            {
                throw std::runtime_error("Each hub highlight entry must be a string.");
            }
            const std::string highlightKey = highlight.get<std::string>();
            if (!highlightKey.empty())
            {
                content.hub.highlightLocalizationKeys.push_back(highlightKey);
            }
        }
    }

    content.hub.widgets.clear();
    if (hubJson.contains("widgets"))
    {
        const auto& widgetsJson = hubJson["widgets"];
        if (!widgetsJson.is_array())
        {
            throw std::runtime_error("Hub widgets must be declared as an array.");
        }
        for (const auto& widgetJson : widgetsJson)
        {
            content.hub.widgets.emplace_back(ParseHubWidget(widgetJson));
        }
    }

    if (!hubJson.contains("branches"))
    {
        return;
    }

    const auto& branchesJson = hubJson["branches"];
    if (!branchesJson.is_array())
    {
        throw std::runtime_error("Hub branches must be declared as an array.");
    }

    for (const auto& branchJson : branchesJson)
    {
        content.hub.branches.emplace_back(ParseHubBranch(branchJson));
    }
}

HubBranch ContentValidator::ParseHubBranch(const nlohmann::json& json) const
{
    if (!json.is_object())
    {
        throw std::runtime_error("Each hub branch entry must be a JSON object.");
    }

    auto requireString = [](const nlohmann::json& node, const char* field) -> std::string {
        if (!node.contains(field) || !node[field].is_string())
        {
            throw std::runtime_error(std::string{"Hub branch field \""} + field + "\" must be a string.");
        }
        const std::string value = node[field].get<std::string>();
        if (value.empty())
        {
            throw std::runtime_error(std::string{"Hub branch field \""} + field + "\" must not be empty.");
        }
        return value;
    };

    HubBranch branch;
    branch.id = requireString(json, "id");
    branch.titleLocalizationKey = requireString(json, "titleKey");
    branch.descriptionLocalizationKey = requireString(json, "descriptionKey");
    branch.accentColor = json.value("accentColor", "");
    branch.channelId = json.value("channelId", "");
    branch.programId = json.value("programId", "");
    if (json.contains("tags"))
    {
        const auto& tagsJson = json["tags"];
        if (!tagsJson.is_array())
        {
            throw std::runtime_error("Hub branch tags must be provided as an array.");
        }
        for (const auto& tagJson : tagsJson)
        {
            if (!tagJson.is_string())
            {
                throw std::runtime_error("Each hub branch tag must be a string.");
            }
            const std::string tagKey = tagJson.get<std::string>();
            if (!tagKey.empty())
            {
                branch.tagLocalizationKeys.push_back(tagKey);
            }
        }
    }
    branch.actionLocalizationKey = json.value("actionKey", "");
    branch.metricsLocalizationKey = json.value("metricsKey", "");
    return branch;
}

HubWidget ContentValidator::ParseHubWidget(const nlohmann::json& json) const
{
    if (!json.is_object())
    {
        throw std::runtime_error("Each hub widget entry must be a JSON object.");
    }

    auto requireString = [](const nlohmann::json& node, const char* field) -> std::string {
        if (!node.contains(field) || !node[field].is_string())
        {
            throw std::runtime_error(std::string{"Hub widget field \""} + field + "\" must be a string.");
        }
        const std::string value = node[field].get<std::string>();
        if (value.empty())
        {
            throw std::runtime_error(std::string{"Hub widget field \""} + field + "\" must not be empty.");
        }
        return value;
    };

    HubWidget widget;
    widget.id = requireString(json, "id");
    widget.titleLocalizationKey = requireString(json, "titleKey");
    widget.descriptionLocalizationKey = requireString(json, "descriptionKey");
    widget.accentColor = json.value("accentColor", "");

    if (json.contains("items"))
    {
        const auto& itemsJson = json["items"];
        if (!itemsJson.is_array())
        {
            throw std::runtime_error("Hub widget items must be declared as an array.");
        }
        for (const auto& itemJson : itemsJson)
        {
            if (!itemJson.is_string())
            {
                throw std::runtime_error("Each hub widget item must be a string.");
            }
            const std::string itemKey = itemJson.get<std::string>();
            if (!itemKey.empty())
            {
                widget.itemLocalizationKeys.push_back(itemKey);
            }
        }
    }

    return widget;
}

void ContentValidator::ParseUserSection(const nlohmann::json& document, AppContent& content) const
{
    if (!document.contains(kUserKey))
    {
        return;
    }

    const auto& userJson = document[kUserKey];
    if (!userJson.is_object())
    {
        throw std::runtime_error("Content file field \"user\" must be an object.");
    }

    if (userJson.contains("name"))
    {
        if (!userJson["name"].is_string())
        {
            throw std::runtime_error("User name must be a string.");
        }
        content.user.name = userJson["name"].get<std::string>();
    }

    if (userJson.contains("status"))
    {
        if (!userJson["status"].is_string())
        {
            throw std::runtime_error("User status must be a string.");
        }
        content.user.status = userJson["status"].get<std::string>();
    }
}

void ContentValidator::ParseViewsSection(const nlohmann::json& document, AppContent& content) const
{
    if (!document.contains(kViewsKey) || !document[kViewsKey].is_object())
    {
        throw std::runtime_error("Content file missing \"views\" object.");
    }

    if (document[kViewsKey].empty())
    {
        throw std::runtime_error("Content file must declare at least one view.");
    }

    for (const auto& [id, value] : document[kViewsKey].items())
    {
        if (!value.is_object())
        {
            throw std::runtime_error("View \"" + id + "\" must be a JSON object.");
        }
        content.views.emplace(id, ParseViewContent(id, value));
    }
}

ViewContent ContentValidator::ParseViewContent(const std::string& viewId, const nlohmann::json& json) const
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

    ViewContent content;
    content.heading = json["heading"].get<std::string>();
    content.primaryActionLabel = json["primaryActionLabel"].get<std::string>();
    content.statusMessage = json.value("statusMessage", "");
    content.tagline = json.value("tagline", "");
    content.version = json.value("version", "");
    content.installState = json.value("installState", "");
    content.availability = json.value("availability", "");
    content.lastLaunched = json.value("lastLaunched", "");
    content.accentColor = json.value("accentColor", "#3B82F6");

    if (json.contains("heroGradient"))
    {
        if (!json["heroGradient"].is_array() || json["heroGradient"].size() != 2)
        {
            throw std::runtime_error("View \"" + viewId + "\" must declare heroGradient as an array of two hex colors.");
        }
        for (std::size_t i = 0; i < 2; ++i)
        {
            if (!json["heroGradient"][i].is_string())
            {
                throw std::runtime_error("View \"" + viewId + "\" heroGradient entries must be strings.");
            }
            content.heroGradient[i] = json["heroGradient"][i].get<std::string>();
        }
    }

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

    if (json.contains("heroHighlights"))
    {
        if (!json["heroHighlights"].is_array())
        {
            throw std::runtime_error("View \"" + viewId + "\" must declare heroHighlights as an array.");
        }
        for (const auto& highlight : json["heroHighlights"])
        {
            if (!highlight.is_string())
            {
                throw std::runtime_error("View \"" + viewId + "\" heroHighlights must contain only strings.");
            }
            content.heroHighlights.emplace_back(highlight.get<std::string>());
        }
    }

    if (json.contains("sections"))
    {
        if (!json["sections"].is_array())
        {
            throw std::runtime_error("View \"" + viewId + "\" must declare sections as an array.");
        }

        for (const auto& sectionJson : json["sections"])
        {
            if (!sectionJson.is_object())
            {
                throw std::runtime_error("View \"" + viewId + "\" has a section that is not an object.");
            }

            if (!sectionJson.contains("title") || !sectionJson["title"].is_string()
                || sectionJson["title"].get<std::string>().empty())
            {
                throw std::runtime_error("View \"" + viewId + "\" requires each section to declare a non-empty title.");
            }

            if (!sectionJson.contains("options") || !sectionJson["options"].is_array())
            {
                throw std::runtime_error(
                    "View \"" + viewId + "\" requires each section to declare an array of options.");
            }

            ViewSection section;
            section.title = sectionJson["title"].get<std::string>();

            for (const auto& optionJson : sectionJson["options"])
            {
                if (!optionJson.is_string())
                {
                    throw std::runtime_error("View \"" + viewId + "\" has a section option that is not a string.");
                }
                const std::string optionText = optionJson.get<std::string>();
                if (optionText.empty())
                {
                    continue;
                }
                section.options.emplace_back(optionText);
            }

            if (!section.options.empty())
            {
                content.sections.emplace_back(std::move(section));
            }
        }
    }

    return content;
}

void ContentValidator::ParseChannelsSection(const nlohmann::json& document, AppContent& content) const
{
    if (!document.contains(kChannelsKey) || !document[kChannelsKey].is_array())
    {
        throw std::runtime_error("Content file missing \"channels\" array.");
    }

    for (const auto& channelJson : document[kChannelsKey])
    {
        if (!channelJson.is_object())
        {
            throw std::runtime_error("Each channel entry must be an object.");
        }

        Channel channel;
        if (!channelJson.contains("id") || !channelJson["id"].is_string() || channelJson["id"].get<std::string>().empty())
        {
            throw std::runtime_error("Each channel must include a non-empty id.");
        }
        channel.id = channelJson["id"].get<std::string>();

        if (!channelJson.contains("label") || !channelJson["label"].is_string()
            || channelJson["label"].get<std::string>().empty())
        {
            throw std::runtime_error("Each channel must include a non-empty label.");
        }
        channel.label = channelJson["label"].get<std::string>();

        if (!channelJson.contains("programs") || !channelJson["programs"].is_array())
        {
            throw std::runtime_error("Channel \"" + channel.id + "\" requires a programs array.");
        }
        for (const auto& programJson : channelJson["programs"])
        {
            if (!programJson.is_string() || programJson.get<std::string>().empty())
            {
                throw std::runtime_error("Channel \"" + channel.id + "\" has an invalid program entry.");
            }
            channel.programs.emplace_back(programJson.get<std::string>());
        }

        if (channel.programs.empty())
        {
            throw std::runtime_error("Channel \"" + channel.id + "\" must declare at least one program id.");
        }

        content.channels.emplace_back(std::move(channel));
    }

    if (content.channels.empty())
    {
        throw std::runtime_error("Content file must declare at least one channel.");
    }

    for (const auto& channel : content.channels)
    {
        for (const auto& programId : channel.programs)
        {
            if (content.views.find(programId) == content.views.end())
            {
                throw std::runtime_error(
                    "Channel \"" + channel.id + "\" references unknown program id \"" + programId + "\".");
            }
        }
    }
}

AppContent LoadContentFromFile(const std::string& filePath)
{
    ContentValidator validator;
    return validator.LoadFromFile(filePath);
}

} // namespace colony
