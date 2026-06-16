#pragma once
#include <vector>
#include <array>
#include <miniaudio.h>
#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>


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
            this->sample_data = nullptr;
            this->channels = 0;
            this->sample_rate = 0;
            this->total_sample_count = 0;

            is_ready = true;
            return true;
        }

        void Shutdown() override {
            drwav_free(this->sample_data, nullptr);

            this->is_ready = false;
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
            drwav_uint64 totalPCMFrameCount;
            this->sample_data = drwav_open_file_and_read_pcm_frames_f32(_path.c_str(), &this->channels, &this->sample_rate, &this->total_sample_count, nullptr);
            if (this->sample_data == nullptr) {
                throw std::runtime_error("Error opening and reading WAV file.");
            }

            return true;
        }

        bool LoadDataFromMem(std::vector<float> &_data) override {
            return true;
        }

    private:
        bool is_ready = false;
        uint32_t channels = 0;
        uint32_t sample_rate = 0;
        unsigned long long total_sample_count = 0;
        float *sample_data = nullptr;
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
