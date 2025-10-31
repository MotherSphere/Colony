#include "colony/security/SecurityManager.hpp"

namespace colony::security {

PermissionSet SecurityManager::requestPermissions(const std::string &moduleIdentifier,
                                                  const PermissionSet &requested) {
    auto &granted = grants_[moduleIdentifier];
    granted.insert(requested.begin(), requested.end());
    return granted;
}

bool SecurityManager::hasPermission(const std::string &moduleIdentifier,
                                    const std::string &permission) const {
    auto it = grants_.find(moduleIdentifier);
    if (it == grants_.end()) {
        return false;
    }

    return it->second.find(permission) != it->second.end();
}

void SecurityManager::revokeAll(const std::string &moduleIdentifier) {
    grants_.erase(moduleIdentifier);
}

} // namespace colony::security
