#include "colony/modules/ResourceManagerModule.hpp"

#include <iostream>

namespace colony::modules {

ResourceManagerModule::ResourceManagerModule() {
    metadata_.identifier = "resource_manager";
    metadata_.name = "Resource Manager";
    metadata_.version = "0.1.0";
    metadata_.description = "Prototype resource manager module";
}

const core::ModuleMetadata &ResourceManagerModule::metadata() const {
    return metadata_;
}

void ResourceManagerModule::initialize() {
    if (!initialized_) {
        initialized_ = true;
        std::cout << "[ResourceManager] Initialized" << std::endl;
    }
}

void ResourceManagerModule::shutdown() {
    if (initialized_) {
        initialized_ = false;
        std::cout << "[ResourceManager] Shutdown" << std::endl;
    }
}

} // namespace colony::modules
