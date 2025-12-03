#include "core/filesystem_discovery.hpp"

#include "doctest/doctest.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <random>
#include <string_view>

namespace
{
std::filesystem::path GenerateUniqueTempPath(std::string_view prefix)
{
    const auto tempDir = std::filesystem::temp_directory_path();
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<std::uint64_t> dist;

    for (int attempt = 0; attempt < 8; ++attempt)
    {
        std::filesystem::path candidate = tempDir / (std::string{prefix} + "-" + std::to_string(dist(gen)));
        if (!std::filesystem::exists(candidate))
        {
            return candidate;
        }
    }

    throw std::runtime_error("unable to find unique temp path");
}

std::filesystem::path WriteExecutableFile(const std::filesystem::path& path)
{
    std::ofstream output{path};
    REQUIRE(output.is_open());
    output << "echo run";
    output.close();

    std::filesystem::permissions(
        path,
        std::filesystem::perms::owner_exec | std::filesystem::perms::owner_write | std::filesystem::perms::owner_read,
        std::filesystem::perm_options::add);

    return path;
}
} // namespace

TEST_CASE("DiscoverChannelsFromFilesystem builds entries for folders")
{
    const auto root = GenerateUniqueTempPath("colony-fs-root");
    const auto appFolder = root / "Applications" / "alpha-mission";
    std::filesystem::create_directories(appFolder);
    WriteExecutableFile(appFolder / "launch.sh");

    const std::vector<colony::FolderChannelSpec> specs{{
        {"applications", "Applications", "Applications"},
        {"programs", "Programs", "Programs"},
    }};

    const auto channels = colony::DiscoverChannelsFromFilesystem(root, specs);
    REQUIRE(channels.size() == 1);
    CHECK(channels.front().id == "applications");
    CHECK(channels.front().programs.size() == 1);
    CHECK(channels.front().programs.front().programId == "APPLICATIONS_ALPHA_MISSION");
    CHECK_FALSE(channels.front().programs.front().launchTarget.empty());
    CHECK(channels.front().programs.front().view.heading == "Alpha Mission");
}

TEST_CASE("Python scripts are marked for interpreter dispatch")
{
    const auto root = GenerateUniqueTempPath("colony-fs-root");
    const auto gameFolder = root / "Games" / "nebula_trainer";
    std::filesystem::create_directories(gameFolder);

    std::ofstream script{gameFolder / "trainer.py"};
    REQUIRE(script.is_open());
    script << "print('hi')";
    script.close();

    const std::vector<colony::FolderChannelSpec> specs{{
        {"games", "Games", "Games"},
    }};

    const auto channels = colony::DiscoverChannelsFromFilesystem(root, specs);
    REQUIRE(channels.size() == 1);
    REQUIRE(channels.front().programs.size() == 1);
    CHECK(channels.front().programs.front().isPythonScript);
}

