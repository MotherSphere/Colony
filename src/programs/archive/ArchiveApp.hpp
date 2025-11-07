#pragma once

#include "programs/archive/ArchiveEvent.hpp"
#include "programs/launch_contract.hpp"

#include <memory>

namespace colony::programs::archive
{

class ArchiveStateMachine;

// Entry point for the Archive Vault program. ArchiveApp wires the SDL
// primitives provided by the Colony shell with Archive Vault specific
// subsystems such as the ImGui runtime, localization, preferences, and the
// internal state machine.
class ArchiveApp : public programs::LaunchContract
{
  public:
    ArchiveApp();
    ~ArchiveApp() override;

    void Launch(const programs::LaunchContext& context) override;

  private:
    void EnsureSdlSubsystemsReady() const;
    void InitializeImGui();
    void ConfigureLocalization(const programs::LaunchContext& context);
    void SynchronizePreferences(const programs::LaunchContext& context);
    void ConfigureStateMachine(const programs::LaunchContext& context);
    void PumpOnce();

    std::unique_ptr<ArchiveStateMachine> state_machine_;
    ArchiveEventBus event_bus_{};
    bool initialized_ = false;
    bool imgui_initialized_ = false;
};

} // namespace colony::programs::archive

