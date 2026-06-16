#include <cassert>
#include <iostream>

#include "RTAudio.h"


int main(int argc, char **argv) {
    auto *rta = new RTA::RTAudio();

    if (!rta->IsReady()) {
        if (!rta->Initialize()) {
            throw std::runtime_error("Failed to initialize audio backend");
        }

        std::cout << "Successd to initialize audio backend\n";
    }

    assert(argc == 2);
    std::string audio_file = std::string(argv[1]);
    std::cout << "audio_file = " << audio_file << std::endl;
    assert(rta->LoadDataFromFile(audio_file));

    std::cout << "\n--- Audio loaded successfully ---" << std::endl;
    std::cout << "Total duration: " << rta->GetDurationInMilliseconds() << " ms" << std::endl;
    std::cout << "Control commands: [1] Play/Resume  [2] Pause  [3] Restart from beginning  [4] Exit program" << std::endl;
    std::cout << "-----------------------\n" << std::endl;

    bool running = true;
    while (running) {
        std::cout << "Enter command: ";
        int cmd = 0;
        std::cin >> cmd;

        switch (cmd) {
            case 1:
                std::cout << ">> Playing/Resuming..." << std::endl;
                rta->Play();
                break;
            case 2:
                std::cout << ">> Paused. Enter 1 to resume playback." << std::endl;
                rta->Pause();
                break;
            case 3:
                std::cout << ">> Reset to the start of audio." << std::endl;
                rta->Reset();
                break;
            case 4:
                std::cout << ">> Exiting program..." << std::endl;
                running = false;
                break;
            default:
                std::cout << "Unknown command, please try again!" << std::endl;
                break;
        }
    }

    rta->Shutdown();
    delete rta;

    return 0;
}
