#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace colony
{

struct ViewContent
{
    std::string heading;
    std::vector<std::string> paragraphs;
    std::string primaryActionLabel;
    std::string statusMessage;
};

struct AppContent
{
    std::string brandName;
    std::vector<std::string> navigation;
    std::unordered_map<std::string, ViewContent> views;
};

AppContent LoadContentFromFile(const std::string& filePath);

} // namespace colony
