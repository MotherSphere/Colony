#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

namespace colony
{

struct ViewSection
{
    std::string title;
    std::vector<std::string> options;
};

struct ChartDatum
{
    std::string label;
    double value = 0.0;
};

struct MediaItem
{
    std::string title;
    std::string description;
};

struct ViewContent
{
    std::string type{"text"};
    std::string heading;
    std::string tagline;
    std::vector<std::string> paragraphs;
    std::vector<ViewSection> sections;
    std::vector<std::string> heroHighlights;
    std::vector<ChartDatum> chartData;
    std::vector<MediaItem> mediaItems;
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

} // namespace colony
