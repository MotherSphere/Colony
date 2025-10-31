#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace colony::core {

struct Manifest {
    std::string identifier;
    std::string name;
    std::string version;
    std::string description;
    std::vector<std::string> permissions;
    std::vector<std::string> dependencies;

    static std::optional<Manifest> fromFile(const std::filesystem::path &path);
};

} // namespace colony::core
