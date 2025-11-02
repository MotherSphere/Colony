#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "content_loader.hpp"
#include "utils/sdl_wrappers.hpp"
#include "utils/text.hpp"
#include "utils/text_wrapping.hpp"

namespace
{

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 768;
constexpr SDL_Color kBackgroundColor{10, 14, 22, SDL_ALPHA_OPAQUE};
constexpr SDL_Color kNavRailColor{8, 10, 16, SDL_ALPHA_OPAQUE};
constexpr SDL_Color kLibraryBackground{18, 25, 37, SDL_ALPHA_OPAQUE};
constexpr SDL_Color kLibraryCardColor{26, 36, 53, SDL_ALPHA_OPAQUE};
constexpr SDL_Color kLibraryCardHover{36, 49, 68, SDL_ALPHA_OPAQUE};
constexpr SDL_Color kLibraryCardActive{44, 62, 88, SDL_ALPHA_OPAQUE};
constexpr SDL_Color kNavTextColor{168, 182, 205, SDL_ALPHA_OPAQUE};
constexpr SDL_Color kHeroTitleColor{235, 242, 255, SDL_ALPHA_OPAQUE};
constexpr SDL_Color kHeroBodyColor{190, 202, 222, SDL_ALPHA_OPAQUE};
constexpr SDL_Color kMutedColor{132, 146, 170, SDL_ALPHA_OPAQUE};
constexpr SDL_Color kBorderColor{42, 56, 80, SDL_ALPHA_OPAQUE};
constexpr SDL_Color kStatusBarColor{11, 16, 25, SDL_ALPHA_OPAQUE};
constexpr SDL_Color kStatusBarTextColor{164, 180, 203, SDL_ALPHA_OPAQUE};
constexpr SDL_Color kChannelBadgeColor{40, 54, 78, SDL_ALPHA_OPAQUE};
constexpr char kFontFileName[] = "DejaVuSans.ttf";
constexpr char kBundledFontDirectory[] = "assets/fonts";
constexpr char kFontDownloadUrl[] =
    "https://raw.githubusercontent.com/dejavu-fonts/dejavu-fonts/master/ttf/DejaVuSans.ttf";
constexpr char kContentFile[] = "assets/content/app_content.json";

const std::array<std::filesystem::path, 3> kSystemFontCandidates{
    std::filesystem::path{"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"},
    std::filesystem::path{"/usr/local/share/fonts/DejaVuSans.ttf"},
    std::filesystem::path{"/Library/Fonts/DejaVuSans.ttf"}};

std::filesystem::path GetBundledFontPath()
{
    return std::filesystem::path{kBundledFontDirectory} / kFontFileName;
}

bool CopyFontIfPresent(const std::filesystem::path& source, const std::filesystem::path& destination)
{
    std::error_code error;
    if (!std::filesystem::exists(source, error))
    {
        return false;
    }

    std::filesystem::create_directories(destination.parent_path(), error);
    if (error)
    {
        return false;
    }

    std::filesystem::copy_file(
        source,
        destination,
        std::filesystem::copy_options::overwrite_existing,
        error);

    return !error;
}

bool IsCommandAvailable(std::string_view command)
{
#ifdef _WIN32
    std::string checkCommand{"where "};
#else
    std::string checkCommand{"command -v "};
#endif
    checkCommand.append(command);
#ifdef _WIN32
    checkCommand.append(" >nul 2>&1");
#else
    checkCommand.append(" >/dev/null 2>&1");
#endif
    return std::system(checkCommand.c_str()) == 0;
}

bool DownloadFont(const std::filesystem::path& destination)
{
    if (!IsCommandAvailable("curl"))
    {
        return false;
    }

    std::error_code error;
    std::filesystem::create_directories(destination.parent_path(), error);
    if (error)
    {
        return false;
    }

    std::string command{"curl -fsSL --create-dirs -o \""};
    command.append(destination.generic_string());
    command.append("\" \"");
    command.append(kFontDownloadUrl);
    command.append("\"");

    if (std::system(command.c_str()) != 0)
    {
        std::filesystem::remove(destination, error);
        return false;
    }

    return std::filesystem::exists(destination, error);
}

bool EnsureBundledFontAvailable()
{
    const std::filesystem::path bundledPath = GetBundledFontPath();
    std::error_code error;
    if (std::filesystem::exists(bundledPath, error))
    {
        return true;
    }

    for (const auto& candidate : kSystemFontCandidates)
    {
        if (CopyFontIfPresent(candidate, bundledPath))
        {
            return true;
        }
    }

    return DownloadFont(bundledPath);
}

std::string ResolveFontPath()
{
    std::vector<std::filesystem::path> candidates;

    if (const char* envFontPath = std::getenv("COLONY_FONT_PATH"); envFontPath != nullptr)
    {
        std::filesystem::path envPath{envFontPath};
        std::error_code envError;
        if (std::filesystem::exists(envPath, envError))
        {
            return envPath.string();
        }
        std::cerr << "Environment variable COLONY_FONT_PATH is set to '" << envFontPath
                  << "', but the file could not be found. Falling back to defaults.\n";
    }

    EnsureBundledFontAvailable();

    if (char* basePath = SDL_GetBasePath(); basePath != nullptr)
    {
        std::filesystem::path base{basePath};
        SDL_free(basePath);
        candidates.emplace_back(base / kBundledFontDirectory / kFontFileName);
    }

    candidates.emplace_back(GetBundledFontPath());
    candidates.emplace_back(std::filesystem::path{"fonts"} / kFontFileName);
    candidates.emplace_back(std::filesystem::path{kFontFileName});
    candidates.insert(candidates.end(), kSystemFontCandidates.begin(), kSystemFontCandidates.end());

    for (const auto& candidate : candidates)
    {
        std::error_code error;
        if (std::filesystem::exists(candidate, error))
        {
            return candidate.string();
        }
    }

    return {};
}

std::filesystem::path ResolveContentPath()
{
    std::filesystem::path candidate{kContentFile};
    std::error_code error;
    if (std::filesystem::exists(candidate, error))
    {
        return candidate;
    }

    if (char* basePath = SDL_GetBasePath(); basePath != nullptr)
    {
        std::filesystem::path base{basePath};
        SDL_free(basePath);
        std::filesystem::path baseCandidate = base / kContentFile;
        if (std::filesystem::exists(baseCandidate, error))
        {
            return baseCandidate;
        }
    }

    return candidate;
}

bool PointInRect(const SDL_Rect& rect, int x, int y)
{
    return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
}

SDL_Color ParseHexColor(std::string_view hex, SDL_Color fallback = {255, 255, 255, SDL_ALPHA_OPAQUE})
{
    auto hexToByte = [](char c) -> int {
        if (c >= '0' && c <= '9')
        {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f')
        {
            return 10 + (c - 'a');
        }
        if (c >= 'A' && c <= 'F')
        {
            return 10 + (c - 'A');
        }
        return -1;
    };

    std::string cleaned;
    cleaned.reserve(hex.size());
    for (char c : hex)
    {
        if (c != '#')
        {
            cleaned.push_back(c);
        }
    }
    if (cleaned.size() != 6)
    {
        return fallback;
    }

    auto parseComponent = [&](int index) -> int {
        const int high = hexToByte(cleaned[index]);
        const int low = hexToByte(cleaned[index + 1]);
        if (high < 0 || low < 0)
        {
            return -1;
        }
        return (high << 4) | low;
    };

    const int r = parseComponent(0);
    const int g = parseComponent(2);
    const int b = parseComponent(4);
    if (r < 0 || g < 0 || b < 0)
    {
        return fallback;
    }

    return SDL_Color{static_cast<Uint8>(r), static_cast<Uint8>(g), static_cast<Uint8>(b), SDL_ALPHA_OPAQUE};
}

SDL_Color MixColor(const SDL_Color& a, const SDL_Color& b, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    auto blend = [&](Uint8 componentA, Uint8 componentB) {
        return static_cast<Uint8>(componentA + static_cast<int>((componentB - componentA) * t));
    };
    return SDL_Color{blend(a.r, b.r), blend(a.g, b.g), blend(a.b, b.b), blend(a.a, b.a)};
}

void RenderVerticalGradient(SDL_Renderer* renderer, const SDL_Rect& area, SDL_Color top, SDL_Color bottom)
{
    if (renderer == nullptr || area.h <= 0)
    {
        return;
    }

    for (int offset = 0; offset < area.h; ++offset)
    {
        const float t = area.h > 1 ? static_cast<float>(offset) / static_cast<float>(area.h - 1) : 0.0f;
        const SDL_Color color = MixColor(top, bottom, t);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(renderer, area.x, area.y + offset, area.x + area.w, area.y + offset);
    }
}

struct WrappedLine
{
    colony::TextTexture texture;
    bool indent = false;
};

struct ProgramVisuals
{
    const colony::ViewContent* content{};

    colony::TextTexture heroTitle;
    colony::TextTexture heroTagline;
    colony::TextTexture availability;
    colony::TextTexture version;
    colony::TextTexture installState;
    colony::TextTexture lastLaunched;
    colony::TextTexture actionLabel;
    colony::TextTexture statusBar;

    colony::TextTexture tileTitle;
    colony::TextTexture tileSubtitle;
    colony::TextTexture tileMeta;

    SDL_Color accent{};
    SDL_Color gradientStart{};
    SDL_Color gradientEnd{};

    int descriptionWidth = 0;
    std::vector<std::vector<colony::TextTexture>> descriptionLines;

    int highlightsWidth = 0;
    std::vector<std::vector<WrappedLine>> highlightLines;

    struct PatchSection
    {
        colony::TextTexture title;
        int width = 0;
        std::vector<std::vector<WrappedLine>> lines;
    };
    std::vector<PatchSection> sections;
};

void RebuildDescription(ProgramVisuals& visuals, SDL_Renderer* renderer, TTF_Font* font, int maxWidth)
{
    if (renderer == nullptr || font == nullptr || maxWidth <= 0)
    {
        return;
    }
    if (visuals.descriptionWidth == maxWidth)
    {
        return;
    }

    visuals.descriptionWidth = maxWidth;
    visuals.descriptionLines.clear();

    const int lineSkip = TTF_FontLineSkip(font);
    for (const auto& paragraph : visuals.content->paragraphs)
    {
        const auto wrappedLines = colony::WrapTextToWidth(font, paragraph, maxWidth);
        std::vector<colony::TextTexture> lineTextures;
        lineTextures.reserve(wrappedLines.size());
        for (const auto& line : wrappedLines)
        {
            if (line.empty())
            {
                colony::TextTexture placeholder{};
                placeholder.width = 0;
                placeholder.height = std::max(lineSkip, 0);
                lineTextures.emplace_back(std::move(placeholder));
                continue;
            }
            lineTextures.emplace_back(colony::CreateTextTexture(renderer, font, line, kHeroBodyColor));
        }
        if (!lineTextures.empty())
        {
            visuals.descriptionLines.emplace_back(std::move(lineTextures));
        }
    }
}

void RebuildHighlights(
    ProgramVisuals& visuals,
    SDL_Renderer* renderer,
    TTF_Font* font,
    int maxWidth,
    SDL_Color textColor)
{
    if (renderer == nullptr || font == nullptr || maxWidth <= 0)
    {
        return;
    }
    if (visuals.highlightsWidth == maxWidth)
    {
        return;
    }

    visuals.highlightsWidth = maxWidth;
    visuals.highlightLines.clear();

    const int lineSkip = TTF_FontLineSkip(font);
    const int bulletIndent = 24;
    const int availableWidth = std::max(0, maxWidth - bulletIndent);

    for (const auto& highlight : visuals.content->heroHighlights)
    {
        const auto wrappedLines = colony::WrapTextToWidth(font, highlight, availableWidth);
        if (wrappedLines.empty())
        {
            continue;
        }

        std::vector<WrappedLine> lines;
        lines.reserve(wrappedLines.size());
        for (std::size_t i = 0; i < wrappedLines.size(); ++i)
        {
            std::string text = wrappedLines[i];
            bool indent = i != 0;
            if (!indent)
            {
                text.insert(0, "\xE2\x80\xA2 ");
            }
            else
            {
                text.insert(0, "  ");
            }

            WrappedLine line;
            if (text.empty())
            {
                line.texture.width = 0;
                line.texture.height = std::max(lineSkip, 0);
            }
            else
            {
                line.texture = colony::CreateTextTexture(renderer, font, text, textColor);
            }
            line.indent = indent;
            lines.emplace_back(std::move(line));
        }
        if (!lines.empty())
        {
            visuals.highlightLines.emplace_back(std::move(lines));
        }
    }
}

void RebuildSections(
    ProgramVisuals& visuals,
    SDL_Renderer* renderer,
    TTF_Font* titleFont,
    TTF_Font* bodyFont,
    int maxWidth,
    SDL_Color titleColor,
    SDL_Color bodyColor)
{
    if (renderer == nullptr || bodyFont == nullptr || maxWidth <= 0)
    {
        return;
    }

    for (std::size_t i = 0; i < visuals.sections.size(); ++i)
    {
        auto& sectionVisual = visuals.sections[i];
        if (sectionVisual.width == maxWidth)
        {
            continue;
        }

        sectionVisual.width = maxWidth;
        sectionVisual.lines.clear();

        if (titleFont != nullptr)
        {
            const std::string& titleText = visuals.content->sections[i].title;
            sectionVisual.title = colony::CreateTextTexture(renderer, titleFont, titleText, titleColor);
        }

        const int bulletIndent = 20;
        const int availableWidth = std::max(0, maxWidth - bulletIndent);
        const int lineSkip = TTF_FontLineSkip(bodyFont);

        for (const auto& option : visuals.content->sections[i].options)
        {
            const auto wrappedLines = colony::WrapTextToWidth(bodyFont, option, availableWidth);
            if (wrappedLines.empty())
            {
                continue;
            }
            std::vector<WrappedLine> optionLines;
            optionLines.reserve(wrappedLines.size());
            for (std::size_t lineIndex = 0; lineIndex < wrappedLines.size(); ++lineIndex)
            {
                bool indent = lineIndex != 0;
                std::string lineText = wrappedLines[lineIndex];
                if (!indent)
                {
                    lineText.insert(0, "\xE2\x80\xA2 ");
                }
                else
                {
                    lineText.insert(0, "  ");
                }

                WrappedLine line;
                if (lineText.empty())
                {
                    line.texture.width = 0;
                    line.texture.height = std::max(lineSkip, 0);
                }
                else
                {
                    line.texture = colony::CreateTextTexture(renderer, bodyFont, lineText, bodyColor);
                }
                line.indent = indent;
                optionLines.emplace_back(std::move(line));
            }
            if (!optionLines.empty())
            {
                sectionVisual.lines.emplace_back(std::move(optionLines));
            }
        }
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "Failed to initialize SDL2: " << SDL_GetError() << '\n';
        return EXIT_FAILURE;
    }

    colony::sdl::WindowHandle window{SDL_CreateWindow(
        "Colony Launcher",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        kWindowWidth,
        kWindowHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE)};

    if (!window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << '\n';
        SDL_Quit();
        return EXIT_FAILURE;
    }

    colony::sdl::RendererHandle renderer{SDL_CreateRenderer(
        window.get(),
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE)};
    if (!renderer)
    {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << '\n';
        SDL_Quit();
        return EXIT_FAILURE;
    }

    if (TTF_Init() == -1)
    {
        std::cerr << "Failed to initialize SDL_ttf: " << TTF_GetError() << '\n';
        SDL_Quit();
        return EXIT_FAILURE;
    }

    struct TtfGuard
    {
        ~TtfGuard() { TTF_Quit(); }
    } ttfGuard;

    const std::string fontPath = ResolveFontPath();
    if (fontPath.empty())
    {
        std::cerr << "Unable to locate a usable font file. Provide DejaVuSans.ttf in assets/fonts, set COLONY_FONT_PATH, or "
                     "ensure curl is installed for automatic download." << '\n';
        SDL_Quit();
        return EXIT_FAILURE;
    }

    auto openFont = [&](int size) { return colony::sdl::FontHandle{TTF_OpenFont(fontPath.c_str(), size)}; };

    colony::sdl::FontHandle brandFont = openFont(32);
    colony::sdl::FontHandle navFont = openFont(18);
    colony::sdl::FontHandle channelFont = openFont(22);
    colony::sdl::FontHandle tileTitleFont = openFont(22);
    colony::sdl::FontHandle tileMetaFont = openFont(16);
    colony::sdl::FontHandle heroTitleFont = openFont(46);
    colony::sdl::FontHandle heroSubtitleFont = openFont(22);
    colony::sdl::FontHandle heroBodyFont = openFont(18);
    colony::sdl::FontHandle patchTitleFont = openFont(18);
    colony::sdl::FontHandle patchBodyFont = openFont(16);
    colony::sdl::FontHandle buttonFont = openFont(24);
    colony::sdl::FontHandle statusFont = openFont(16);

    if (!brandFont || !navFont || !channelFont || !tileTitleFont || !tileMetaFont || !heroTitleFont || !heroSubtitleFont
        || !heroBodyFont || !patchTitleFont || !patchBodyFont || !buttonFont || !statusFont)
    {
        std::cerr << "Failed to load required fonts from " << fontPath << ": " << TTF_GetError() << '\n';
        SDL_Quit();
        return EXIT_FAILURE;
    }

    colony::AppContent content;
    try
    {
        content = colony::LoadContentFromFile(ResolveContentPath().string());
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << '\n';
        SDL_Quit();
        return EXIT_FAILURE;
    }

    if (content.channels.empty())
    {
        std::cerr << "No channels defined in content file." << '\n';
        SDL_Quit();
        return EXIT_FAILURE;
    }

    std::unordered_map<std::string, ProgramVisuals> programVisuals;
    programVisuals.reserve(content.views.size());

    auto heroSubtitleColor = MixColor(kHeroBodyColor, kHeroTitleColor, 0.35f);

    for (const auto& [id, view] : content.views)
    {
        ProgramVisuals visuals;
        visuals.content = &view;
        visuals.heroTitle = colony::CreateTextTexture(renderer.get(), heroTitleFont.get(), view.heading, kHeroTitleColor);
        if (!view.tagline.empty())
        {
            visuals.heroTagline = colony::CreateTextTexture(renderer.get(), heroSubtitleFont.get(), view.tagline, heroSubtitleColor);
        }
        if (!view.availability.empty())
        {
            visuals.availability = colony::CreateTextTexture(renderer.get(), heroBodyFont.get(), view.availability, kHeroBodyColor);
        }
        if (!view.version.empty())
        {
            visuals.version = colony::CreateTextTexture(renderer.get(), tileMetaFont.get(), view.version, kMutedColor);
        }
        if (!view.installState.empty())
        {
            visuals.installState = colony::CreateTextTexture(renderer.get(), tileMetaFont.get(), view.installState, kMutedColor);
        }
        if (!view.lastLaunched.empty())
        {
            visuals.lastLaunched = colony::CreateTextTexture(renderer.get(), tileMetaFont.get(), view.lastLaunched, kMutedColor);
        }
        visuals.actionLabel = colony::CreateTextTexture(renderer.get(), buttonFont.get(), view.primaryActionLabel, kHeroTitleColor);
        if (!view.statusMessage.empty())
        {
            visuals.statusBar = colony::CreateTextTexture(renderer.get(), statusFont.get(), view.statusMessage, kStatusBarTextColor);
        }

        visuals.tileTitle = colony::CreateTextTexture(renderer.get(), tileTitleFont.get(), view.heading, kHeroTitleColor);
        std::string subtitle = !view.tagline.empty() ? view.tagline : (view.paragraphs.empty() ? std::string{} : view.paragraphs.front());
        if (!subtitle.empty())
        {
            visuals.tileSubtitle = colony::CreateTextTexture(renderer.get(), tileMetaFont.get(), subtitle, kMutedColor);
        }
        std::string meta;
        if (!view.version.empty())
        {
            meta.append(view.version);
        }
        if (!view.installState.empty())
        {
            if (!meta.empty())
            {
                meta.append(" • ");
            }
            meta.append(view.installState);
        }
        if (!meta.empty())
        {
            visuals.tileMeta = colony::CreateTextTexture(renderer.get(), tileMetaFont.get(), meta, kMutedColor);
        }

        visuals.accent = ParseHexColor(view.accentColor, SDL_Color{91, 150, 255, SDL_ALPHA_OPAQUE});
        visuals.gradientStart = ParseHexColor(view.heroGradient[0], SDL_Color{25, 37, 56, SDL_ALPHA_OPAQUE});
        visuals.gradientEnd = ParseHexColor(view.heroGradient[1], SDL_Color{12, 17, 26, SDL_ALPHA_OPAQUE});

        visuals.sections.reserve(view.sections.size());
        for (const auto& section : view.sections)
        {
            ProgramVisuals::PatchSection patchSection;
            if (!section.title.empty())
            {
                patchSection.title = colony::CreateTextTexture(renderer.get(), patchTitleFont.get(), section.title, kHeroTitleColor);
            }
            visuals.sections.emplace_back(std::move(patchSection));
        }

        programVisuals.emplace(id, std::move(visuals));
    }

    std::vector<int> channelSelections(content.channels.size(), 0);
    auto clampSelection = [&](int channelIndex) {
        const auto& channel = content.channels[channelIndex];
        if (channel.programs.empty())
        {
            channelSelections[channelIndex] = 0;
            return;
        }
        channelSelections[channelIndex] = std::clamp(channelSelections[channelIndex], 0, static_cast<int>(channel.programs.size()) - 1);
    };
    for (std::size_t index = 0; index < content.channels.size(); ++index)
    {
        clampSelection(static_cast<int>(index));
    }

    int activeChannelIndex = 0;
    auto getActiveProgramId = [&]() -> std::string {
        const auto& channel = content.channels[activeChannelIndex];
        if (channel.programs.empty())
        {
            return {};
        }
        clampSelection(activeChannelIndex);
        return channel.programs[channelSelections[activeChannelIndex]];
    };

    auto activeProgramId = getActiveProgramId();

    auto rebuildStatusTexture = [&]() {
        const auto visualsIt = programVisuals.find(activeProgramId);
        actionButtonRect.reset();
        if (visualsIt != programVisuals.end())
        {
            if (!visualsIt->second.content->statusMessage.empty())
            {
                visualsIt->second.statusBar = colony::CreateTextTexture(
                    renderer.get(),
                    statusFont.get(),
                    visualsIt->second.content->statusMessage,
                    kStatusBarTextColor);
            }
            else
            {
                visualsIt->second.statusBar = {};
            }
        }
    };

    rebuildStatusTexture();

    std::vector<colony::TextTexture> channelLabelTextures;
    channelLabelTextures.reserve(content.channels.size());
    for (const auto& channel : content.channels)
    {
        channelLabelTextures.emplace_back(colony::CreateTextTexture(renderer.get(), navFont.get(), channel.label, kNavTextColor));
    }

    colony::TextTexture brandTexture = colony::CreateTextTexture(renderer.get(), brandFont.get(), content.brandName, kHeroTitleColor);
    colony::TextTexture userNameTexture = colony::CreateTextTexture(renderer.get(), navFont.get(), content.user.name, kHeroTitleColor);
    colony::TextTexture userStatusTexture = colony::CreateTextTexture(renderer.get(), tileMetaFont.get(), content.user.status, kMutedColor);

    colony::TextTexture capabilitiesLabel = colony::CreateTextTexture(renderer.get(), tileMetaFont.get(), "CAPABILITIES", kMutedColor);
    colony::TextTexture updatesLabel = colony::CreateTextTexture(renderer.get(), tileMetaFont.get(), "PATCH NOTES", kMutedColor);

    bool running = true;
    SDL_Event event;

    std::vector<SDL_Rect> channelButtonRects(content.channels.size());
    std::vector<SDL_Rect> programTileRects;
    std::optional<SDL_Rect> actionButtonRect;

    auto activateProgram = [&](const std::string& programId) {
        if (programVisuals.find(programId) == programVisuals.end())
        {
            return;
        }
        activeProgramId = programId;
        rebuildStatusTexture();
    };

    auto activateChannel = [&](int index) {
        if (index < 0 || index >= static_cast<int>(content.channels.size()))
        {
            return;
        }
        activeChannelIndex = index;
        clampSelection(activeChannelIndex);
        activateProgram(getActiveProgramId());
    };

    auto activateProgramInChannel = [&](int programIndex) {
        clampSelection(activeChannelIndex);
        const auto& channel = content.channels[activeChannelIndex];
        if (channel.programs.empty())
        {
            return;
        }
        const int clamped = std::clamp(programIndex, 0, static_cast<int>(channel.programs.size()) - 1);
        channelSelections[activeChannelIndex] = clamped;
        activateProgram(channel.programs[clamped]);
    };

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                const int mouseX = event.button.x;
                const int mouseY = event.button.y;

                for (std::size_t i = 0; i < channelButtonRects.size(); ++i)
                {
                    if (PointInRect(channelButtonRects[i], mouseX, mouseY))
                    {
                        activateChannel(static_cast<int>(i));
                        break;
                    }
                }

                for (std::size_t i = 0; i < programTileRects.size(); ++i)
                {
                    if (PointInRect(programTileRects[i], mouseX, mouseY))
                    {
                        activateProgramInChannel(static_cast<int>(i));
                        break;
                    }
                }

                if (actionButtonRect.has_value() && PointInRect(*actionButtonRect, mouseX, mouseY))
                {
                    rebuildStatusTexture();
                }
            }
            else if (event.type == SDL_KEYDOWN)
            {
                const SDL_Keycode key = event.key.keysym.sym;
                switch (key)
                {
                case SDLK_UP:
                    activateProgramInChannel(channelSelections[activeChannelIndex] - 1);
                    break;
                case SDLK_DOWN:
                    activateProgramInChannel(channelSelections[activeChannelIndex] + 1);
                    break;
                case SDLK_LEFT:
                    activateChannel(activeChannelIndex - 1);
                    break;
                case SDLK_RIGHT:
                    activateChannel(activeChannelIndex + 1);
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                case SDLK_SPACE:
                    rebuildStatusTexture();
                    break;
                default:
                    break;
                }
            }
        }

