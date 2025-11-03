#include "utils/font_manager.hpp"

#include "cpp-httplib/httplib.h"

#include <SDL2/SDL.h>

#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <system_error>
#include <vector>

namespace colony::fonts
{
namespace
{
constexpr char kFontFileName[] = "DejaVuSans.ttf";
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

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    std::unique_ptr<httplib::Client> client;
    if (scheme == "https")
    {
        client.reset(new httplib::SSLClient(host));
    }
    else if (scheme == "http")
    {
        client = std::make_unique<httplib::Client>(host);
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
    std::unique_ptr<httplib::Client> client = std::make_unique<httplib::Client>(host);
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

    candidates.emplace_back(BundledFontDestination());
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

} // namespace colony::fonts
