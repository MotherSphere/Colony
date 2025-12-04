#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "core/content_loader.hpp"
#include "core/localization_manager.hpp"
#define private public
#include "app/application.h"
#undef private
#include "utils/color.hpp"
#include "utils/font_manager.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <SDL2/SDL.h>

namespace
{
std::filesystem::path GenerateUniqueTempPath(std::string_view prefix)
{
    const auto tempDir = std::filesystem::temp_directory_path();
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<std::uint64_t> dist;

    for (int attempt = 0; attempt < 16; ++attempt)
    {
        std::ostringstream builder;
        builder << prefix << '-' << std::hex << dist(gen);
        auto candidate = tempDir / builder.str();
        if (!std::filesystem::exists(candidate))
        {
            return candidate;
        }
    }

    throw std::runtime_error("Failed to generate unique temporary path");
}

std::filesystem::path WriteTempContent(std::string_view name, std::string_view json)
{
    const auto tempDir = std::filesystem::temp_directory_path();
    std::filesystem::path filePath = tempDir / name;
    std::ofstream output{filePath};
    REQUIRE(output.is_open());
    output << json;
    return filePath;
}

std::string BuildDocument(std::string_view viewSection, std::string_view channelsSection, std::string_view extra = "")
{
    std::string document = "{\n    \"brand\": \"Test Colony\"";
    if (!extra.empty())
    {
        document.append(",\n    ");
        document.append(extra);
    }
    document.append(",\n    \"views\": ");
    document.append(viewSection);
    document.append(",\n    \"channels\": ");
    document.append(channelsSection);
    document.append("\n}");
    return document;
}

const char* kValidViewSection = R"({
    "PROGRAM": {
        "heading": "Program Heading",
        "primaryActionLabel": "Launch",
        "paragraphs": ["Paragraph"],
        "sections": [
            {
                "title": "Section",
                "options": ["Option"]
            }
        ]
    }
})";

const char* kValidChannelsSection = R"([
    {"id": "alpha", "label": "Alpha", "programs": ["PROGRAM"]}
])";

