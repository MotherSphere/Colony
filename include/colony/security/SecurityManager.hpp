#pragma once

#include "colony/core/Manifest.hpp"
#include "colony/security/Permissions.hpp"

#include <map>
#include <string>

namespace colony::security {

class SecurityManager {
public:
    // For the prototype we automatically grant requested permissions but record them.
    PermissionSet requestPermissions(const std::string &moduleIdentifier,
                                     const PermissionSet &requested);

    [[nodiscard]] bool hasPermission(const std::string &moduleIdentifier,
                                     const std::string &permission) const;

    void revokeAll(const std::string &moduleIdentifier);

private:
    std::map<std::string, PermissionSet> grants_;
};

} // namespace colony::security
