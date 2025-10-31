#include "colony/appcenter/AppCenter.hpp"

#include <filesystem>

namespace colony::appcenter {

void AppCenter::registerManifest(const core::Manifest &manifest) {
    catalog_[manifest.identifier] = manifest;
}

void AppCenter::loadFromDirectory(const std::filesystem::path &directory) {
    if (!std::filesystem::exists(directory)) {
        return;
    }

    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        auto manifest = core::Manifest::fromFile(entry.path());
        if (manifest) {
            registerManifest(*manifest);
        }
    }
}

std::vector<core::Manifest> AppCenter::catalog() const {
    std::vector<core::Manifest> result;
    result.reserve(catalog_.size());
    for (const auto &pair : catalog_) {
        result.push_back(pair.second);
    }
    return result;
}

std::optional<core::Manifest> AppCenter::manifest(const std::string &identifier) const {
    auto it = catalog_.find(identifier);
    if (it == catalog_.end()) {
        return std::nullopt;
    }

    return it->second;
}

} // namespace colony::appcenter
