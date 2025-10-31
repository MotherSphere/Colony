#pragma once

#include <string>

namespace colony::core {

struct ModuleMetadata {
    std::string identifier;
    std::string name;
    std::string version;
    std::string description;
};

class Module {
public:
    virtual ~Module() = default;

    virtual const ModuleMetadata &metadata() const = 0;
    virtual void initialize() = 0;
    virtual void shutdown() = 0;
};

} // namespace colony::core
