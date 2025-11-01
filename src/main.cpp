#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

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
    "https://github.com/dejavu-fonts/dejavu-fonts/raw/master/ttf/DejaVuSans.ttf";

constexpr std::array<std::filesystem::path, 3> kSystemFontCandidates{
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

struct TextTexture
{
    SDL_Texture* texture{};
    int width{};
    int height{};
};

TextTexture CreateTextTexture(SDL_Renderer* renderer, TTF_Font* font, std::string_view text, SDL_Color color)
{
    const std::string textString{text};
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, textString.c_str(), color);
    if (surface == nullptr)
    {
        std::cerr << "Failed to render text surface: " << TTF_GetError() << '\n';
        return {};
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == nullptr)
    {
        std::cerr << "Failed to create text texture: " << SDL_GetError() << '\n';
        SDL_FreeSurface(surface);
        return {};
    }

    TextTexture result{texture, surface->w, surface->h};
    SDL_FreeSurface(surface);
    return result;
}

void DestroyTextTexture(TextTexture& textTexture)
{
    if (textTexture.texture != nullptr)
    {
        SDL_DestroyTexture(textTexture.texture);
        textTexture = {};
    }
}
}

int main(int argc, char** argv)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "Failed to initialize SDL2: " << SDL_GetError() << '\n';
        return EXIT_FAILURE;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Ecosystem Application",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        kWindowWidth,
        kWindowHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (window == nullptr)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << '\n';
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Renderer* renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    if (renderer == nullptr)
    {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    if (TTF_Init() == -1)
    {
        std::cerr << "Failed to initialize SDL_ttf: " << TTF_GetError() << '\n';
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    const std::string fontPath = ResolveFontPath();
    if (fontPath.empty())
    {
        std::cerr << "Unable to locate a usable font file. Provide DejaVuSans.ttf in assets/fonts, set COLONY_FONT_PATH, "
                  << "or ensure curl is installed for automatic download." << '\n';
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    auto loadFont = [&](int size) { return TTF_OpenFont(fontPath.c_str(), size); };

    TTF_Font* brandFont = loadFont(44);
    TTF_Font* navFont = loadFont(22);
    TTF_Font* headingFont = loadFont(58);
    TTF_Font* paragraphFont = loadFont(20);
    TTF_Font* buttonFont = loadFont(24);

    if (brandFont == nullptr || navFont == nullptr || headingFont == nullptr || paragraphFont == nullptr
        || buttonFont == nullptr)
    {
        std::cerr << "Failed to load font from " << fontPath << ": " << TTF_GetError() << '\n';
        if (brandFont != nullptr)
            TTF_CloseFont(brandFont);
        if (navFont != nullptr)
            TTF_CloseFont(navFont);
        if (headingFont != nullptr)
            TTF_CloseFont(headingFont);
        if (paragraphFont != nullptr)
            TTF_CloseFont(paragraphFont);
        if (buttonFont != nullptr)
            TTF_CloseFont(buttonFont);
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    TextTexture brandText = CreateTextTexture(renderer, brandFont, "COLONY", kPrimaryTextColor);
    std::vector<TextTexture> navigationTexts;
    std::vector<std::string> navigationLabels{"HOME", "MISSIONS", "DATABASE", "SETTINGS"};
    navigationTexts.reserve(navigationLabels.size());
    for (const auto& label : navigationLabels)
    {
        navigationTexts.emplace_back(CreateTextTexture(renderer, navFont, label, label == "HOME" ? kPrimaryTextColor : kMutedTextColor));
    }

    TextTexture welcomeText = CreateTextTexture(renderer, headingFont, "WELCOME", kPrimaryTextColor);
    TextTexture paragraphLine1 = CreateTextTexture(
        renderer,
        paragraphFont,
        "Lorem ipsum dolor sit amet, consectetur adipiscing",
        kMutedTextColor);
    TextTexture paragraphLine2 = CreateTextTexture(
        renderer,
        paragraphFont,
        "elit, sed do eiusmod tempor incididunt ut labore",
        kMutedTextColor);
    TextTexture paragraphLine3 =
        CreateTextTexture(renderer, paragraphFont, "et dolore magna aliqua.", kMutedTextColor);
    TextTexture launchText = CreateTextTexture(renderer, buttonFont, "LAUNCH", kAccentColor);

    auto destroyTextResources = [&]() {
        DestroyTextTexture(brandText);
        for (auto& text : navigationTexts)
        {
            DestroyTextTexture(text);
        }
        DestroyTextTexture(welcomeText);
        DestroyTextTexture(paragraphLine1);
        DestroyTextTexture(paragraphLine2);
        DestroyTextTexture(paragraphLine3);
        DestroyTextTexture(launchText);

        if (brandFont != nullptr)
            TTF_CloseFont(brandFont);
        if (navFont != nullptr)
            TTF_CloseFont(navFont);
        if (headingFont != nullptr)
            TTF_CloseFont(headingFont);
        if (paragraphFont != nullptr)
            TTF_CloseFont(paragraphFont);
        if (buttonFont != nullptr)
            TTF_CloseFont(buttonFont);
    };

    bool textCreationFailed = brandText.texture == nullptr || welcomeText.texture == nullptr || paragraphLine1.texture == nullptr
        || paragraphLine2.texture == nullptr || paragraphLine3.texture == nullptr || launchText.texture == nullptr;
    for (const auto& navigationText : navigationTexts)
    {
        if (navigationText.texture == nullptr)
        {
            textCreationFailed = true;
            break;
        }
    }

    if (textCreationFailed)
    {
        std::cerr << "Failed to create UI text resources." << '\n';
        destroyTextResources();
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

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
        }

        int outputWidth = 0;
        int outputHeight = 0;
        SDL_GetRendererOutputSize(renderer, &outputWidth, &outputHeight);

        SDL_SetRenderDrawColor(renderer, kBackgroundColor.r, kBackgroundColor.g, kBackgroundColor.b, kBackgroundColor.a);
        SDL_RenderClear(renderer);

        const int sidebarWidth = static_cast<int>(std::clamp(outputWidth / 4, 220, 280));
        const int contentPadding = 48;

        SDL_Rect sidebarRect{0, 0, sidebarWidth, outputHeight};
        SDL_SetRenderDrawColor(renderer, kSidebarColor.r, kSidebarColor.g, kSidebarColor.b, kSidebarColor.a);
        SDL_RenderFillRect(renderer, &sidebarRect);

        SDL_SetRenderDrawColor(renderer, kAccentColor.r, kAccentColor.g, kAccentColor.b, kAccentColor.a);
        SDL_RenderDrawLine(renderer, sidebarWidth, 0, sidebarWidth, outputHeight);

        SDL_Rect brandRect{contentPadding / 2, contentPadding, brandText.width, brandText.height};
        SDL_RenderCopy(renderer, brandText.texture, nullptr, &brandRect);

        int navY = brandRect.y + brandRect.h + 48;
        const int navSpacing = 36;
        for (std::size_t i = 0; i < navigationTexts.size(); ++i)
        {
            SDL_Rect navRect{contentPadding / 2, navY, navigationTexts[i].width, navigationTexts[i].height};
            SDL_RenderCopy(renderer, navigationTexts[i].texture, nullptr, &navRect);
            if (i == 0)
            {
                SDL_RenderDrawLine(renderer, navRect.x, navRect.y + navRect.h + 6, navRect.x + navRect.w, navRect.y + navRect.h + 6);
            }
            navY += navSpacing;
        }

        const int contentStartX = sidebarWidth + contentPadding;
        const int contentWidth = outputWidth - contentStartX - contentPadding;

        const int timelineY = contentPadding + 8;
        const int timelineStartX = contentStartX;
        const int timelineEndX = contentStartX + contentWidth - 120;
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(renderer, timelineStartX, timelineY, timelineEndX, timelineY);
        SDL_Rect timelineAccent{timelineEndX, timelineY - 3, 12, 12};
        SDL_SetRenderDrawColor(renderer, kAccentColor.r, kAccentColor.g, kAccentColor.b, kAccentColor.a);
        SDL_RenderFillRect(renderer, &timelineAccent);

        SDL_Rect welcomeRect{contentStartX, timelineY + 72, welcomeText.width, welcomeText.height};
        SDL_RenderCopy(renderer, welcomeText.texture, nullptr, &welcomeRect);

        SDL_Rect paragraphRect1{contentStartX, welcomeRect.y + welcomeRect.h + 32, paragraphLine1.width, paragraphLine1.height};
        SDL_RenderCopy(renderer, paragraphLine1.texture, nullptr, &paragraphRect1);

        SDL_Rect paragraphRect2{contentStartX, paragraphRect1.y + paragraphRect1.h + 8, paragraphLine2.width, paragraphLine2.height};
        SDL_RenderCopy(renderer, paragraphLine2.texture, nullptr, &paragraphRect2);

        SDL_Rect paragraphRect3{contentStartX, paragraphRect2.y + paragraphRect2.h + 8, paragraphLine3.width, paragraphLine3.height};
        SDL_RenderCopy(renderer, paragraphLine3.texture, nullptr, &paragraphRect3);

        SDL_Rect buttonRect{contentStartX, paragraphRect3.y + paragraphRect3.h + 40, 200, 60};
        SDL_SetRenderDrawColor(renderer, 245, 245, 245, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer, &buttonRect);
        SDL_SetRenderDrawColor(renderer, kAccentColor.r, kAccentColor.g, kAccentColor.b, kAccentColor.a);
        SDL_RenderDrawRect(renderer, &buttonRect);

        SDL_Rect buttonTextRect{
            buttonRect.x + (buttonRect.w - launchText.width) / 2,
            buttonRect.y + (buttonRect.h - launchText.height) / 2,
            launchText.width,
            launchText.height};
        SDL_RenderCopy(renderer, launchText.texture, nullptr, &buttonTextRect);

        SDL_RenderPresent(renderer);
    }

    destroyTextResources();
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