std::filesystem::path ResolveDefaultContentPath()
{
    const std::filesystem::path relative{"assets/content/app_content.json"};

    auto search = [&](std::filesystem::path start) -> std::optional<std::filesystem::path> {
        for (int depth = 0; depth < 8 && !start.empty(); ++depth)
        {
            const auto candidate = start / relative;
            if (std::filesystem::exists(candidate))
            {
                return std::filesystem::weakly_canonical(candidate);
            }
            if (!start.has_parent_path())
            {
                break;
            }
            start = start.parent_path();
        }
        return std::nullopt;
    };

    if (auto found = search(std::filesystem::current_path()))
    {
        return *found;
    }

    std::filesystem::path sourceRoot = std::filesystem::path(__FILE__).parent_path();
    if (!sourceRoot.is_absolute())
    {
        sourceRoot = std::filesystem::current_path() / sourceRoot;
    }
    sourceRoot = std::filesystem::weakly_canonical(sourceRoot);
    if (auto found = search(sourceRoot))
    {
        return *found;
    }

    throw std::runtime_error("Unable to locate assets/content/app_content.json");
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

TEST_CASE("Application PointInRect honors exclusive bounds and dimensions")
{
    colony::Application app;
    SDL_Rect rect{10, 20, 5, 6};

    CHECK(app.PointInRect(rect, 10, 20));
    CHECK(app.PointInRect(rect, 14, 25));
    CHECK_FALSE(app.PointInRect(rect, 15, 25));
    CHECK_FALSE(app.PointInRect(rect, 10, 26));

    SDL_Rect emptyWidth{0, 0, 0, 10};
    CHECK_FALSE(app.PointInRect(emptyWidth, 0, 0));

    SDL_Rect emptyHeight{0, 0, 5, 0};
    CHECK_FALSE(app.PointInRect(emptyHeight, 0, 0));
}

TEST_CASE("Default content defines navigation channels for programs, addons, and games")
{
    const auto appContentPath = ResolveDefaultContentPath();
    auto content = colony::LoadContentFromFile(appContentPath.string());

    auto requireChannel = [&](
                               std::string_view id,
                               std::string_view expectedLabel,
                               const std::vector<std::string>& expectedPrograms) {
        INFO("checking channel: " << id);
        const auto it = std::find_if(
            content.channels.begin(),
            content.channels.end(),
            [&](const colony::Channel& channel) { return channel.id == id; });
        REQUIRE_MESSAGE(it != content.channels.end(), "Missing channel for " << id);
        CHECK(it->label == expectedLabel);
        REQUIRE_FALSE_MESSAGE(it->programs.empty(), "Channel " << id << " must list at least one program");

        if (!expectedPrograms.empty())
        {
            CHECK_MESSAGE(
                it->programs.size() == expectedPrograms.size(),
                "Channel " << id << " should expose " << expectedPrograms.size() << " programs");
            for (const auto& expectedProgram : expectedPrograms)
            {
                CHECK_MESSAGE(
                    std::find(it->programs.begin(), it->programs.end(), expectedProgram) != it->programs.end(),
                    "Channel " << id << " is missing expected program " << expectedProgram);
            }
        }

        for (const auto& programId : it->programs)
        {
            INFO("verifying view for program: " << programId);
            CHECK_MESSAGE(
                content.views.contains(programId),
                "Missing view for program " << programId);
        }
    };

    requireChannel(
        "programs",
        "Programs",
        {"PROGRAMS_CORE_SUITE", "PROGRAMS_SIGNAL_MATRIX", "PROGRAMS_AUTOMATION_DIRECTOR"});
    requireChannel(
        "addons",
        "Addons",
        {"ADDONS_EXTENSION_BAY", "ADDONS_REACTIVE_SHIELDING", "ADDONS_SYNAPSE_BRIDGE"});
    requireChannel(
        "games",
        "Games",
        {"GAMES_SIMULATION_DECK", "GAMES_TACTICAL_BRIEFING", "GAMES_STARFORGE_TRAINER"});
}

TEST_CASE("RenderVerticalGradient draws within bounds")
{
    REQUIRE(SDL_Init(SDL_INIT_VIDEO) == 0);

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 4, 4, 32, SDL_PIXELFORMAT_RGBA32);
    REQUIRE(surface != nullptr);

    SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
    REQUIRE(renderer != nullptr);

    auto clearSurface = [&]() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);
    };

    clearSurface();

    const SDL_Rect area{1, 0, 2, 4};
    colony::color::RenderVerticalGradient(renderer, area, SDL_Color{255, 0, 0, 255}, SDL_Color{0, 0, 255, 255});
    SDL_RenderPresent(renderer);

    auto getPixel = [&](int x, int y) {
        Uint32* pixels = static_cast<Uint32*>(surface->pixels);
        const int pitch = surface->pitch / static_cast<int>(sizeof(Uint32));
        return pixels[y * pitch + x];
    };

    const Uint32 clearedPixel = SDL_MapRGBA(surface->format, 0, 0, 0, SDL_ALPHA_OPAQUE);
    const Uint32 insidePixel = getPixel(area.x, area.y);
    const Uint32 outsidePixel = getPixel(area.x + area.w, area.y);

    CHECK(insidePixel != outsidePixel);
    CHECK(outsidePixel == clearedPixel);

    clearSurface();
    const SDL_Rect zeroWidthArea{0, 0, 0, 4};
    colony::color::RenderVerticalGradient(renderer, zeroWidthArea, SDL_Color{255, 255, 255, 255}, SDL_Color{0, 0, 0, 255});
    SDL_RenderPresent(renderer);
    CHECK(getPixel(0, 0) == clearedPixel);

    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(surface);
    SDL_Quit();
}

