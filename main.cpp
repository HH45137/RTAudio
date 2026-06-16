#include <cassert>
#include <iostream>

#include "RTAudio.h"


int main(int argc, char **argv) {
    auto *audio_backend = new RTA::AudioBackendMiniAudio();

    if (!audio_backend->IsReady()) {
        if (!audio_backend->Initialize()) {
            throw std::runtime_error("Failed to initialize audio backend");
        }

        std::cout << "Successd to initialize audio backend\n";
    }

    assert(argc == 2);
    std::string audio_file = std::string(argv[1]);
    std::cout << "audio_file = " << audio_file << std::endl;
    assert(audio_backend->LoadDataFromFile(audio_file));

    std::cout << "\n--- 音频加载成功 ---" << std::endl;
    std::cout << "总长度: " << audio_backend->GetDurationInMilliseconds() << " 毫秒" << std::endl;
    std::cout << "控制命令: [1] 播放/恢复  [2] 暂停  [3] 重置回到开头  [4] 退出程序" << std::endl;
    std::cout << "-----------------------\n" << std::endl;

    bool running = true;
    while (running) {
        std::cout << "请输入指令: ";
        int cmd = 0;
        std::cin >> cmd;

        switch (cmd) {
            case 1:
                std::cout << ">> 正在播放/恢复..." << std::endl;
                audio_backend->Play();
                break;
            case 2:
                std::cout << ">> 已暂停。输入 1 可以继续播放。" << std::endl;
                audio_backend->Pause();
                break;
            case 3:
                std::cout << ">> 已重置到音频开头。" << std::endl;
                audio_backend->Reset();
                break;
            case 4:
                std::cout << ">> 正在退出程序..." << std::endl;
                running = false;
                break;
            default:
                std::cout << "未知指令，请重新输入！" << std::endl;
                break;
        }
    }

    audio_backend->Shutdown();
    delete audio_backend;

    return 0;
}
