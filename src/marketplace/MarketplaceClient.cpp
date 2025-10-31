#include "colony/marketplace/MarketplaceClient.hpp"

#include <algorithm>

namespace colony::marketplace {

void MarketplaceClient::publish(const PackageInfo &package) {
    packages_[package.identifier] = package;
}

std::vector<PackageInfo> MarketplaceClient::availablePackages() const {
    std::vector<PackageInfo> result;
    result.reserve(packages_.size());
    for (const auto &pair : packages_) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<PackageInfo> MarketplaceClient::availableUpdates(const std::map<std::string, std::string> &installed) const {
    std::vector<PackageInfo> result;

    for (const auto &pair : packages_) {
        const auto &identifier = pair.first;
        const auto &package = pair.second;
        auto it = installed.find(identifier);
        if (it == installed.end()) {
            continue;
        }

        if (it->second != package.version) {
            result.push_back(package);
        }
    }

    return result;
}

} // namespace colony::marketplace
