#pragma once

#include <memory>
#include <string>

namespace colony::programs::archive {

class ArchiveStateMachine;

//! Entry point for the Archive Vault program.
class ArchiveApp {
public:
    ArchiveApp();
    ~ArchiveApp();

    // Initializes the Archive Vault subsystems and enters the UI loop.
    void launch();

private:
    void initialize_services();
    void configure_state_machine();

    std::unique_ptr<ArchiveStateMachine> state_machine_;
    bool initialized_ = false;
};

} // namespace colony::programs::archive
