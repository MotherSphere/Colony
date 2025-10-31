#include "colony/installer/ModuleInstaller.hpp"

#include "colony/core/Module.hpp"

#include <functional>
#include <sstream>
#include <unordered_set>

namespace colony::installer {

ModuleInstaller::ModuleInstaller(core::ModuleRegistry &registry,
                                 security::SecurityManager &security,
                                 appcenter::AppCenter &appCenter)
    : registry_(registry), security_(security), appCenter_(appCenter) {}

InstallResult ModuleInstaller::install(const std::string &identifier) {
    std::unordered_set<std::string> visiting;

    std::function<InstallResult(const std::string &)> installRecursive =
        [&](const std::string &moduleId) -> InstallResult {
        if (registry_.isInstalled(moduleId)) {
            return {true, moduleId + " already installed"};
        }

        if (!visiting.insert(moduleId).second) {
            return {false, "Circular dependency detected for " + moduleId};
        }

        auto manifest = appCenter_.manifest(moduleId);
        if (!manifest) {
            visiting.erase(moduleId);
            return {false, "Manifest not found for " + moduleId};
        }

        for (const auto &dependency : manifest->dependencies) {
            auto dependencyResult = installRecursive(dependency);
            if (!dependencyResult.success) {
                visiting.erase(moduleId);
                return dependencyResult;
            }
        }

        visiting.erase(moduleId);

        if (!registry_.hasFactory(moduleId)) {
            return {false, "No module implementation registered for " + moduleId};
        }

        security::PermissionSet requested(manifest->permissions.begin(), manifest->permissions.end());
        security_.requestPermissions(moduleId, requested);

        auto module = registry_.create(moduleId);
        module->initialize();
        module->shutdown();

        registry_.markInstalled(*manifest);
        return {true, moduleId + " installed"};
    };

    return installRecursive(identifier);
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
