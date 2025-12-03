#pragma once

#include "core/content.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace colony
{

struct FolderChannelSpec
{
    std::string id;
    std::string label;
    std::string folderName;
};

struct DiscoveredProgram
{
    std::string programId;
    ViewContent view;
    std::filesystem::path launchTarget;
    bool isPythonScript = false;
};

struct DiscoveredChannel
{
    std::string id;
    std::string label;
    std::vector<DiscoveredProgram> programs;
};

std::vector<DiscoveredChannel> DiscoverChannelsFromFilesystem(
    const std::filesystem::path& contentRoot,
    const std::vector<FolderChannelSpec>& channelSpecs);

} // namespace colony

