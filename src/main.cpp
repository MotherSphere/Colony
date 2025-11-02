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
#include <vector>

#include "content_loader.hpp"
#include "controllers/navigation_controller.hpp"
#include "utils/sdl_wrappers.hpp"
#include "utils/text.hpp"
#include "views/simple_text_view.hpp"

namespace
{

constexpr int kWindowWidth = 1024;
constexpr int kWindowHeight = 640;
constexpr SDL_Color kBackgroundColor{245, 245, 245, SDL_ALPHA_OPAQUE};
constexpr SDL_Color kSidebarColor{236, 236, 236, SDL_ALPHA_OPAQUE};
constexpr SDL_Color kPrimaryTextColor{30, 30, 30, SDL_ALPHA_OPAQUE};
constexpr SDL_Color kMutedTextColor{120, 120, 120, SDL_ALPHA_OPAQUE};
constexpr SDL_Color kAccentColor{20, 20, 20, SDL_ALPHA_OPAQUE};
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

} // namespace

int main(int argc, char** argv)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "Failed to initialize SDL2: " << SDL_GetError() << '\n';
        return EXIT_FAILURE;
    }

    colony::sdl::WindowHandle window{SDL_CreateWindow(
        "Ecosystem Application",
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

    colony::sdl::FontHandle brandFont = openFont(44);
    colony::sdl::FontHandle navFont = openFont(22);
    colony::sdl::FontHandle headingFont = openFont(52);
    colony::sdl::FontHandle paragraphFont = openFont(20);
    colony::sdl::FontHandle buttonFont = openFont(24);

    if (!brandFont || !navFont || !headingFont || !paragraphFont || !buttonFont)
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

    colony::NavigationController navigationController;
    navigationController.SetEntries(content.navigation);

    colony::ViewCollection views;
    views.reserve(content.navigation.size());
    for (const auto& id : content.navigation)
    {
        auto view = std::make_unique<colony::SimpleTextView>(id);
        if (auto it = content.views.find(id); it != content.views.end())
        {
            view->BindContent(it->second);
        }
        else
        {
            colony::ViewContent fallbackContent{
                .heading = id,
                .paragraphs = {"This section is waiting for mission data."},
                .primaryActionLabel = "Acknowledge",
                .statusMessage = "No module connected to this panel yet."};
            view->BindContent(fallbackContent);
        }
        views.emplace_back(std::move(view));
    }

    colony::RenderContext renderContext{
        .renderer = renderer.get(),
        .headingFont = headingFont.get(),
        .paragraphFont = paragraphFont.get(),
        .buttonFont = buttonFont.get(),
        .primaryColor = kPrimaryTextColor,
        .mutedColor = kMutedTextColor,
        .accentColor = kAccentColor};

    colony::View* activeView = nullptr;
    std::string statusMessage{"Systems idle."};
    colony::TextTexture statusTexture;

    auto rebuildStatusTexture = [&]() {
        statusTexture = colony::CreateTextTexture(renderContext.renderer, paragraphFont.get(), statusMessage, kMutedTextColor);
    };

    colony::TextTexture brandTexture = colony::CreateTextTexture(
        renderContext.renderer,
        brandFont.get(),
        content.brandName,
        kPrimaryTextColor);

    std::vector<colony::TextTexture> navigationTextures;
    std::vector<SDL_Rect> navigationRects(content.navigation.size());

    auto refreshNavigationTextures = [&]() {
        navigationTextures.clear();
        navigationTextures.reserve(content.navigation.size());
        for (std::size_t i = 0; i < content.navigation.size(); ++i)
        {
            const bool isActive = static_cast<int>(i) == navigationController.ActiveIndex();
            navigationTextures.emplace_back(colony::CreateTextTexture(
                renderContext.renderer,
                navFont.get(),
                content.navigation[i],
                isActive ? kPrimaryTextColor : kMutedTextColor));
        }
    };

    auto activateView = [&](int index) {
        if (index < 0 || index >= static_cast<int>(views.size()))
        {
            return;
        }

        if (activeView != nullptr)
        {
            activeView->Deactivate();
        }

        activeView = views[index].get();
        activeView->Activate(renderContext);

        const auto& entryId = navigationController.Entries()[index];
        if (auto it = content.views.find(entryId); it != content.views.end())
        {
            statusMessage = it->second.statusMessage;
        }
        else
        {
            statusMessage = "Awaiting subsystem telemetry.";
        }
        rebuildStatusTexture();
        refreshNavigationTextures();
    };

    navigationController.OnSelectionChanged([&](int index) { activateView(index); });

    refreshNavigationTextures();
    activateView(navigationController.ActiveIndex());

    bool running = true;
    SDL_Event event;
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

                for (std::size_t i = 0; i < navigationRects.size(); ++i)
                {
                    if (PointInRect(navigationRects[i], mouseX, mouseY))
                    {
                        navigationController.Activate(static_cast<int>(i));
                        break;
                    }
                }

                if (activeView != nullptr)
                {
                    if (const std::optional<SDL_Rect> actionRect = activeView->PrimaryActionRect())
                    {
                        if (PointInRect(*actionRect, mouseX, mouseY))
                        {
                            activeView->OnPrimaryAction(statusMessage);
                            rebuildStatusTexture();
                        }
                    }
                }
            }
        }

        int outputWidth = 0;
        int outputHeight = 0;
        SDL_GetRendererOutputSize(renderer.get(), &outputWidth, &outputHeight);

        SDL_SetRenderDrawColor(renderer.get(), kBackgroundColor.r, kBackgroundColor.g, kBackgroundColor.b, kBackgroundColor.a);
        SDL_RenderClear(renderer.get());

        const int sidebarWidth = static_cast<int>(std::clamp(outputWidth / 4, 220, 280));
        const int contentPadding = 48;

        SDL_Rect sidebarRect{0, 0, sidebarWidth, outputHeight};
        SDL_SetRenderDrawColor(renderer.get(), kSidebarColor.r, kSidebarColor.g, kSidebarColor.b, kSidebarColor.a);
        SDL_RenderFillRect(renderer.get(), &sidebarRect);

        SDL_SetRenderDrawColor(renderer.get(), kAccentColor.r, kAccentColor.g, kAccentColor.b, kAccentColor.a);
        SDL_RenderDrawLine(renderer.get(), sidebarWidth, 0, sidebarWidth, outputHeight);

        if (brandTexture.texture)
        {
            SDL_Rect brandRect{contentPadding / 2, contentPadding, brandTexture.width, brandTexture.height};
            colony::RenderTexture(renderer.get(), brandTexture, brandRect);
        }

        int navY = contentPadding + 120;
        const int navSpacing = 48;
        for (std::size_t i = 0; i < navigationTextures.size(); ++i)
        {
            const auto& texture = navigationTextures[i];
            SDL_Rect navRect{contentPadding / 2, navY, texture.width, texture.height};
            navigationRects[i] = navRect;
            colony::RenderTexture(renderer.get(), texture, navRect);
            if (static_cast<int>(i) == navigationController.ActiveIndex())
            {
                SDL_RenderDrawLine(renderer.get(), navRect.x, navRect.y + navRect.h + 6, navRect.x + navRect.w, navRect.y + navRect.h + 6);
            }
            navY += navSpacing;
        }

        const int contentStartX = sidebarWidth + contentPadding;
        const int contentWidth = outputWidth - contentStartX - contentPadding;
        const int contentStartY = contentPadding;

        const int timelineY = contentStartY + 8;
        const int timelineStartX = contentStartX;
        const int timelineEndX = contentStartX + std::max(contentWidth - 120, 120);
        SDL_SetRenderDrawColor(renderer.get(), 200, 200, 200, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(renderer.get(), timelineStartX, timelineY, timelineEndX, timelineY);
        SDL_Rect timelineAccent{timelineEndX, timelineY - 3, 12, 12};
        SDL_SetRenderDrawColor(renderer.get(), kAccentColor.r, kAccentColor.g, kAccentColor.b, kAccentColor.a);
        SDL_RenderFillRect(renderer.get(), &timelineAccent);

        if (activeView != nullptr)
        {
            SDL_Rect contentBounds{contentStartX, timelineY + 72, contentWidth, outputHeight - (timelineY + 72) - contentPadding};
            activeView->Render(renderContext, contentBounds);
        }

        if (statusTexture.texture)
        {
            SDL_Rect statusRect{contentStartX, outputHeight - contentPadding - statusTexture.height, statusTexture.width, statusTexture.height};
            colony::RenderTexture(renderer.get(), statusTexture, statusRect);
        }

        SDL_RenderPresent(renderer.get());
    }

    if (activeView != nullptr)
    {
        activeView->Deactivate();
    }

    return EXIT_SUCCESS;
}
