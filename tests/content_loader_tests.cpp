#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "core/content_loader.hpp"

#include <filesystem>
#include <fstream>
#include <string_view>

namespace
{
std::filesystem::path WriteTempContent(std::string_view name, std::string_view json)
{
    const auto tempDir = std::filesystem::temp_directory_path();
    std::filesystem::path filePath = tempDir / name;
    std::ofstream output{filePath};
    REQUIRE(output.is_open());
    output << json;
    return filePath;
}
}

TEST_CASE("LoadContentFromFile parses minimal valid document")
{
    const auto path = WriteTempContent(
        "colony_valid.json",
        R"({
            "brand": "Test Colony",
            "channels": [
                {"id": "alpha", "label": "Alpha", "programs": ["PROGRAM"]}
            ],
            "views": {
                "PROGRAM": {
                    "heading": "Program Heading",
                    "primaryActionLabel": "Launch",
                    "paragraphs": ["Paragraph"],
                    "statusMessage": "Ready"
                }
            }
        })");

    auto content = colony::LoadContentFromFile(path.string());
    CHECK(content.brandName == "Test Colony");
    REQUIRE(content.channels.size() == 1);
    CHECK(content.channels[0].programs.front() == "PROGRAM");
    REQUIRE(content.views.contains("PROGRAM"));
    CHECK(content.views.at("PROGRAM").heading == "Program Heading");
}

TEST_CASE("LoadContentFromFile detects invalid view heading")
{
    const auto path = WriteTempContent(
        "colony_invalid_heading.json",
        R"({
            "brand": "Test Colony",
            "channels": [
                {"id": "alpha", "label": "Alpha", "programs": ["PROGRAM"]}
            ],
            "views": {
                "PROGRAM": {
                    "primaryActionLabel": "Launch",
                    "paragraphs": []
                }
            }
        })");

    CHECK_THROWS_WITH_AS(
        colony::LoadContentFromFile(path.string()),
        doctest::Contains("requires a non-empty heading"),
        std::runtime_error);
}
