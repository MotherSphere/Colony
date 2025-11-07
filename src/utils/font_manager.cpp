#include "utils/font_manager.hpp"

#include "cpp-httplib/httplib.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <system_error>
#include <unordered_set>
#include <vector>

namespace colony::fonts
{
namespace
{
constexpr char kFontFileName[] = "DejaVuSans.ttf";
constexpr std::array<const char*, 3> kSupportedFontExtensions{".ttf", ".otf", ".ttc"};
constexpr char kBundledFontDirectory[] = "assets/fonts";
constexpr char kFontDownloadUrl[] =
    "https://raw.githubusercontent.com/dejavu-fonts/dejavu-fonts/master/ttf/DejaVuSans.ttf";

const std::array<std::filesystem::path, 3> kSystemFontCandidates{
    std::filesystem::path{"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"},
    std::filesystem::path{"/usr/local/share/fonts/DejaVuSans.ttf"},
    std::filesystem::path{"/Library/Fonts/DejaVuSans.ttf"}};

std::filesystem::path BundledFontDestination()
{
    return std::filesystem::path{kBundledFontDirectory} / kFontFileName;
}

bool HasSupportedFontExtension(const std::filesystem::path& path)
{
    const std::string extension = path.extension().string();
    return std::any_of(
        kSupportedFontExtensions.begin(),
        kSupportedFontExtensions.end(),
        [&](const char* candidate) { return extension == candidate; });
}

void AppendFontFilesInDirectory(
    const std::filesystem::path& directory,
    std::vector<std::filesystem::path>& candidates)
{
    std::error_code error;
    if (!std::filesystem::exists(directory, error))
    {
        return;
    }

    std::filesystem::directory_iterator iterator{directory, error};
    if (error)
    {
        return;
    }

    std::vector<std::filesystem::path> discovered;
    for (const auto& entry : iterator)
    {
        std::error_code entryError;
        if (!entry.is_regular_file(entryError))
        {
            continue;
        }

        const auto& path = entry.path();
        if (!HasSupportedFontExtension(path))
        {
            continue;
        }

        discovered.emplace_back(path);
    }

    std::sort(discovered.begin(), discovered.end());

    auto appendMatching = [&](bool expectDejaVu) {
        for (const auto& path : discovered)
        {
            const bool isDejaVu = path.filename() == kFontFileName;
            if (isDejaVu == expectDejaVu)
            {
                candidates.emplace_back(path);
            }
        }
    };

    appendMatching(false);
    appendMatching(true);
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

std::filesystem::path GetBundledFontPath()
{
    return BundledFontDestination();
}

bool EnsureBundledFontAvailable()
{
    const std::filesystem::path bundledPath = BundledFontDestination();
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

    if (!DownloadToFile(kFontDownloadUrl, bundledPath))
    {
        std::cerr << "Unable to download font from " << kFontDownloadUrl << "." << std::endl;
        return false;
    }

    return std::filesystem::exists(bundledPath, error);
}

std::string ResolveFontPath()
{
    std::vector<std::filesystem::path> candidates;

    auto appendIfExists = [&](const std::filesystem::path& path) {
        std::error_code error;
        if (std::filesystem::exists(path, error))
        {
            candidates.emplace_back(path);
        }
    };

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
        AppendFontFilesInDirectory(base / kBundledFontDirectory, candidates);
        appendIfExists(base / kBundledFontDirectory / kFontFileName);
    }

    AppendFontFilesInDirectory(kBundledFontDirectory, candidates);
    AppendFontFilesInDirectory(std::filesystem::path{"fonts"}, candidates);

    appendIfExists(BundledFontDestination());
    appendIfExists(std::filesystem::path{"fonts"} / kFontFileName);
    appendIfExists(std::filesystem::path{kFontFileName});
    candidates.insert(candidates.end(), kSystemFontCandidates.begin(), kSystemFontCandidates.end());

    std::vector<std::filesystem::path> deduplicated;
    std::unordered_set<std::string> seen;
    deduplicated.reserve(candidates.size());
    for (const auto& candidate : candidates)
    {
        const std::string key = candidate.lexically_normal().string();
        if (seen.insert(key).second)
        {
            deduplicated.emplace_back(candidate);
        }
    }

    for (const auto& candidate : deduplicated)
    {
        std::error_code error;
        if (std::filesystem::exists(candidate, error))
        {
            return candidate.string();
        }
    }

    return {};
}

} // namespace colony::fonts
