#include "colony/core/ModuleRegistry.hpp"

#include <stdexcept>

namespace colony::core {

void ModuleRegistry::registerFactory(const std::string &identifier, ModuleFactory factory) {
    factories_[identifier] = std::move(factory);
}

bool ModuleRegistry::hasFactory(const std::string &identifier) const {
    return factories_.find(identifier) != factories_.end();
}

std::unique_ptr<Module> ModuleRegistry::create(const std::string &identifier) const {
    auto it = factories_.find(identifier);
    if (it == factories_.end()) {
        throw std::runtime_error("No factory registered for module: " + identifier);
    }

    return (it->second)();
}

void ModuleRegistry::markInstalled(const Manifest &manifest) {
    installed_[manifest.identifier] = manifest;
}

void ModuleRegistry::markUninstalled(const std::string &identifier) {
    installed_.erase(identifier);
}

bool ModuleRegistry::isInstalled(const std::string &identifier) const {
    return installed_.find(identifier) != installed_.end();
}

std::optional<Manifest> ModuleRegistry::manifestFor(const std::string &identifier) const {
    auto it = installed_.find(identifier);
    if (it == installed_.end()) {
        return std::nullopt;
    }

    return it->second;
}

std::map<std::string, Manifest> ModuleRegistry::installedManifests() const {
    return installed_;
}

} // namespace colony::core
