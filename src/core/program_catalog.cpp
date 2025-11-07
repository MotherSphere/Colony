#include "core/program_catalog.hpp"

#include "json.hpp"

#include <fstream>
#include <stdexcept>
#include <utility>

namespace colony
{

namespace
{
constexpr const char* kProgramsKey = "programs";
constexpr const char* kIdKey = "id";
constexpr const char* kLauncherKey = "launcher";

nlohmann::json LoadDocument(const std::filesystem::path& path)
{
    std::ifstream input{path};
    if (!input.is_open())
    {
        throw std::runtime_error("Unable to open program catalog: " + path.string());
    }

    return nlohmann::json::parse(input);
}
} // namespace

void ProgramCatalog::Add(ProgramModuleDescriptor descriptor)
{
    const std::string id = descriptor.id;
    modules_[id] = std::move(descriptor);
}

const ProgramModuleDescriptor* ProgramCatalog::Find(const std::string& id) const
{
    if (auto it = modules_.find(id); it != modules_.end())
    {
        return &it->second;
    }
    return nullptr;
}

ProgramCatalog LoadProgramCatalog(const std::filesystem::path& path)
{
    ProgramCatalog catalog;

    if (path.empty())
    {
        return catalog;
    }

    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error))
    {
        return catalog;
    }

    const nlohmann::json document = LoadDocument(path);
    if (!document.contains(kProgramsKey) || !document[kProgramsKey].is_array())
    {
        throw std::runtime_error("Program catalog must contain an array under the 'programs' key.");
    }

    for (const auto& entry : document[kProgramsKey])
    {
        if (!entry.is_object())
        {
            continue;
        }

        if (!entry.contains(kIdKey) || !entry[kIdKey].is_string() || entry[kIdKey].get<std::string>().empty())
        {
            continue;
        }
        if (!entry.contains(kLauncherKey) || !entry[kLauncherKey].is_string() || entry[kLauncherKey].get<std::string>().empty())
        {
            continue;
        }

        ProgramModuleDescriptor descriptor;
        descriptor.id = entry[kIdKey].get<std::string>();
        descriptor.launcher = entry[kLauncherKey].get<std::string>();
        if (auto executableIt = entry.find("executable"); executableIt != entry.end() && executableIt->is_string())
        {
            descriptor.executable = std::filesystem::path{executableIt->get<std::string>()};
        }
        catalog.Add(std::move(descriptor));
    }

    return catalog;
}

} // namespace colony

