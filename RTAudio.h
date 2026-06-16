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

    class RTAudio {
    public:
        bool Initialize() {
            IPLContextSettings contextSettings{};
            contextSettings.version = STEAMAUDIO_VERSION;

            iplContextCreate(&contextSettings, &context);

            is_ready = true;
            return true;
        }

        void Shutdown() {
        }

        bool IsReady() {
            return is_ready;
        }

    private:
        bool is_ready = false;

        IPLContext context = nullptr;
    };

    namespace AudioBackend {
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

        class AudioBackendMiniAudio : public IAudioBackend {
        public:
            bool Initialize() override {
                is_playing_finished = false;
                sample_data = nullptr;
                this->channels = 0;
                this->sample_rate = 0;
                total_sample_data_count = 0;
                sample_data_cursor = 0;
                device_config = {};
                device = {};

                this->rta_audio = new RTAudio();

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

        private:
            bool is_ready = false;
            static std::atomic<bool> is_playing_finished;

            uint32_t channels = 0;
            uint32_t sample_rate = 0;
            static float *sample_data;
            static size_t total_sample_data_count;
            static size_t sample_data_cursor;

            ma_device_config device_config = {};
            ma_device device = {};

            RTAudio *rta_audio = nullptr;

            static void Effect(void *_buffer, size_t _size, uint32_t _channels) {
                auto *temp_buffer = static_cast<float *>(malloc(_size));
                memcpy(temp_buffer, _buffer, _size);
                {
                }
                memcpy(_buffer, temp_buffer, _size);
                free(temp_buffer);
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

        std::atomic<bool> AudioBackendMiniAudio::is_playing_finished;
        float *AudioBackendMiniAudio::sample_data = nullptr;
        size_t AudioBackendMiniAudio::total_sample_data_count{};
        size_t AudioBackendMiniAudio::sample_data_cursor{};
    }

}
