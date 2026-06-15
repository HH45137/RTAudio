#pragma once


namespace RTAudio {
    class IAudioBackend {
    public:
        virtual ~IAudioBackend() = default;

        virtual bool Initialize() = 0;

        virtual void Shutdown() = 0;

        virtual bool IsReady() = 0;

        virtual void Play() = 0;

        virtual void Pause() = 0;

        virtual void Reset() = 0;
    };

    class AudioBackendOpenAL : public IAudioBackend {
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
