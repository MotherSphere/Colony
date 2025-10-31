#include "colony/core/Manifest.hpp"

#include <fstream>
#include <sstream>

namespace {

std::string extractString(const std::string &data, const std::string &key) {
    const std::string token = "\"" + key + "\"";
    auto keyPos = data.find(token);
    if (keyPos == std::string::npos) {
        return {};
    }

    auto colonPos = data.find(':', keyPos);
    if (colonPos == std::string::npos) {
        return {};
    }

    auto valueStart = data.find('"', colonPos);
    if (valueStart == std::string::npos) {
        return {};
    }

    auto valueEnd = data.find('"', valueStart + 1);
    if (valueEnd == std::string::npos) {
        return {};
    }

    return data.substr(valueStart + 1, valueEnd - valueStart - 1);
}

std::vector<std::string> extractArray(const std::string &data, const std::string &key) {
    std::vector<std::string> result;

    const std::string token = "\"" + key + "\"";
    auto keyPos = data.find(token);
    if (keyPos == std::string::npos) {
        return result;
    }

    auto arrayStart = data.find('[', keyPos);
    if (arrayStart == std::string::npos) {
        return result;
    }

    auto arrayEnd = data.find(']', arrayStart);
    if (arrayEnd == std::string::npos) {
        return result;
    }

    auto content = data.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
    std::size_t current = 0;
    while (true) {
        auto quoteStart = content.find('"', current);
        if (quoteStart == std::string::npos) {
            break;
        }
        auto quoteEnd = content.find('"', quoteStart + 1);
        if (quoteEnd == std::string::npos) {
            break;
        }
        result.push_back(content.substr(quoteStart + 1, quoteEnd - quoteStart - 1));
        current = quoteEnd + 1;
    }

    return result;
}

} // namespace

namespace colony::core {

std::optional<Manifest> Manifest::fromFile(const std::filesystem::path &path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return std::nullopt;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    auto data = buffer.str();

    Manifest manifest;
    manifest.identifier = extractString(data, "identifier");
    manifest.name = extractString(data, "name");
    manifest.version = extractString(data, "version");
    manifest.description = extractString(data, "description");
    manifest.permissions = extractArray(data, "permissions");
    manifest.dependencies = extractArray(data, "dependencies");

    if (manifest.identifier.empty() || manifest.name.empty()) {
        return std::nullopt;
    }

    return manifest;
}

} // namespace colony::core
