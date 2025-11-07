#pragma once

#include <functional>
#include <string>

namespace colony::programs::archive {

// Central coordinator for Archive Vault screens and transitions.
class ArchiveStateMachine {
public:
    void update();
    void render();
    void request_transition(const std::string& route);
};

} // namespace colony::programs::archive
