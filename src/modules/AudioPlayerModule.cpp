#include "colony/modules/AudioPlayerModule.hpp"

#include <iostream>

namespace colony::modules {

AudioPlayerModule::AudioPlayerModule() {
    metadata_.identifier = "audio_player";
    metadata_.name = "Audio Player";
    metadata_.version = "0.1.0";
    metadata_.description = "Prototype audio player module";
}

const core::ModuleMetadata &AudioPlayerModule::metadata() const {
    return metadata_;
}

void AudioPlayerModule::initialize() {
    if (!initialized_) {
        initialized_ = true;
        std::cout << "[AudioPlayer] Initialized" << std::endl;
    }
}

void AudioPlayerModule::shutdown() {
    if (initialized_) {
        initialized_ = false;
        std::cout << "[AudioPlayer] Shutdown" << std::endl;
    }
}

} // namespace colony::modules
