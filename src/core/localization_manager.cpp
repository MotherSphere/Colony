#include "core/localization_manager.hpp"

#include "json.hpp"

#include <array>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

namespace colony
{
namespace
{
std::string_view TrimView(std::string_view view)
{
    const auto begin = view.find_first_not_of(" \t\r");
    if (begin == std::string_view::npos)
    {
        return {};
    }
    const auto end = view.find_last_not_of(" \t\r");
    return view.substr(begin, end - begin + 1);
}

std::string TrimCopy(std::string_view view)
{
    return std::string{TrimView(view)};
}

void FlattenJsonNode(const nlohmann::json& node, const std::string& prefix, LocalizationManager::StringsMap& outStrings)
{
    if (node.is_object())
    {
        for (auto it = node.begin(); it != node.end(); ++it)
        {
            std::string nextPrefix = prefix.empty() ? it.key() : prefix + "." + it.key();
            FlattenJsonNode(it.value(), nextPrefix, outStrings);
        }
        return;
    }

    if (node.is_array())
    {
        for (std::size_t index = 0; index < node.size(); ++index)
        {
            std::string nextPrefix = prefix + "[" + std::to_string(index) + "]";
            FlattenJsonNode(node[index], nextPrefix, outStrings);
        }
        return;
    }

    if (!prefix.empty())
    {
        if (node.is_string())
        {
            outStrings[prefix] = node.get<std::string>();
        }
        else if (node.is_boolean())
        {
            outStrings[prefix] = node.get<bool>() ? "true" : "false";
        }
        else if (node.is_number())
        {
            outStrings[prefix] = node.dump();
        }
        else if (node.is_null())
        {
            outStrings[prefix] = {};
        }
    }
}

std::string StripYamlValueQuotes(std::string value)
{
    if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\'')))
    {
        value = value.substr(1, value.size() - 2);
    }
    return value;
}

} // namespace

void LocalizationManager::SetResourceDirectory(std::filesystem::path directory)
{
    resourceDirectory_ = std::move(directory);
    activeStrings_.clear();
    fallbackStrings_.clear();
    activeLanguageId_.clear();
}

void LocalizationManager::SetFallbackLanguage(std::string languageId)
{
    fallbackLanguageId_ = std::move(languageId);
    fallbackStrings_.clear();
}

bool LocalizationManager::LoadLanguage(const std::string& languageId)
{
    if (resourceDirectory_.empty())
    {
        return false;
    }

    if (!EnsureFallbackLoaded() && languageId != fallbackLanguageId_)
    {
        std::cerr << "Failed to load fallback language resources from " << resourceDirectory_ << '\n';
        return false;
    }

    LocalizationManager::StringsMap newStrings;
    const std::filesystem::path filePath = ResolveLanguageFile(languageId);
    if (!LoadFromFile(filePath, newStrings))
    {
        std::cerr << "Failed to load localization file: " << filePath << '\n';
        return false;
    }

    activeLanguageId_ = languageId;
    activeStrings_ = std::move(newStrings);
    return true;
}

std::string LocalizationManager::GetString(std::string_view key) const
{
    if (key.empty())
    {
        return {};
    }

    const std::string keyStr{key};
    if (const auto it = activeStrings_.find(keyStr); it != activeStrings_.end())
    {
        return it->second;
    }

    if (const auto fallbackIt = fallbackStrings_.find(keyStr); fallbackIt != fallbackStrings_.end())
    {
        return fallbackIt->second;
    }

    return keyStr;
}

std::string LocalizationManager::GetStringOrDefault(std::string_view key, std::string_view fallback) const
{
    if (key.empty())
    {
        return std::string{fallback};
    }

    const std::string keyStr{key};
    if (const auto it = activeStrings_.find(keyStr); it != activeStrings_.end())
    {
        return it->second;
    }

    if (const auto fallbackIt = fallbackStrings_.find(keyStr); fallbackIt != fallbackStrings_.end())
    {
        return fallbackIt->second;
    }

    return std::string{fallback};
}

