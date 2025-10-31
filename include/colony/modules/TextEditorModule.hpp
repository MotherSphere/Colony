#pragma once

#include "colony/core/Module.hpp"

namespace colony::modules {

class TextEditorModule : public core::Module {
public:
    TextEditorModule();

    const core::ModuleMetadata &metadata() const override;
    void initialize() override;
    void shutdown() override;

private:
    core::ModuleMetadata metadata_;
    bool initialized_{false};
};

} // namespace colony::modules