        int outputWidth = 0;
        int outputHeight = 0;
        SDL_GetRendererOutputSize(renderer.get(), &outputWidth, &outputHeight);

        SDL_SetRenderDrawColor(renderer.get(), kBackgroundColor.r, kBackgroundColor.g, kBackgroundColor.b, kBackgroundColor.a);
        SDL_RenderClear(renderer.get());

        const int navRailWidth = 96;
        const int statusBarHeight = 52;
        const int libraryWidth = std::clamp(outputWidth / 4, 320, 360);
        SDL_Rect navRailRect{0, 0, navRailWidth, outputHeight};
        SDL_SetRenderDrawColor(renderer.get(), kNavRailColor.r, kNavRailColor.g, kNavRailColor.b, kNavRailColor.a);
        SDL_RenderFillRect(renderer.get(), &navRailRect);

        SDL_Rect libraryRect{navRailWidth, 0, libraryWidth, outputHeight};
        SDL_SetRenderDrawColor(renderer.get(), kLibraryBackground.r, kLibraryBackground.g, kLibraryBackground.b, kLibraryBackground.a);
        SDL_RenderFillRect(renderer.get(), &libraryRect);

        const SDL_Rect heroRect{navRailWidth + libraryWidth, 0, outputWidth - navRailWidth - libraryWidth, outputHeight};
        const auto visualsIt = programVisuals.find(activeProgramId);
        if (visualsIt != programVisuals.end())
        {
            RenderVerticalGradient(renderer.get(), heroRect, visualsIt->second.gradientStart, visualsIt->second.gradientEnd);
        }
        else
        {
            RenderVerticalGradient(renderer.get(), heroRect, SDL_Color{28, 40, 59, SDL_ALPHA_OPAQUE}, SDL_Color{14, 18, 28, SDL_ALPHA_OPAQUE});
        }

