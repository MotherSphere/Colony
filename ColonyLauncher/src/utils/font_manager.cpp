#include "utils/font_manager.hpp"
#include "utils/asset_paths.hpp"

#include "cpp-httplib/httplib.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string_view>
#include <system_error>
#include <vector>

namespace colony::fonts
{
namespace
{
constexpr char kBundledFontDirectory[] = "assets/fonts";
constexpr char kJetBrainsFontSubdirectory[] = "JetBrainsMono";
constexpr char kPrimaryFontRelativePath[] = "JetBrainsMono-Regular.ttf";
constexpr char kFontDownloadUrl[] =
    "https://raw.githubusercontent.com/JetBrains/JetBrainsMono/master/fonts/ttf/JetBrainsMono-Regular.ttf";
constexpr char kDevanagariFontRelativePath[] =
    "Noto_Sans_Devanagari/static/NotoSansDevanagari-Regular.ttf";
constexpr char kCjkFontRelativePath[] = "NotoSansCJK-Regular.ttc";
constexpr char kArabicFontRelativePath[] = "NotoSansArabic/NotoSansArabic-Regular.ttf";

const std::array<std::filesystem::path, 7> kSystemFontCandidates{
    std::filesystem::path{"/usr/share/fonts/truetype/jetbrains-mono/JetBrainsMono-Regular.ttf"},
    std::filesystem::path{"/usr/share/fonts/truetype/jetbrains-mono/JetBrainsMonoNL-Regular.ttf"},
    std::filesystem::path{"/usr/share/fonts/truetype/nerd-fonts/JetBrainsMono-Regular.ttf"},
    std::filesystem::path{"/usr/share/fonts/truetype/nerd-fonts/JetBrainsMonoNLNerdFont-Regular.ttf"},
    std::filesystem::path{"/Library/Fonts/JetBrainsMono-Regular.ttf"},
    std::filesystem::path{"/Library/Fonts/JetBrainsMonoNL-Regular.ttf"},
    std::filesystem::path{"/Library/Fonts/JetBrainsMonoNLNerdFont-Regular.ttf"}};

constexpr std::array<std::string_view, 3> kLegacyPrimaryFontRelativePaths{
    "JetBrainsMonoNLNerdFont-Regular.ttf",
    "JetBrainsMonoNL-Regular.ttf",
    "JetBrainsMonoNLNerdFont-Regular.otf"};

std::filesystem::path BundledFontDestination()
{
    const std::filesystem::path bundledRoot = colony::paths::ResolveAssetDirectory(kBundledFontDirectory);
    return bundledRoot / kJetBrainsFontSubdirectory / kPrimaryFontRelativePath;
}

std::filesystem::path ResolveBundledFontInternal(std::string_view relativePath)
{
    const std::filesystem::path relative{relativePath};
    const bool hasSubdirectory = relative.has_parent_path() && relative.parent_path() != std::filesystem::path{};
    std::vector<std::filesystem::path> directCandidates;

    const auto appendIfUnique = [](std::vector<std::filesystem::path>& container, std::filesystem::path path) {
        if (path.empty())
        {
            return;
        }

        if (std::find(container.begin(), container.end(), path) == container.end())
        {
            container.emplace_back(std::move(path));
        }
    };

    const std::filesystem::path bundledFonts = colony::paths::ResolveAssetDirectory(kBundledFontDirectory);

    std::filesystem::path basePath;
    if (char* rawBasePath = SDL_GetBasePath(); rawBasePath != nullptr)
    {
        basePath = std::filesystem::path{rawBasePath};
        SDL_free(rawBasePath);
    }

    std::vector<std::filesystem::path> assetRoots;
    if (!basePath.empty())
    {
        appendIfUnique(assetRoots, basePath);
        appendIfUnique(assetRoots, basePath / kBundledFontDirectory);

        const std::filesystem::path baseParent = basePath.parent_path();
        if (!baseParent.empty())
        {
            appendIfUnique(assetRoots, baseParent);
            appendIfUnique(assetRoots, baseParent / kBundledFontDirectory);
        }
    }

    if (!bundledFonts.empty())
    {
        appendIfUnique(assetRoots, bundledFonts);
        appendIfUnique(assetRoots, bundledFonts.parent_path());
    }

    appendIfUnique(assetRoots, std::filesystem::path{kBundledFontDirectory});
    appendIfUnique(assetRoots, std::filesystem::path{"fonts"});

    for (const auto& root : assetRoots)
    {
        appendIfUnique(directCandidates, root / relative);
        appendIfUnique(directCandidates, root / kJetBrainsFontSubdirectory / relative);

        if (!hasSubdirectory)
        {
            appendIfUnique(directCandidates, root / relative.filename());
            appendIfUnique(directCandidates, root / kJetBrainsFontSubdirectory / relative.filename());
        }
    }

    appendIfUnique(directCandidates, relative);
    appendIfUnique(directCandidates, std::filesystem::path{"fonts"} / relative);

    if (!hasSubdirectory)
    {
        appendIfUnique(directCandidates, std::filesystem::path{"fonts"} / relative.filename());
    }

    for (const auto& candidate : directCandidates)
    {
        std::error_code error;
        if (!candidate.empty() && std::filesystem::exists(candidate, error))
        {
            return candidate;
        }
    }

    const std::string filename = relative.filename().string();
    if (filename.empty())
    {
        return {};
    }

    auto normalizeFilename = [](std::string_view name) {
        std::string normalized;
        normalized.reserve(name.size());
        for (char ch : name)
        {
            if (std::isalnum(static_cast<unsigned char>(ch)))
            {
                normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
            }
        }
        return normalized;
    };

    const std::string normalizedFilename = normalizeFilename(filename);
    if (normalizedFilename.empty())
    {
        return {};
    }

    std::vector<std::filesystem::path> searchDirectories;
    for (const auto& root : assetRoots)
    {
        appendIfUnique(searchDirectories, root);
        appendIfUnique(searchDirectories, root / kJetBrainsFontSubdirectory);
    }

    for (const auto& directory : searchDirectories)
    {
        std::error_code error;
        if (!std::filesystem::exists(directory, error) || !std::filesystem::is_directory(directory, error))
        {
            continue;
        }

        std::filesystem::recursive_directory_iterator it{directory, error};
        std::filesystem::recursive_directory_iterator end{};
        for (; it != end && !error; it.increment(error))
        {
            std::error_code statusError;
            if (!it->is_regular_file(statusError))
            {
                continue;
            }

            if (normalizeFilename(it->path().filename().string()) == normalizedFilename)
            {
                return it->path();
            }
        }
    }

    return {};
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

bool DownloadToFile(const std::string& url, const std::filesystem::path& destination)
{
    const auto protocolSeparator = url.find("://");
    if (protocolSeparator == std::string::npos)
    {
        return false;
    }

    const std::string scheme = url.substr(0, protocolSeparator);
    std::string remainder = url.substr(protocolSeparator + 3);
    const auto pathSeparator = remainder.find('/');
    std::string host;
    std::string path = "/";
    if (pathSeparator == std::string::npos)
    {
        host = std::move(remainder);
    }
    else
    {
        host = remainder.substr(0, pathSeparator);
        path = remainder.substr(pathSeparator);
        if (path.empty())
        {
            path = "/";
        }
    }

    std::unique_ptr<httplib::Client> client;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    if (scheme == "https" || scheme == "http")
    {
        client = std::make_unique<httplib::Client>(scheme + "://" + host);
    }
    else
    {
        return false;
    }
#else
    if (scheme != "http")
    {
        return false;
    }
    client = std::make_unique<httplib::Client>(host);
#endif

    client->set_follow_location(true);
    client->set_read_timeout(20, 0);
    client->set_connection_timeout(20, 0);

    const auto response = client->Get(path.c_str());
    if (!response || response->status >= 400)
    {
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(destination.parent_path(), ec);
    if (ec)
    {
        return false;
    }

    std::ofstream output(destination, std::ios::binary);
    if (!output.is_open())
    {
        return false;
    }

    output.write(response->body.data(), static_cast<std::streamsize>(response->body.size()));
    output.close();
    return output.good();
}

} // namespace

std::filesystem::path ResolveBundledFont(std::string_view relativePath)
{
    return ResolveBundledFontInternal(relativePath);
}

std::filesystem::path GetBundledFontPath()
{
    return BundledFontDestination();
}

bool EnsureBundledFontAvailable()
{
    const std::filesystem::path bundledPath = BundledFontDestination();
    const std::filesystem::path bundledRoot = colony::paths::ResolveAssetDirectory(kBundledFontDirectory);
    std::error_code error;
    if (std::filesystem::exists(bundledPath, error))
    {
        return true;
    }

    const std::array<std::filesystem::path, 2> legacyDirectories{
        bundledRoot,
        bundledRoot / kJetBrainsFontSubdirectory};

    for (const auto& legacyRelativePath : kLegacyPrimaryFontRelativePaths)
    {
        for (const auto& legacyDirectory : legacyDirectories)
        {
            if (CopyFontIfPresent(legacyDirectory / legacyRelativePath, bundledPath))
            {
                return true;
            }
        }
    }

    for (const auto& candidate : kSystemFontCandidates)
    {
        if (CopyFontIfPresent(candidate, bundledPath))
        {
            return true;
        }
    }

    if (!DownloadToFile(kFontDownloadUrl, bundledPath))
    {
        std::cerr << "Unable to download font from " << kFontDownloadUrl << "." << std::endl;
        return false;
    }

    return std::filesystem::exists(bundledPath, error);
}

FontConfiguration BuildFontConfiguration(std::string_view activeLanguageId)
{
    FontConfiguration configuration{};

    if (const char* envFontPath = std::getenv("COLONY_FONT_PATH"); envFontPath != nullptr)
    {
        std::filesystem::path envPath{envFontPath};
        std::error_code envError;
        if (std::filesystem::exists(envPath, envError))
        {
            configuration.primaryFontPath = envPath.string();
        }
        else
        {
            std::cerr << "Environment variable COLONY_FONT_PATH is set to '" << envFontPath
                      << "', but the file could not be found. Falling back to defaults.\n";
        }
    }

    if (configuration.primaryFontPath.empty())
    {
        EnsureBundledFontAvailable();

        const auto resolvePrimary = [&](std::string_view relativePath) {
            const std::filesystem::path path = ResolveBundledFont(relativePath);
            if (!path.empty())
            {
                configuration.primaryFontPath = path.string();
            }
        };

        if (activeLanguageId == "hi")
        {
            resolvePrimary(kDevanagariFontRelativePath);
        }
        else if (activeLanguageId == "zh")
        {
            resolvePrimary(kCjkFontRelativePath);
        }
        else if (activeLanguageId == "ar")
        {
            resolvePrimary(kArabicFontRelativePath);
        }

        if (configuration.primaryFontPath.empty())
        {
            resolvePrimary(kPrimaryFontRelativePath);
        }
    }

    const auto addNativeFont = [&](std::string_view languageId, std::string_view relativePath) {
        const std::filesystem::path path = ResolveBundledFont(relativePath);
        if (!path.empty())
        {
            configuration.nativeLanguageFonts.emplace(languageId, path.string());
        }
    };

    addNativeFont("zh", kCjkFontRelativePath);
    addNativeFont("hi", kDevanagariFontRelativePath);
    addNativeFont("ar", kArabicFontRelativePath);

    return configuration;
}

} // namespace colony::fonts
