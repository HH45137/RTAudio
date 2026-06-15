#include <cassert>
#include <iostream>

#include "RTAudio.h"


int main(int argc, char **argv) {
    RTA::AudioBackendOpenAL *oal_backend = new RTA::AudioBackendOpenAL();

    if (!oal_backend->IsReady()) {
        if (!oal_backend->Initialize()) {
            std::runtime_error("Failed to initialize audio backend");
            oal_backend->Shutdown();
            return -1;
        }

        std::cout << "Successd to initialize audio backend\n";
    }

    assert(argc == 2);
    std::string audio_file = std::string(argv[1]);
    std::cout << "audio_file = " << audio_file << std::endl;
    assert(oal_backend->LoadDataFromFile(audio_file));

    oal_backend->Play();

    oal_backend->Shutdown();

    return 0;
}
