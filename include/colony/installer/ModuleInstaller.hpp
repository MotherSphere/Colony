#pragma once

#include "colony/appcenter/AppCenter.hpp"
#include "colony/core/ModuleRegistry.hpp"
#include "colony/security/SecurityManager.hpp"

#include <optional>
#include <string>

namespace colony::installer {

struct InstallResult {
    bool success{false};
    std::string message;
};

class ModuleInstaller {
public:
    ModuleInstaller(core::ModuleRegistry &registry,
                    security::SecurityManager &security,
                    appcenter::AppCenter &appCenter);

    InstallResult install(const std::string &identifier);
    InstallResult uninstall(const std::string &identifier);

private:
    core::ModuleRegistry &registry_;
    security::SecurityManager &security_;
    appcenter::AppCenter &appCenter_;
};

} // namespace colony::installer
