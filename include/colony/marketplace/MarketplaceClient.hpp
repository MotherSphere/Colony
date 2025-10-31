#pragma once

#include "colony/core/Manifest.hpp"

#include <map>
#include <string>
#include <vector>

namespace colony::marketplace {

struct PackageInfo {
    std::string identifier;
    std::string version;
    std::string source;
    colony::core::Manifest manifest;
};

class MarketplaceClient {
public:
    void publish(const PackageInfo &package);

    [[nodiscard]] std::vector<PackageInfo> availablePackages() const;
    [[nodiscard]] std::vector<PackageInfo> availableUpdates(const std::map<std::string, std::string> &installed) const;

private:
    std::map<std::string, PackageInfo> packages_;
};

} // namespace colony::marketplace