        // Draw brand in nav rail
        const int navPadding = 28;
        if (brandTexture.texture)
        {
            SDL_Rect brandRect{
                navRailRect.x + (navRailRect.w - brandTexture.width) / 2,
                navPadding,
                brandTexture.width,
                brandTexture.height};
            colony::RenderTexture(renderer.get(), brandTexture, brandRect);
        }

        SDL_Rect navDivider{navRailRect.x + navRailRect.w - 2, 0, 2, outputHeight};
        SDL_SetRenderDrawColor(renderer.get(), kBorderColor.r, kBorderColor.g, kBorderColor.b, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer.get(), &navDivider);

        const auto channelAccentColor = [&](int index) {
            const auto& channel = content.channels[index];
            if (channel.programs.empty())
            {
                return kChannelBadgeColor;
            }
            const auto& programId = channel.programs[std::clamp(channelSelections[index], 0, static_cast<int>(channel.programs.size()) - 1)];
            if (const auto it = programVisuals.find(programId); it != programVisuals.end())
            {
                return MixColor(it->second.accent, kChannelBadgeColor, 0.25f);
            }
            return kChannelBadgeColor;
        };

        int channelStartY = navPadding + (brandTexture.height > 0 ? brandTexture.height + 32 : 48);
        const int channelButtonSize = 48;
        const int channelSpacing = 32;
        for (std::size_t i = 0; i < content.channels.size(); ++i)
        {
            const bool isActive = static_cast<int>(i) == activeChannelIndex;
            SDL_Rect buttonRect{
                navRailRect.x + (navRailRect.w - channelButtonSize) / 2,
                channelStartY,
                channelButtonSize,
                channelButtonSize};

            SDL_Color baseColor = channelAccentColor(static_cast<int>(i));
            SDL_Color fillColor = isActive ? MixColor(baseColor, kHeroTitleColor, 0.15f) : baseColor;
            SDL_SetRenderDrawColor(renderer.get(), fillColor.r, fillColor.g, fillColor.b, fillColor.a);
            SDL_RenderFillRect(renderer.get(), &buttonRect);
            SDL_SetRenderDrawColor(renderer.get(), kBorderColor.r, kBorderColor.g, kBorderColor.b, SDL_ALPHA_OPAQUE);
            SDL_RenderDrawRect(renderer.get(), &buttonRect);

            if (i < channelLabelTextures.size() && channelLabelTextures[i].texture)
            {
                SDL_Rect labelRect{
                    navRailRect.x + (navRailRect.w - channelLabelTextures[i].width) / 2,
                    buttonRect.y + buttonRect.h + 6,
                    channelLabelTextures[i].width,
                    channelLabelTextures[i].height};
                colony::RenderTexture(renderer.get(), channelLabelTextures[i], labelRect);
            }

            channelButtonRects[i] = buttonRect;
            channelStartY += channelButtonSize + channelSpacing;
        }

