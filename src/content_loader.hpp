#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <array>

namespace colony
{

struct ViewSection
{
    std::string title;
    std::vector<std::string> options;
};

struct ViewContent
{
    std::string heading;
    std::string tagline;
    std::vector<std::string> paragraphs;
    std::vector<ViewSection> sections;
    std::vector<std::string> heroHighlights;
    std::array<std::string, 2> heroGradient{"#17233b", "#0b111d"};
    std::string primaryActionLabel;
    std::string statusMessage;
    std::string version;
    std::string installState;
    std::string availability;
    std::string lastLaunched;
    std::string accentColor{"#3B82F6"};
};

struct Channel
{
    std::string id;
    std::string label;
    std::vector<std::string> programs;
};

struct UserProfile
{
    std::string name{"Operator"};
    std::string status{"Offline"};
};

struct AppContent
{
    std::string brandName;
    UserProfile user;
    std::vector<Channel> channels;
    std::unordered_map<std::string, ViewContent> views;
};

AppContent LoadContentFromFile(const std::string& filePath);

} // namespace colony
