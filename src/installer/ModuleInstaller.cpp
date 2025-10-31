#include "colony/installer/ModuleInstaller.hpp"

#include "colony/core/Module.hpp"

#include <sstream>

namespace colony::installer {

ModuleInstaller::ModuleInstaller(core::ModuleRegistry &registry,
                                 security::SecurityManager &security,
                                 appcenter::AppCenter &appCenter)
    : registry_(registry), security_(security), appCenter_(appCenter) {}

InstallResult ModuleInstaller::install(const std::string &identifier) {
    if (registry_.isInstalled(identifier)) {
        return {false, identifier + " is already installed"};
    }

    auto manifest = appCenter_.manifest(identifier);
    if (!manifest) {
        return {false, "Manifest not found for " + identifier};
    }

    for (const auto &dependency : manifest->dependencies) {
        if (!registry_.isInstalled(dependency)) {
            std::ostringstream oss;
            oss << "Missing dependency '" << dependency << "' for module '" << identifier << "'";
            return {false, oss.str()};
        }
    }

    if (!registry_.hasFactory(identifier)) {
        return {false, "No module implementation registered for " + identifier};
    }

    security::PermissionSet requested(manifest->permissions.begin(), manifest->permissions.end());
    security_.requestPermissions(identifier, requested);

    auto module = registry_.create(identifier);
    module->initialize();
    module->shutdown();

    registry_.markInstalled(*manifest);
    return {true, identifier + " installed"};
}

InstallResult ModuleInstaller::uninstall(const std::string &identifier) {
    if (!registry_.isInstalled(identifier)) {
        return {false, identifier + " is not installed"};
    }

    try {
        if (registry_.hasFactory(identifier)) {
            auto module = registry_.create(identifier);
            module->shutdown();
        }
    } catch (...) {
        // Ignore errors while shutting down for this prototype.
    }

    registry_.markUninstalled(identifier);
    security_.revokeAll(identifier);

    return {true, identifier + " uninstalled"};
}

} // namespace colony::installer
