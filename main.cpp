#include <cassert>
#include <iostream>

#include "RTAudio.h"


int main(int argc, char **argv) {
    RTA::AudioBackendMiniAudio *audio_backend = new RTA::AudioBackendMiniAudio();

    if (!audio_backend->IsReady()) {
        if (!audio_backend->Initialize()) {
            std::runtime_error("Failed to initialize audio backend");
            audio_backend->Shutdown();
            return -1;
        }

        std::cout << "Successd to initialize audio backend\n";
    }

    assert(argc == 2);
    std::string audio_file = std::string(argv[1]);
    std::cout << "audio_file = " << audio_file << std::endl;
    assert(audio_backend->LoadDataFromFile(audio_file));

    audio_backend->Play();

    audio_backend->Shutdown();

    return 0;
}
