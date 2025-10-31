#pragma once

#include "colony/core/Manifest.hpp"
#include "colony/core/Module.hpp"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>

namespace colony::core {

class ModuleRegistry {
public:
    using ModuleFactory = std::function<std::unique_ptr<Module>()>;

    void registerFactory(const std::string &identifier, ModuleFactory factory);
    [[nodiscard]] bool hasFactory(const std::string &identifier) const;

    std::unique_ptr<Module> create(const std::string &identifier) const;

    void markInstalled(const Manifest &manifest);
    void markUninstalled(const std::string &identifier);

    [[nodiscard]] bool isInstalled(const std::string &identifier) const;
    [[nodiscard]] std::optional<Manifest> manifestFor(const std::string &identifier) const;
    [[nodiscard]] std::map<std::string, Manifest> installedManifests() const;

private:
    std::map<std::string, ModuleFactory> factories_;
    std::map<std::string, Manifest> installed_;
};

} // namespace colony::core