TEST_CASE("LoadContentFromFile validates user section")
{
    SUBCASE("user field must be an object")
    {
        const auto path = WriteTempContent(
            "colony_user_not_object.json",
            BuildDocument(kValidViewSection, kValidChannelsSection, "\"user\": 123"));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("Content file field \"user\" must be an object."),
            std::runtime_error);
    }

    SUBCASE("user name must be a string")
    {
        const auto path = WriteTempContent(
            "colony_user_name_not_string.json",
            BuildDocument(kValidViewSection, kValidChannelsSection, R"("user": {"name": 42})"));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("User name must be a string."),
            std::runtime_error);
    }

    SUBCASE("user status must be a string")
    {
        const auto path = WriteTempContent(
            "colony_user_status_not_string.json",
            BuildDocument(kValidViewSection, kValidChannelsSection, R"("user": {"name": "Ada", "status": []})"));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("User status must be a string."),
            std::runtime_error);
    }
}

TEST_CASE("LocalizationManager loads translations with fallback and YAML support")
{
    const std::filesystem::path tempRoot = GenerateUniqueTempPath("colony_localization_test");
    REQUIRE(std::filesystem::create_directories(tempRoot));

    auto writeFile = [&](std::string_view name, std::string_view contents) {
        std::ofstream output{tempRoot / std::string{name}};
        REQUIRE(output.is_open());
        output << contents;
    };

    writeFile("en.json", R"({
        "messages": {
            "greeting": "Hello",
            "farewell": "Goodbye"
        }
    })");

    writeFile("fr.json", R"({
        "messages": {
            "greeting": "Bonjour"
        }
    })");

    writeFile("es.yaml", R"(messages:
  greeting: Hola
)");

    colony::LocalizationManager manager;
    manager.SetResourceDirectory(tempRoot);
    manager.SetFallbackLanguage("en");

    REQUIRE(manager.LoadLanguage("en"));
    CHECK(manager.GetString("messages.greeting") == "Hello");

    REQUIRE(manager.LoadLanguage("fr"));
    CHECK(manager.GetString("messages.greeting") == "Bonjour");
    CHECK(manager.GetString("messages.farewell") == "Goodbye");

    REQUIRE(manager.LoadLanguage("es"));
    CHECK(manager.GetString("messages.greeting") == "Hola");
    CHECK(manager.GetStringOrDefault("messages.unknown", "Default") == "Default");

    std::filesystem::remove_all(tempRoot);
}