        if (userNameTexture.texture)
        {
            const int avatarSize = 14;
            SDL_Rect avatarRect{
                navRailRect.x + (navRailRect.w - avatarSize) / 2,
                outputHeight - statusBarHeight - 40,
                avatarSize,
                avatarSize};
            SDL_SetRenderDrawColor(renderer.get(), 90, 214, 102, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(renderer.get(), &avatarRect);

            SDL_Rect nameRect{
                navRailRect.x + (navRailRect.w - userNameTexture.width) / 2,
                avatarRect.y + avatarRect.h + 8,
                userNameTexture.width,
                userNameTexture.height};
            colony::RenderTexture(renderer.get(), userNameTexture, nameRect);

            if (userStatusTexture.texture)
            {
                SDL_Rect statusRect{
                    navRailRect.x + (navRailRect.w - userStatusTexture.width) / 2,
                    nameRect.y + nameRect.h + 4,
                    userStatusTexture.width,
                    userStatusTexture.height};
                colony::RenderTexture(renderer.get(), userStatusTexture, statusRect);
            }
        }

        // Library content
        const auto& activeChannel = content.channels[activeChannelIndex];
        colony::TextTexture channelTitleTexture = colony::CreateTextTexture(renderer.get(), channelFont.get(), activeChannel.label, kHeroTitleColor);

        const int libraryPadding = 28;
        int libraryCursorY = libraryPadding;

        if (channelTitleTexture.texture)
        {
            SDL_Rect channelTitleRect{libraryRect.x + libraryPadding, libraryCursorY, channelTitleTexture.width, channelTitleTexture.height};
            colony::RenderTexture(renderer.get(), channelTitleTexture, channelTitleRect);
            libraryCursorY += channelTitleRect.h + 24;
        }

        SDL_Rect filterRect{libraryRect.x + libraryPadding, libraryCursorY, libraryRect.w - 2 * libraryPadding, 36};
        SDL_SetRenderDrawColor(renderer.get(), kLibraryCardColor.r, kLibraryCardColor.g, kLibraryCardColor.b, kLibraryCardColor.a);
        SDL_RenderFillRect(renderer.get(), &filterRect);
        SDL_SetRenderDrawColor(renderer.get(), kBorderColor.r, kBorderColor.g, kBorderColor.b, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawRect(renderer.get(), &filterRect);
        colony::TextTexture filterLabel = colony::CreateTextTexture(renderer.get(), tileMetaFont.get(), "Installed programs", kMutedColor);
        if (filterLabel.texture)
        {
            SDL_Rect filterLabelRect{
                filterRect.x + 12,
                filterRect.y + (filterRect.h - filterLabel.height) / 2,
                filterLabel.width,
                filterLabel.height};
            colony::RenderTexture(renderer.get(), filterLabel, filterLabelRect);
        }
        libraryCursorY += filterRect.h + 24;

        programTileRects.clear();
        programTileRects.reserve(activeChannel.programs.size());

        const int tileHeight = 100;
        const int tileSpacing = 18;
        for (std::size_t index = 0; index < activeChannel.programs.size(); ++index)
        {
            const std::string& programId = activeChannel.programs[index];
            const auto programIt = programVisuals.find(programId);
            if (programIt == programVisuals.end())
            {
                continue;
            }
            const bool isActiveProgram = static_cast<int>(index) == channelSelections[activeChannelIndex];

            SDL_Rect tileRect{
                libraryRect.x + libraryPadding,
                libraryCursorY,
                libraryRect.w - 2 * libraryPadding,
                tileHeight};

            SDL_Color baseColor = isActiveProgram ? kLibraryCardActive : kLibraryCardColor;
            if (isActiveProgram)
            {
                baseColor = MixColor(baseColor, programIt->second.accent, 0.2f);
            }

            SDL_SetRenderDrawColor(renderer.get(), baseColor.r, baseColor.g, baseColor.b, baseColor.a);
            SDL_RenderFillRect(renderer.get(), &tileRect);
            SDL_SetRenderDrawColor(renderer.get(), kBorderColor.r, kBorderColor.g, kBorderColor.b, SDL_ALPHA_OPAQUE);
            SDL_RenderDrawRect(renderer.get(), &tileRect);

            SDL_Rect accentStrip{tileRect.x, tileRect.y, 6, tileRect.h};
            SDL_SetRenderDrawColor(renderer.get(), programIt->second.accent.r, programIt->second.accent.g, programIt->second.accent.b, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(renderer.get(), &accentStrip);

            int textX = tileRect.x + 18;
            int textY = tileRect.y + 14;
            if (programIt->second.tileTitle.texture)
            {
                SDL_Rect titleRect{textX, textY, programIt->second.tileTitle.width, programIt->second.tileTitle.height};
                colony::RenderTexture(renderer.get(), programIt->second.tileTitle, titleRect);
                textY += titleRect.h + 6;
            }
            if (programIt->second.tileSubtitle.texture)
            {
                SDL_Rect subtitleRect{textX, textY, programIt->second.tileSubtitle.width, programIt->second.tileSubtitle.height};
                colony::RenderTexture(renderer.get(), programIt->second.tileSubtitle, subtitleRect);
                textY += subtitleRect.h + 6;
            }
            if (programIt->second.tileMeta.texture)
            {
                SDL_Rect metaRect{textX, textY, programIt->second.tileMeta.width, programIt->second.tileMeta.height};
                colony::RenderTexture(renderer.get(), programIt->second.tileMeta, metaRect);
            }

            programTileRects.emplace_back(tileRect);
            libraryCursorY += tileHeight + tileSpacing;
        }

        // Hero content
        if (visualsIt != programVisuals.end())
        {
            ProgramVisuals& visuals = visualsIt->second;
            const int heroPaddingX = 56;
            const int heroPaddingY = 58;
            const int heroContentX = heroRect.x + heroPaddingX;
            int heroCursorY = heroRect.y + heroPaddingY;
            const int heroContentWidth = heroRect.w - heroPaddingX * 2;
            const int heroColumnsGap = 32;
            int patchPanelWidth = heroRect.w >= 960 ? std::min(340, heroContentWidth / 2) : 0;
            int textColumnWidth = heroContentWidth - (patchPanelWidth > 0 ? (patchPanelWidth + heroColumnsGap) : 0);
            if (textColumnWidth < 360)
            {
                patchPanelWidth = 0;
                textColumnWidth = heroContentWidth;
            }

            const SDL_Color highlightColor = MixColor(visuals.accent, kHeroBodyColor, 0.25f);
            RebuildDescription(visuals, renderer.get(), heroBodyFont.get(), textColumnWidth);
            RebuildHighlights(visuals, renderer.get(), heroBodyFont.get(), textColumnWidth, highlightColor);
            if (patchPanelWidth > 0)
            {
                RebuildSections(visuals, renderer.get(), patchTitleFont.get(), patchBodyFont.get(), patchPanelWidth - 32, kHeroTitleColor, kHeroBodyColor);
            }

            if (visuals.availability.texture)
            {
                SDL_Rect chipRect{
                    heroContentX,
                    heroCursorY,
                    visuals.availability.width + 28,
                    visuals.availability.height + 12};
                SDL_Color chipColor = MixColor(visuals.accent, kStatusBarColor, 0.2f);
                SDL_SetRenderDrawColor(renderer.get(), chipColor.r, chipColor.g, chipColor.b, chipColor.a);
                SDL_RenderFillRect(renderer.get(), &chipRect);
                SDL_SetRenderDrawColor(renderer.get(), visuals.accent.r, visuals.accent.g, visuals.accent.b, SDL_ALPHA_OPAQUE);
                SDL_RenderDrawRect(renderer.get(), &chipRect);
                SDL_Rect chipTextRect{
                    chipRect.x + 14,
                    chipRect.y + (chipRect.h - visuals.availability.height) / 2,
                    visuals.availability.width,
                    visuals.availability.height};
                colony::RenderTexture(renderer.get(), visuals.availability, chipTextRect);
                heroCursorY += chipRect.h + 18;
            }

            if (visuals.heroTitle.texture)
            {
                SDL_Rect titleRect{heroContentX, heroCursorY, visuals.heroTitle.width, visuals.heroTitle.height};
                colony::RenderTexture(renderer.get(), visuals.heroTitle, titleRect);
                heroCursorY += titleRect.h + 18;
            }

            if (visuals.heroTagline.texture)
            {
                SDL_Rect taglineRect{heroContentX, heroCursorY, visuals.heroTagline.width, visuals.heroTagline.height};
                colony::RenderTexture(renderer.get(), visuals.heroTagline, taglineRect);
                heroCursorY += taglineRect.h + 24;
            }

            const int descriptionSpacing = 18;
            const int baseLineSkip = heroBodyFont ? TTF_FontLineSkip(heroBodyFont.get()) : 0;
            for (const auto& paragraphLines : visuals.descriptionLines)
            {
                for (std::size_t lineIndex = 0; lineIndex < paragraphLines.size(); ++lineIndex)
                {
                    const auto& lineTexture = paragraphLines[lineIndex];
                    SDL_Rect lineRect{heroContentX, heroCursorY, lineTexture.width, lineTexture.height};
                    colony::RenderTexture(renderer.get(), lineTexture, lineRect);
                    heroCursorY += lineRect.h;
                    if (lineIndex + 1 < paragraphLines.size())
                    {
                        const int spacing = baseLineSkip > 0 ? std::max(0, baseLineSkip - lineTexture.height) : 6;
                        heroCursorY += spacing;
                    }
                }
                heroCursorY += descriptionSpacing;
            }

            if (!visuals.highlightLines.empty())
            {
                if (capabilitiesLabel.texture)
                {
                    SDL_Rect labelRect{heroContentX, heroCursorY, capabilitiesLabel.width, capabilitiesLabel.height};
                    colony::RenderTexture(renderer.get(), capabilitiesLabel, labelRect);
                    heroCursorY += labelRect.h + 12;
                }

                const int bulletIndent = 24;
                for (const auto& lines : visuals.highlightLines)
                {
                    for (const auto& line : lines)
                    {
                        const int bulletX = heroContentX + (line.indent ? bulletIndent : 0);
                        SDL_Rect lineRect{bulletX, heroCursorY, line.texture.width, line.texture.height};
                        colony::RenderTexture(renderer.get(), line.texture, lineRect);
                        heroCursorY += lineRect.h + 4;
                    }
                    heroCursorY += 8;
                }
            }

            heroCursorY += 16;

            const int buttonWidth = 240;
            const int buttonHeight = 64;
            SDL_Rect buttonRect{heroContentX, heroCursorY, buttonWidth, buttonHeight};
            SDL_Color buttonColor = MixColor(visuals.accent, kHeroTitleColor, 0.15f);
            SDL_SetRenderDrawColor(renderer.get(), buttonColor.r, buttonColor.g, buttonColor.b, buttonColor.a);
            SDL_RenderFillRect(renderer.get(), &buttonRect);
            SDL_SetRenderDrawColor(renderer.get(), visuals.accent.r, visuals.accent.g, visuals.accent.b, SDL_ALPHA_OPAQUE);
            SDL_RenderDrawRect(renderer.get(), &buttonRect);
            if (visuals.actionLabel.texture)
            {
                SDL_Rect buttonTextRect{
                    buttonRect.x + (buttonRect.w - visuals.actionLabel.width) / 2,
                    buttonRect.y + (buttonRect.h - visuals.actionLabel.height) / 2,
                    visuals.actionLabel.width,
                    visuals.actionLabel.height};
                colony::RenderTexture(renderer.get(), visuals.actionLabel, buttonTextRect);
            }
            actionButtonRect = buttonRect;

            heroCursorY += buttonRect.h + 22;

            int chipCursorX = heroContentX;
            const int chipSpacing = 12;
            auto drawMetaChip = [&](const colony::TextTexture& texture) {
                if (!texture.texture)
                {
                    return;
                }
                SDL_Rect chipRect{chipCursorX, heroCursorY, texture.width + 26, texture.height + 12};
                SDL_SetRenderDrawColor(renderer.get(), kStatusBarColor.r, kStatusBarColor.g, kStatusBarColor.b, kStatusBarColor.a);
                SDL_RenderFillRect(renderer.get(), &chipRect);
                SDL_SetRenderDrawColor(renderer.get(), kBorderColor.r, kBorderColor.g, kBorderColor.b, SDL_ALPHA_OPAQUE);
                SDL_RenderDrawRect(renderer.get(), &chipRect);
                SDL_Rect textRect{
                    chipRect.x + 13,
                    chipRect.y + (chipRect.h - texture.height) / 2,
                    texture.width,
                    texture.height};
                colony::RenderTexture(renderer.get(), texture, textRect);
                chipCursorX += chipRect.w + chipSpacing;
            };
            drawMetaChip(visuals.version);
            drawMetaChip(visuals.installState);
            drawMetaChip(visuals.lastLaunched);

            if (patchPanelWidth > 0 && !visuals.sections.empty())
            {
                SDL_Rect patchRect{
                    heroRect.x + heroRect.w - heroPaddingX - patchPanelWidth,
                    heroRect.y + heroPaddingY,
                    patchPanelWidth,
                    outputHeight - heroPaddingY * 2 - statusBarHeight};
                SDL_Color patchBg = MixColor(kStatusBarColor, visuals.accent, 0.12f);
                SDL_SetRenderDrawColor(renderer.get(), patchBg.r, patchBg.g, patchBg.b, patchBg.a);
                SDL_RenderFillRect(renderer.get(), &patchRect);
                SDL_SetRenderDrawColor(renderer.get(), visuals.accent.r, visuals.accent.g, visuals.accent.b, SDL_ALPHA_OPAQUE);
                SDL_RenderDrawRect(renderer.get(), &patchRect);

                int patchCursorX = patchRect.x + 24;
                int patchCursorY = patchRect.y + 24;
                if (updatesLabel.texture)
                {
                    SDL_Rect labelRect{patchCursorX, patchCursorY, updatesLabel.width, updatesLabel.height};
                    colony::RenderTexture(renderer.get(), updatesLabel, labelRect);
                    patchCursorY += labelRect.h + 12;
                }

                const int bulletIndent = 20;
                for (const auto& section : visuals.sections)
                {
                    if (section.title.texture)
                    {
                        SDL_Rect titleRect{patchCursorX, patchCursorY, section.title.width, section.title.height};
                        colony::RenderTexture(renderer.get(), section.title, titleRect);
                        patchCursorY += titleRect.h + 12;
                    }

                    for (const auto& optionLines : section.lines)
                    {
                        for (const auto& line : optionLines)
                        {
                            SDL_Rect lineRect{
                                patchCursorX + (line.indent ? bulletIndent : 0),
                                patchCursorY,
                                line.texture.width,
                                line.texture.height};
                            colony::RenderTexture(renderer.get(), line.texture, lineRect);
                            patchCursorY += lineRect.h + 4;
                        }
                        patchCursorY += 10;
                    }

                    patchCursorY += 12;
                }
            }
        }

        // Status bar
        SDL_Rect statusRect{heroRect.x, outputHeight - statusBarHeight, heroRect.w, statusBarHeight};
        SDL_SetRenderDrawColor(renderer.get(), kStatusBarColor.r, kStatusBarColor.g, kStatusBarColor.b, kStatusBarColor.a);
        SDL_RenderFillRect(renderer.get(), &statusRect);
        SDL_SetRenderDrawColor(renderer.get(), kBorderColor.r, kBorderColor.g, kBorderColor.b, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(renderer.get(), statusRect.x, statusRect.y, statusRect.x + statusRect.w, statusRect.y);

        if (visualsIt != programVisuals.end())
        {
            const auto& visuals = visualsIt->second;
            if (visuals.statusBar.texture)
            {
                SDL_Rect textRect{
                    statusRect.x + 24,
                    statusRect.y + (statusRect.h - visuals.statusBar.height) / 2,
                    visuals.statusBar.width,
                    visuals.statusBar.height};
                colony::RenderTexture(renderer.get(), visuals.statusBar, textRect);
            }
        }

        SDL_RenderPresent(renderer.get());
    }

    return EXIT_SUCCESS;
}
