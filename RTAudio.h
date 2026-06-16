#pragma once
#include <vector>
#include <array>
#include <atomic>
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>
#include <phonon.h>


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

        virtual bool LoadDataFromMem(float *_data, size_t _count, size_t _sample_rate) = 0;

        virtual size_t GetDurationInMilliseconds() = 0;
    };

    class RTAudio : public IAudioBackend {
    public:
        bool Initialize() override {
            {
                is_playing_finished = false;
                sample_data = nullptr;
                this->channels = 0;
                this->sample_rate = 0;
                total_sample_data_count = 0;
                sample_data_cursor = 0;
                device_config = {};
                device = {};
            }

            {
                IPLContextSettings contextSettings{};
                contextSettings.version = STEAMAUDIO_VERSION;

                if (iplContextCreate(&contextSettings, &context) != IPL_STATUS_SUCCESS) {
                    std::cout << "Failed to initial SteamAudio context!\n";
                    return false;
                }
            }

            is_ready = true;
            return true;
        }

        void Shutdown() override {
            ma_device_uninit(&this->device);

            if (sample_data != nullptr) {
                drwav_free(sample_data, nullptr);
                sample_data = nullptr;
            }

            this->is_ready = false;
        }

        bool IsReady() override {
            return is_ready;
        }

        void Play() override {
            if (!this->is_ready || sample_data == nullptr) { return; }

            if (is_playing_finished) {
                this->Reset();
            }

            if (ma_device_start(&this->device) != MA_SUCCESS) {
                std::cout << "Failed to start playback device.\n";
                return;
            }
        }

        void Pause() override {
            if (!this->is_ready || sample_data == nullptr) { return; }

            ma_device_stop(&this->device);
        }

        void Reset() override {
            if (!this->is_ready || sample_data == nullptr) { return; }

            ma_device_stop(&this->device);
            sample_data_cursor = 0;
            is_playing_finished = false;
        }

        bool LoadDataFromFile(std::string &_path) override {
            sample_data = drwav_open_file_and_read_pcm_frames_f32(_path.c_str(), &this->channels, &this->sample_rate, &total_sample_data_count, nullptr);
            if (sample_data == nullptr) {
                std::cout << "Error opening and reading WAV file.\n";
                return false;
            }

            device_config = ma_device_config_init(ma_device_type_playback);
            device_config.playback.format = ma_format_f32;
            device_config.playback.channels = this->channels;
            device_config.sampleRate = this->sample_rate;
            device_config.dataCallback = data_callback;

            if (ma_device_init(nullptr, &this->device_config, &this->device) != MA_SUCCESS) {
                std::cout << "Failed to open playback device.\n";
                return false;
            }

            return true;
        }

        bool LoadDataFromMem(float *_data, size_t _count, size_t _sample_rate) override {
            return true;
        }

        size_t GetDurationInMilliseconds() override {
            if (sample_rate == 0) {
                return 0;
            }
            return (total_sample_data_count / sample_rate) * 1000;
        }

        bool SetAudio(std::vector<float> _audio_buffer, size_t _sample_rate, size_t _frame_size) {
            // HRTF
            IPLHRTFSettings hrtfSettings{};
            hrtfSettings.type = IPL_HRTFTYPE_DEFAULT;
            hrtfSettings.volume = 1.0f;

            // Audio
            IPLAudioSettings audioSettings{};
            audioSettings.samplingRate = static_cast<int32_t>(_sample_rate);
            audioSettings.frameSize = static_cast<int32_t>(_frame_size);

            // Create HRTF
            if (iplHRTFCreate(context, &audioSettings, &hrtfSettings, &hrtf) != IPL_STATUS_SUCCESS) {
                std::cout << "Failed to create HRTF!\n";
                return false;
            }

            // Effect
            IPLBinauralEffectSettings effectSettings{};
            effectSettings.hrtf = hrtf;
            if (iplBinauralEffectCreate(context, &audioSettings, &effectSettings, &effect) != IPL_STATUS_SUCCESS) {
                std::cout << "Failed to create effect!\n";
                return false;
            }

            // Audio input buffer
            float *in_data[] = {_audio_buffer.data()};
            IPLAudioBuffer in_buffer{};
            in_buffer.numChannels = 1;
            in_buffer.numSamples = audioSettings.frameSize;
            in_buffer.data = in_data;

            // Audio output buffer
            IPLAudioBuffer out_buffer{};
            if (iplAudioBufferAllocate(context, 2, audioSettings.frameSize, &out_buffer) != IPL_STATUS_SUCCESS) {
                std::cout << "Failed to allocate audio output buffer!\n";
                return false;
            }
            std::vector<float> out_data(2 * audioSettings.frameSize);

            return true;
        }

    private:
        static std::atomic<bool> is_playing_finished;
        static float *sample_data;
        static size_t total_sample_data_count;
        static size_t sample_data_cursor;

        bool is_ready = false;
        uint32_t channels = 0;
        uint32_t sample_rate = 0;
        ma_device_config device_config = {};
        ma_device device = {};
        IPLContext context = nullptr;
        IPLHRTF hrtf = nullptr;
        IPLBinauralEffect effect = nullptr;

        static void Effect(void *_buffer, size_t _size, uint32_t _channels) {
            auto temp_buffer = std::vector<float>(_size);
            memcpy(temp_buffer.data(), _buffer, _size);
            {
                // SetAudio(temp_buffer, sample_data_cursor, total_sample_data_count);
            }
            memcpy(_buffer, temp_buffer.data(), _size);
        }

        static void data_callback(ma_device *_device, void *_output, const void *_input, ma_uint32 _frame_count) {
            // 1. 明确我们要复制多少个 float 采样
            // 总采样数 = 帧数 * 声道数
            size_t samples_to_read = _frame_count * _device->playback.channels;

            // dr_wav 返回的 total_sample_data_count 实际上是总帧数（Total Frames）
            // 所以总采样数 = 总帧数 * 声道数
            size_t total_samples = total_sample_data_count * _device->playback.channels;

            // 2. 计算内存里还剩下多少个采样没播放
            size_t samples_remaining = total_samples - sample_data_cursor;
            size_t samples_to_copy = (samples_to_read < samples_remaining) ? samples_to_read : samples_remaining;

            // 3. 复制数据。因为 sample_data 是 float*，sample_data_cursor 是采样数，所以这里的加法是完全正确的。
            // memcpy 的大小是：采样数 * sizeof(float)
            if (samples_to_copy > 0) {
                const size_t temp_size = samples_to_copy * sizeof(float);
                const void *temp_cursor = sample_data + sample_data_cursor;
                memcpy(_output, temp_cursor, temp_size);

                Effect(_output, temp_size, _device->playback.channels);

                sample_data_cursor += samples_to_copy;
            }

            // 4. 如果数据不够，用静音（0）填充剩余区域
            if (samples_to_copy < samples_to_read) {
                size_t samples_to_zero = samples_to_read - samples_to_copy;
                float *remaining_output = static_cast<float *>(_output) + samples_to_copy;
                memset(remaining_output, 0, samples_to_zero * sizeof(float));

                is_playing_finished = true;
            }
        }
    };

    std::atomic<bool> RTAudio::is_playing_finished;
    float *RTAudio::sample_data = nullptr;
    size_t RTAudio::total_sample_data_count{};
    size_t RTAudio::sample_data_cursor{};
}