TEST_CASE("LoadContentFromFile validates view sections")
{
    SUBCASE("views object must not be empty")
    {
        const auto path = WriteTempContent(
            "colony_views_empty.json",
            BuildDocument("{}", kValidChannelsSection));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("Content file must declare at least one view."),
            std::runtime_error);
    }

    SUBCASE("view value must be an object")
    {
        const auto path = WriteTempContent(
            "colony_view_not_object.json",
            BuildDocument(R"({"PROGRAM": []})", kValidChannelsSection));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("View \"PROGRAM\" must be a JSON object."),
            std::runtime_error);
    }

    SUBCASE("heroGradient must be two color strings")
    {
        const auto path = WriteTempContent(
            "colony_view_bad_gradient.json",
            BuildDocument(
                R"({
    "PROGRAM": {
        "heading": "Program Heading",
        "primaryActionLabel": "Launch",
        "heroGradient": ["#fff"]
    }
})",
                kValidChannelsSection));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("must declare heroGradient as an array of two hex colors."),
            std::runtime_error);
    }

    SUBCASE("heroGradient entries must be strings")
    {
        const auto path = WriteTempContent(
            "colony_view_gradient_not_strings.json",
            BuildDocument(
                R"({
    "PROGRAM": {
        "heading": "Program Heading",
        "primaryActionLabel": "Launch",
        "heroGradient": ["#fff", 7]
    }
})",
                kValidChannelsSection));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("heroGradient entries must be strings."),
            std::runtime_error);
    }

    SUBCASE("paragraphs must be string array")
    {
        const auto path = WriteTempContent(
            "colony_view_paragraphs_invalid.json",
            BuildDocument(
                R"({
    "PROGRAM": {
        "heading": "Program Heading",
        "primaryActionLabel": "Launch",
        "paragraphs": "invalid"
    }
})",
                kValidChannelsSection));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("must declare paragraphs as an array."),
            std::runtime_error);
    }

    SUBCASE("paragraph entries must be strings")
    {
        const auto path = WriteTempContent(
            "colony_view_paragraph_entry_invalid.json",
            BuildDocument(
                R"({
    "PROGRAM": {
        "heading": "Program Heading",
        "primaryActionLabel": "Launch",
        "paragraphs": ["valid", 3]
    }
})",
                kValidChannelsSection));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("contains a non-string paragraph entry."),
            std::runtime_error);
    }

    SUBCASE("heroHighlights must be array of strings")
    {
        const auto path = WriteTempContent(
            "colony_view_highlights_invalid.json",
            BuildDocument(
                R"({
    "PROGRAM": {
        "heading": "Program Heading",
        "primaryActionLabel": "Launch",
        "heroHighlights": "invalid"
    }
})",
                kValidChannelsSection));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("must declare heroHighlights as an array."),
            std::runtime_error);
    }

    SUBCASE("heroHighlights entries must be strings")
    {
        const auto path = WriteTempContent(
            "colony_view_highlights_entry_invalid.json",
            BuildDocument(
                R"({
    "PROGRAM": {
        "heading": "Program Heading",
        "primaryActionLabel": "Launch",
        "heroHighlights": ["one", 2]
    }
})",
                kValidChannelsSection));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("heroHighlights must contain only strings."),
            std::runtime_error);
    }

    SUBCASE("sections must be array")
    {
        const auto path = WriteTempContent(
            "colony_view_sections_invalid.json",
            BuildDocument(
                R"({
    "PROGRAM": {
        "heading": "Program Heading",
        "primaryActionLabel": "Launch",
        "sections": {}
    }
})",
                kValidChannelsSection));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("must declare sections as an array."),
            std::runtime_error);
    }

    SUBCASE("section entries must be objects")
    {
        const auto path = WriteTempContent(
            "colony_view_section_entry_invalid.json",
            BuildDocument(
                R"({
    "PROGRAM": {
        "heading": "Program Heading",
        "primaryActionLabel": "Launch",
        "sections": ["invalid"]
    }
})",
                kValidChannelsSection));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("has a section that is not an object."),
            std::runtime_error);
    }

    SUBCASE("section must declare non-empty title")
    {
        const auto path = WriteTempContent(
            "colony_view_section_title_invalid.json",
            BuildDocument(
                R"({
    "PROGRAM": {
        "heading": "Program Heading",
        "primaryActionLabel": "Launch",
        "sections": [
            {"title": "", "options": ["one"]}
        ]
    }
})",
                kValidChannelsSection));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("requires each section to declare a non-empty title."),
            std::runtime_error);
    }

    SUBCASE("section must declare options array")
    {
        const auto path = WriteTempContent(
            "colony_view_section_options_missing.json",
            BuildDocument(
                R"({
    "PROGRAM": {
        "heading": "Program Heading",
        "primaryActionLabel": "Launch",
        "sections": [
            {"title": "Section", "options": "invalid"}
        ]
    }
})",
                kValidChannelsSection));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("requires each section to declare an array of options."),
            std::runtime_error);
    }

    SUBCASE("section options must be strings")
    {
        const auto path = WriteTempContent(
            "colony_view_section_option_invalid.json",
            BuildDocument(
                R"({
    "PROGRAM": {
        "heading": "Program Heading",
        "primaryActionLabel": "Launch",
        "sections": [
            {"title": "Section", "options": [1]}
        ]
    }
})",
                kValidChannelsSection));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("has a section option that is not a string."),
            std::runtime_error);
    }
}

