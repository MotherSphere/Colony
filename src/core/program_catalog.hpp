#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace colony
{

struct ProgramModuleDescriptor
{
    std::string id;
    std::string launcher;
};

class ProgramCatalog
{
  public:
    void Add(ProgramModuleDescriptor descriptor);

    [[nodiscard]] const ProgramModuleDescriptor* Find(const std::string& id) const;
    [[nodiscard]] const std::unordered_map<std::string, ProgramModuleDescriptor>& Modules() const noexcept
    {
        return modules_;
    }

  private:
    std::unordered_map<std::string, ProgramModuleDescriptor> modules_{};
};

ProgramCatalog LoadProgramCatalog(const std::filesystem::path& path);

} // namespace colony

