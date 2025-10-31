#include "colony/modules/TextEditorModule.hpp"

#include <iostream>

namespace colony::modules {

TextEditorModule::TextEditorModule() {
    metadata_.identifier = "text_editor";
    metadata_.name = "Text Editor";
    metadata_.version = "0.1.0";
    metadata_.description = "Prototype text editor module";
}

const core::ModuleMetadata &TextEditorModule::metadata() const {
    return metadata_;
}

void TextEditorModule::initialize() {
    if (!initialized_) {
        initialized_ = true;
        std::cout << "[TextEditor] Initialized" << std::endl;
    }
}

void TextEditorModule::shutdown() {
    if (initialized_) {
        initialized_ = false;
        std::cout << "[TextEditor] Shutdown" << std::endl;
    }
}

} // namespace colony::modules