TEST_CASE("LoadContentFromFile validates channels")
{
    SUBCASE("channels array required")
    {
        const auto path = WriteTempContent(
            "colony_channels_missing.json",
            BuildDocument(kValidViewSection, "null"));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("Content file missing \"channels\" array."),
            std::runtime_error);
    }

    SUBCASE("channel entry must be object")
    {
        const auto path = WriteTempContent(
            "colony_channel_entry_invalid.json",
            BuildDocument(kValidViewSection, R"(["invalid"])"));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("Each channel entry must be an object."),
            std::runtime_error);
    }

    SUBCASE("channel must include non-empty id")
    {
        const auto path = WriteTempContent(
            "colony_channel_id_missing.json",
            BuildDocument(kValidViewSection, R"([
    {"label": "Alpha", "programs": ["PROGRAM"]}
])"));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("Each channel must include a non-empty id."),
            std::runtime_error);
    }

    SUBCASE("channel must include non-empty label")
    {
        const auto path = WriteTempContent(
            "colony_channel_label_missing.json",
            BuildDocument(kValidViewSection, R"([
    {"id": "alpha", "programs": ["PROGRAM"]}
])"));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("Each channel must include a non-empty label."),
            std::runtime_error);
    }

    SUBCASE("channel programs must be array")
    {
        const auto path = WriteTempContent(
            "colony_channel_programs_missing.json",
            BuildDocument(kValidViewSection, R"([
    {"id": "alpha", "label": "Alpha", "programs": "invalid"}
])"));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("requires a programs array."),
            std::runtime_error);
    }

    SUBCASE("channel program entries must be non-empty strings")
    {
        const auto path = WriteTempContent(
            "colony_channel_program_invalid.json",
            BuildDocument(kValidViewSection, R"([
    {"id": "alpha", "label": "Alpha", "programs": [""]}
])"));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("has an invalid program entry."),
            std::runtime_error);
    }

    SUBCASE("channel must declare at least one program")
    {
        const auto path = WriteTempContent(
            "colony_channel_programs_empty.json",
            BuildDocument(kValidViewSection, R"([
    {"id": "alpha", "label": "Alpha", "programs": []}
])"));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("must declare at least one program id."),
            std::runtime_error);
    }

    SUBCASE("must declare at least one channel")
    {
        const auto path = WriteTempContent(
            "colony_channels_empty.json",
            BuildDocument(kValidViewSection, "[]"));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("Content file must declare at least one channel."),
            std::runtime_error);
    }

    SUBCASE("channels may not reference unknown program ids")
    {
        const auto path = WriteTempContent(
            "colony_channel_unknown_program.json",
            BuildDocument(
                R"({
    "PROGRAM": {
        "heading": "Program Heading",
        "primaryActionLabel": "Launch"
    }
})",
                R"([
    {"id": "alpha", "label": "Alpha", "programs": ["UNKNOWN"]}
])"));

        CHECK_THROWS_WITH_AS(
            colony::LoadContentFromFile(path.string()),
            doctest::Contains("references unknown program id"),
            std::runtime_error);
    }
}

TEST_CASE("ResolveBundledFont finds nested font beside executable")
{
    std::filesystem::path basePath;
    if (char* rawBasePath = SDL_GetBasePath(); rawBasePath != nullptr)
    {
        basePath = std::filesystem::path{rawBasePath};
        SDL_free(rawBasePath);
    }

    REQUIRE_MESSAGE(!basePath.empty(), "SDL_GetBasePath should provide an executable directory");

    const std::filesystem::path sourceFont{
        "assets/fonts/NotoSansArabic/NotoSansArabic-Regular.ttf"};
    REQUIRE(std::filesystem::exists(sourceFont));

    const std::filesystem::path targetRoot = basePath / "assets/fonts/NotoSansArabic";
    std::error_code error;
    std::filesystem::create_directories(targetRoot, error);
    REQUIRE_FALSE_MESSAGE(error, error.message());

    const std::filesystem::path targetFont = targetRoot / "NotoSansArabic-Regular.ttf";
    std::filesystem::copy_file(
        sourceFont,
        targetFont,
        std::filesystem::copy_options::overwrite_existing,
        error);
    REQUIRE_FALSE_MESSAGE(error, error.message());

    const std::filesystem::path resolved =
        colony::fonts::ResolveBundledFont("NotoSansArabic/NotoSansArabic-Regular.ttf");

    CHECK(resolved == targetFont);
    CHECK(std::filesystem::exists(resolved));

    std::filesystem::remove(targetFont, error);
    std::filesystem::remove(targetRoot, error);
    std::filesystem::remove(targetRoot.parent_path(), error);
    std::filesystem::remove(targetRoot.parent_path().parent_path(), error);
}
