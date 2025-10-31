#pragma once

#include "colony/core/Manifest.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace colony::appcenter {

class AppCenter {
public:
    void registerManifest(const core::Manifest &manifest);
    void loadFromDirectory(const std::filesystem::path &directory);

    [[nodiscard]] std::vector<core::Manifest> catalog() const;
    [[nodiscard]] std::optional<core::Manifest> manifest(const std::string &identifier) const;

private:
    std::map<std::string, core::Manifest> catalog_;
};

} // namespace colony::appcenter
