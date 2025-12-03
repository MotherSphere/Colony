#include "core/filesystem_discovery.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <string_view>
#include <vector>

namespace colony
{
namespace
{
std::string SanitizeProgramId(std::string_view channelId, std::string_view folderName)
{
    std::string programId;
    programId.reserve(channelId.size() + folderName.size() + 1);

    auto appendUpper = [&programId](char ch) {
        programId.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    };

    for (char ch : channelId)
    {
        appendUpper(ch);
    }

    programId.push_back('_');

    bool previousUnderscore = false;
    for (char ch : folderName)
    {
        if (std::isalnum(static_cast<unsigned char>(ch)))
        {
            appendUpper(ch);
            previousUnderscore = false;
        }
        else
        {
            if (!previousUnderscore)
            {
                programId.push_back('_');
                previousUnderscore = true;
            }
        }
    }

    while (!programId.empty() && programId.back() == '_')
    {
        programId.pop_back();
    }

    return programId;
}

std::string MakeDisplayName(std::string name)
{
    std::replace(name.begin(), name.end(), '_', ' ');
    std::replace(name.begin(), name.end(), '-', ' ');

    bool capitalizeNext = true;
    for (char& ch : name)
    {
        if (std::isspace(static_cast<unsigned char>(ch)))
        {
            capitalizeNext = true;
            continue;
        }

        if (capitalizeNext)
        {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            capitalizeNext = false;
        }
    }

    return name;
}

bool HasExecutableBit(const std::filesystem::directory_entry& entry)
{
    std::error_code ec;
    const auto status = entry.status(ec);
    if (ec)
    {
        return false;
    }

    return (status.permissions() & std::filesystem::perms::owner_exec) != std::filesystem::perms::none
        || (status.permissions() & std::filesystem::perms::group_exec) != std::filesystem::perms::none
        || (status.permissions() & std::filesystem::perms::others_exec) != std::filesystem::perms::none;
}

bool IsExecutableFile(const std::filesystem::directory_entry& entry)
{
    if (!entry.is_regular_file())
    {
        return false;
    }

    const std::filesystem::path path = entry.path();
    const std::string extension = path.extension().string();

    static const std::vector<std::string> kKnownExecutableExtensions{
        ".exe", ".bat", ".cmd", ".sh", ".py", ".appimage"};

    if (HasExecutableBit(entry))
    {
        return true;
    }

    return std::find(kKnownExecutableExtensions.begin(), kKnownExecutableExtensions.end(), extension)
        != kKnownExecutableExtensions.end();
}

std::optional<std::filesystem::directory_entry> FindLaunchCandidate(const std::filesystem::path& folder)
{
    std::optional<std::filesystem::directory_entry> firstFile;

    for (const auto& entry : std::filesystem::directory_iterator(folder))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        if (!firstFile)
        {
            firstFile = entry;
        }

        if (IsExecutableFile(entry))
        {
            return entry;
        }
    }

    return firstFile;
}

ViewContent BuildViewFromFolder(
    const std::string& folderName,
    const std::filesystem::path& folderPath,
    const std::optional<std::filesystem::directory_entry>& launchCandidate)
{
    ViewContent view;
    view.heading = MakeDisplayName(folderName);
    view.tagline = "Auto-discovered program";
    view.primaryActionLabel = "Launch";
    view.statusMessage = "Select launch target";
    view.paragraphs.push_back("Folder: " + folderPath.string());

    if (launchCandidate.has_value())
    {
        view.sections.push_back(ViewSection{
            .title = "Launch targets",
            .options = {launchCandidate->path().filename().string()},
        });
    }

    return view;
}

DiscoveredProgram BuildProgramFromFolder(
    const std::string& channelId,
    const std::filesystem::directory_entry& folder)
{
    const auto launchCandidate = FindLaunchCandidate(folder.path());
    const std::string programId = SanitizeProgramId(channelId, folder.path().filename().string());

    DiscoveredProgram program;
    program.programId = programId;
    program.launchTarget = launchCandidate ? launchCandidate->path() : std::filesystem::path{};
    program.isPythonScript = launchCandidate && launchCandidate->path().extension() == ".py";
    program.view = BuildViewFromFolder(folder.path().filename().string(), folder.path(), launchCandidate);

    return program;
}

std::vector<DiscoveredProgram> DiscoverProgramsForChannel(
    const std::filesystem::path& contentRoot,
    const FolderChannelSpec& channel)
{
    std::vector<DiscoveredProgram> programs;

    const std::filesystem::path folderPath = contentRoot / channel.folderName;
    std::error_code ec;
    if (!std::filesystem::is_directory(folderPath, ec))
    {
        return programs;
    }

    for (const auto& entry : std::filesystem::directory_iterator(folderPath))
    {
        if (!entry.is_directory())
        {
            continue;
        }

        programs.emplace_back(BuildProgramFromFolder(channel.id, entry));
    }

    return programs;
}
} // namespace

std::vector<DiscoveredChannel> DiscoverChannelsFromFilesystem(
    const std::filesystem::path& contentRoot,
    const std::vector<FolderChannelSpec>& channelSpecs)
{
    std::vector<DiscoveredChannel> channels;
    channels.reserve(channelSpecs.size());

    for (const auto& spec : channelSpecs)
    {
        DiscoveredChannel channel;
        channel.id = spec.id;
        channel.label = spec.label;
        channel.programs = DiscoverProgramsForChannel(contentRoot, spec);

        if (!channel.programs.empty())
        {
            channels.push_back(std::move(channel));
        }
    }

    return channels;
}

} // namespace colony

