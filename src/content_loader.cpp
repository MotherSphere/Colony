#include "content_loader.hpp"

#include "json.hpp"

#include <fstream>
#include <stdexcept>
#include <utility>

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
                throw std::runtime_error("View \"" + viewId"
                    + "\" requires each section to declare an array of options.");
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

    if (document.contains("user"))
    {
        const auto& userJson = document["user"];
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

    if (!document.contains("channels") || !document["channels"].is_array())
    {
        throw std::runtime_error("Content file missing \"channels\" array.");
    }

    for (const auto& channelJson : document["channels"])
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
                throw std::runtime_error("Channel \"" + channel.id + "\" references unknown program id \"" + programId + "\".");
            }
        }
    }

    return content;
}

} // namespace colony
