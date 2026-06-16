#pragma once
#include <vector>
#include <array>
#include <miniaudio.h>


namespace RTA {
    class IAudioBackend {
    public:
        virtual ~IAudioBackend() = default;

        virtual bool Initialize() = 0;

        virtual void Shutdown() = 0;

        virtual bool IsReady() = 0;

        virtual void Play() = 0;

        virtual void Pause() = 0;

        virtual void Reset() = 0;

        virtual bool LoadDataFromFile(std::string &_path) = 0;

        virtual bool LoadDataFromMem(std::vector<float> &_data) = 0;
    };

    class AudioBackendMiniAudio : public IAudioBackend {
    public:
        bool Initialize() override {
            is_ready = true;
            return true;
        }

        void Shutdown() override {
        }

        bool IsReady() override {
            return is_ready;
        }

        void Play() override {
        }

        void Pause() override {
        }

        void Reset() override {
        }

        bool LoadDataFromFile(std::string &_path) override {
            return true;
        }

        bool LoadDataFromMem(std::vector<float> &_data) override {
            return true;
        }

    private:
        bool is_ready = false;
    };

    class RTAudio {
    public:
        bool Initialize(IAudioBackend *_audio_backend) {
            audio_backend = _audio_backend;;

            is_ready = true;
            return true;
        }

        void Shutdown() {
            audio_backend->Shutdown();
        }

        bool IsReady() {
            return is_ready;
        }

    private:
        bool is_ready = false;
        IAudioBackend *audio_backend = nullptr;
    };
}
