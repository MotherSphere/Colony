#include "programs/archive/ArchiveApp.hpp"

#include "programs/archive/ArchiveStateMachine.hpp"

#include "core/localization_manager.hpp"
#include "utils/preferences.hpp"

#include <SDL.h>
#include <SDL_ttf.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#if __has_include(<imgui.h>)
#include <imgui.h>
#define COLONY_ARCHIVE_HAS_IMGUI 1
#else
#define COLONY_ARCHIVE_HAS_IMGUI 0
#endif

namespace colony::programs::archive
{

namespace
{
constexpr char kArchiveProgramId[] = "ARCHIVE_VAULT";

void EnsureSdlVideoInitialized()
{
    if (SDL_WasInit(SDL_INIT_VIDEO) == 0)
    {
        throw std::runtime_error{"ArchiveApp requires the SDL video subsystem to be initialized"};
    }
    if (TTF_WasInit() == 0)
    {
        throw std::runtime_error{"ArchiveApp requires the SDL_ttf subsystem to be initialized"};
    }
}

void PrintLaunchHeader()
{
    std::cout << "[Archive] Launching Archive Vault module..." << '\n';
}

void PrintLaunchFooter()
{
    std::cout << "[Archive] Archive Vault initialized." << '\n';
}
} // namespace

ArchiveApp::ArchiveApp() = default;
ArchiveApp::~ArchiveApp() = default;

void ArchiveApp::Launch(const programs::LaunchContext& context)
{
    EnsureSdlSubsystemsReady();

    if (!initialized_)
    {
        PrintLaunchHeader();
        InitializeImGui();
        ConfigureLocalization(context);
        SynchronizePreferences(context);
        ConfigureStateMachine(context);
        initialized_ = true;
        PrintLaunchFooter();
    }

    PumpOnce();
}

void ArchiveApp::EnsureSdlSubsystemsReady() const
{
    EnsureSdlVideoInitialized();
}

void ArchiveApp::InitializeImGui()
{
    if (imgui_initialized_)
    {
        return;
    }

#if COLONY_ARCHIVE_HAS_IMGUI
    if (ImGui::GetCurrentContext() == nullptr)
    {
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
    }
#endif

    imgui_initialized_ = true;
}

void ArchiveApp::ConfigureLocalization(const programs::LaunchContext& context)
{
    if (context.localization == nullptr)
    {
        return;
    }

    auto* localization = context.localization;
    const std::string currentLanguage = localization->ActiveLanguage();

    if (!currentLanguage.empty())
    {
        return;
    }

    if (context.preferences != nullptr && !context.preferences->languageId.empty())
    {
        const std::string preferred = context.preferences->languageId;
        if (!localization->LoadLanguage(preferred))
        {
            std::cerr << "[Archive] Unable to load preferred language '" << preferred << "'." << '\n';
        }
    }

    if (localization->ActiveLanguage().empty())
    {
        localization->LoadLanguage(localization->FallbackLanguage());
    }
}

void ArchiveApp::SynchronizePreferences(const programs::LaunchContext& context)
{
    if (context.preferences == nullptr)
    {
        return;
    }

    auto* prefs = context.preferences;
    prefs->lastProgramId = kArchiveProgramId;

    auto [_, inserted] = prefs->toggleStates.emplace("archive.onboarding_complete", false);
    if (inserted)
    {
        event_bus_.Publish(MakeToastEvent("Welcome to Archive Vault"));
    }
}

void ArchiveApp::ConfigureStateMachine(const programs::LaunchContext& context)
{
    if (state_machine_ != nullptr)
    {
        return;
    }

    ArchiveStateMachine::Dependencies dependencies{
        .localization = context.localization,
        .preferences = context.preferences,
        .eventBus = &event_bus_,
    };

    state_machine_ = std::make_unique<ArchiveStateMachine>(dependencies);
    state_machine_->SetStateChangedCallback([](ArchiveStateMachine::Screen screen) {
        std::cout << "[Archive] Screen -> " << ToString(screen) << '\n';
    });

    // Kick off the initial flow depending on onboarding progress.
    if (context.preferences != nullptr)
    {
        const bool onboardingComplete = [&]() {
            const auto it = context.preferences->toggleStates.find("archive.onboarding_complete");
            return it != context.preferences->toggleStates.end() && it->second;
        }();

        if (onboardingComplete)
        {
            event_bus_.Publish({.type = ArchiveEventType::PromptUnlock});
        }
        else
        {
            event_bus_.Publish({.type = ArchiveEventType::BeginOnboarding});
        }
    }
}

void ArchiveApp::PumpOnce()
{
    if (state_machine_ == nullptr)
    {
        return;
    }

    state_machine_->Update();
    state_machine_->Render();

    for (const auto& toast : state_machine_->ConsumeToasts())
    {
        std::cout << "[Archive][Toast] " << toast << '\n';
    }
    for (const auto& notification : state_machine_->ConsumeNotifications())
    {
        std::cout << "[Archive][Notification] " << notification << '\n';
    }
}

} // namespace colony::programs::archive