std::filesystem::path LocalizationManager::ResolveLanguageFile(const std::string& languageId) const
{
    static constexpr std::array<const char*, 3> kExtensions{".json", ".yaml", ".yml"};
    for (const char* extension : kExtensions)
    {
        std::filesystem::path candidate = resourceDirectory_ / (languageId + extension);
        std::error_code error;
        if (!candidate.empty() && std::filesystem::exists(candidate, error))
        {
            return candidate;
        }
    }
    return resourceDirectory_ / (languageId + ".json");
}

bool LocalizationManager::LoadFromFile(const std::filesystem::path& path, StringsMap& outStrings) const
{
    std::error_code error;
    if (!std::filesystem::exists(path, error))
    {
        return false;
    }

    std::ifstream stream{path};
    if (!stream.is_open())
    {
        return false;
    }

    const std::string extension = path.extension().string();
    if (extension == ".json")
    {
        return LoadJson(stream, outStrings);
    }
    if (extension == ".yaml" || extension == ".yml")
    {
        return LoadYaml(stream, outStrings);
    }

    // Attempt JSON parsing by default.
    stream.clear();
    stream.seekg(0);
    return LoadJson(stream, outStrings);
}

bool LocalizationManager::LoadJson(std::istream& stream, StringsMap& outStrings) const
{
    try
    {
        nlohmann::json document = nlohmann::json::parse(stream, nullptr, true, true);
        FlattenJsonNode(document, "", outStrings);
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool LocalizationManager::LoadYaml(std::istream& stream, StringsMap& outStrings) const
{
    struct Frame
    {
        int indent;
        std::string prefix;
    };

    std::vector<Frame> stack;
    stack.push_back(Frame{-1, {}});

    std::string line;
    while (std::getline(stream, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        std::string_view view{line};
        std::string_view trimmed = TrimView(view);
        if (trimmed.empty() || trimmed.front() == '#')
        {
            continue;
        }

        int indent = 0;
        while (indent < static_cast<int>(view.size()) && view[indent] == ' ')
        {
            ++indent;
        }

        const auto colonPos = trimmed.find(':');
        if (colonPos == std::string_view::npos)
        {
            continue;
        }

        std::string key = TrimCopy(trimmed.substr(0, colonPos));
        std::string_view rawValue = trimmed.substr(colonPos + 1);
        rawValue = TrimView(rawValue);

        const auto commentPos = rawValue.find('#');
        if (commentPos != std::string_view::npos)
        {
            rawValue = TrimView(rawValue.substr(0, commentPos));
        }

        while (!stack.empty() && indent <= stack.back().indent)
        {
            stack.pop_back();
        }

        if (stack.empty())
        {
            stack.push_back(Frame{-1, {}});
        }

        std::string fullKey = stack.back().prefix.empty() ? key : stack.back().prefix + "." + key;
        if (rawValue.empty())
        {
            stack.push_back(Frame{indent, std::move(fullKey)});
            continue;
        }

        std::string value = StripYamlValueQuotes(std::string{rawValue});
        outStrings.emplace(std::move(fullKey), std::move(value));
    }

    return true;
}

bool LocalizationManager::EnsureFallbackLoaded()
{
    if (fallbackLanguageId_.empty())
    {
        fallbackStrings_.clear();
        return true;
    }

    if (!fallbackStrings_.empty())
    {
        return true;
    }

    if (resourceDirectory_.empty())
    {
        return false;
    }

    LocalizationManager::StringsMap fallbackStrings;
    const std::filesystem::path fallbackPath = ResolveLanguageFile(fallbackLanguageId_);
    if (!LoadFromFile(fallbackPath, fallbackStrings))
    {
        std::cerr << "Failed to load fallback localization file: " << fallbackPath << '\n';
        return false;
    }

    fallbackStrings_ = std::move(fallbackStrings);
    return true;
}

} // namespace colony
