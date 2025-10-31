#include "colony/appcenter/AppCenter.hpp"
#include "colony/core/ModuleRegistry.hpp"
#include "colony/installer/ModuleInstaller.hpp"
#include "colony/marketplace/MarketplaceClient.hpp"
#include "colony/modules/AudioPlayerModule.hpp"
#include "colony/modules/ResourceManagerModule.hpp"
#include "colony/modules/TextEditorModule.hpp"
#include "colony/security/SecurityManager.hpp"

#include <filesystem>
#include <iostream>

int main() {
    using namespace colony;

    core::ModuleRegistry registry;
    security::SecurityManager security;
    appcenter::AppCenter appCenter;
    installer::ModuleInstaller installer(registry, security, appCenter);
    marketplace::MarketplaceClient marketplace;

    registry.registerFactory("text_editor", []() {
        return std::make_unique<modules::TextEditorModule>();
    });
    registry.registerFactory("audio_player", []() {
        return std::make_unique<modules::AudioPlayerModule>();
    });
    registry.registerFactory("resource_manager", []() {
        return std::make_unique<modules::ResourceManagerModule>();
    });

    const std::filesystem::path manifestsDir{"manifests"};
    appCenter.loadFromDirectory(manifestsDir);

    std::cout << "=== Local Catalog ===" << std::endl;
    for (const auto &manifest : appCenter.catalog()) {
        std::cout << "- " << manifest.name << " (" << manifest.identifier << ") v" << manifest.version
                  << "\n  Permissions: ";
        for (const auto &permission : manifest.permissions) {
            std::cout << permission << ' ';
        }
        std::cout << "\n  Dependencies: ";
        for (const auto &dependency : manifest.dependencies) {
            std::cout << dependency << ' ';
        }
        std::cout << "\n";
    }

    std::cout << "\n=== Installing from App Center ===" << std::endl;
    for (const auto &manifest : appCenter.catalog()) {
        auto result = installer.install(manifest.identifier);
        std::cout << (result.success ? "[OK] " : "[ERR] ") << result.message << std::endl;
    }

    std::map<std::string, std::string> installedVersions;
    for (const auto &entry : registry.installedManifests()) {
        installedVersions.emplace(entry.first, entry.second.version);
    }

    for (const auto &entry : appCenter.catalog()) {
        marketplace.publish({entry.identifier, entry.version, "local", entry});
    }

    if (auto manifest = appCenter.manifest("text_editor")) {
        auto updatedManifest = *manifest;
        updatedManifest.version = "0.2.0";
        marketplace.publish({updatedManifest.identifier, updatedManifest.version, "remote", updatedManifest});
    }

    std::cout << "\n=== Marketplace Catalog ===" << std::endl;
    for (const auto &package : marketplace.availablePackages()) {
        std::cout << "- " << package.manifest.name << " v" << package.version << " (" << package.source
                  << ")" << std::endl;
    }

    std::cout << "\n=== Available Updates ===" << std::endl;
    auto updates = marketplace.availableUpdates(installedVersions);
    if (updates.empty()) {
        std::cout << "No updates available" << std::endl;
    } else {
        for (const auto &update : updates) {
            std::cout << "* " << update.identifier << " -> " << update.version << std::endl;
        }
    }

    return 0;
}
