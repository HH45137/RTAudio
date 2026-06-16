#pragma once
#include <vector>
#include <array>
#include <atomic>
#include <iostream>
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

        virtual size_t GetDurationInMilliseconds() = 0;
    };

    class RTAudio : public IAudioBackend {
    public:
        ~RTAudio() override {
            Shutdown();
        }

        bool Initialize() override {
            is_playing_finished = false;
            sample_data = nullptr;
            this->channels = 0;
            this->sample_rate = 0;
            total_frames = 0;
            sample_frame_cursor = 0;
            device_config = {};
            device = {};

            IPLContextSettings contextSettings{};
            contextSettings.version = STEAMAUDIO_VERSION;
            if (iplContextCreate(&contextSettings, &context) != IPL_STATUS_SUCCESS) {
                std::cout << "Failed to initialize SteamAudio context!\n";
                return false;
            }

            is_ready = true;
            return true;
        }

        void Shutdown() override {
            if (!is_ready) { return; }

            ma_device_uninit(&this->device);
            if (effect) {
                iplBinauralEffectRelease(&effect);
            }
            if (hrtf) {
                iplHRTFRelease(&hrtf);
            }
            if (context) {
                iplContextRelease(&context);
            }

            if (sample_data != nullptr) {
                drwav_free(sample_data, nullptr);
                sample_data = nullptr;
            }

            // 释放预分配的中间缓冲区
            if (out_buffer.data) {
                iplAudioBufferFree(context, &out_buffer);
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
            sample_frame_cursor = 0;
            is_playing_finished = false;
        }

        bool LoadDataFromFile(std::string &_path) override {
            // 读取 WAV 文件
            sample_data = drwav_open_file_and_read_pcm_frames_f32(_path.c_str(), &this->channels, &this->sample_rate, &total_frames, nullptr);
            if (sample_data == nullptr) {
                std::cout << "Error opening and reading WAV file.\n";
                return false;
            }

            // 强制配置：Steam Audio 空间化最适合单声道输入。
            // 如果文件是多声道，以下逻辑假设您将其当做单声道处理，或者输出强制为立体声(2声道)。
            constexpr uint32_t STEAM_AUDIO_FRAME_SIZE = 512; // 推荐使用 512 或 1024

            device_config = ma_device_config_init(ma_device_type_playback);
            device_config.playback.format = ma_format_f32;
            device_config.playback.channels = 2; // 空间化后固定输出立体声（双声道）
            device_config.sampleRate = this->sample_rate;
            // 强迫 miniaudio 的周期帧长等于 Steam Audio 的块大小
            device_config.periodSizeInFrames = STEAM_AUDIO_FRAME_SIZE;
            device_config.dataCallback = data_callback;
            device_config.pUserData = this;

            if (ma_device_init(nullptr, &this->device_config, &this->device) != MA_SUCCESS) {
                std::cout << "Failed to open playback device.\n";
                return false;
            }

            // 初始化 Steam Audio 设置
            audio_settings.samplingRate = static_cast<int32_t>(sample_rate);
            audio_settings.frameSize = STEAM_AUDIO_FRAME_SIZE;

            IPLHRTFSettings hrtfSettings{};
            hrtfSettings.type = IPL_HRTFTYPE_DEFAULT;
            hrtfSettings.volume = 1.0f;

            if (iplHRTFCreate(context, &audio_settings, &hrtfSettings, &hrtf) != IPL_STATUS_SUCCESS) {
                std::cout << "Failed to create HRTF!\n";
                return false;
            }

            IPLBinauralEffectSettings effectSettings{};
            effectSettings.hrtf = hrtf;
            if (iplBinauralEffectCreate(context, &audio_settings, &effectSettings, &effect) != IPL_STATUS_SUCCESS) {
                std::cout << "Failed to create effect!\n";
                return false;
            }

            // 为避免实时线程分配内存，在初始化时提前分配输出 Buffer (立体声 2 通道)
            iplAudioBufferAllocate(context, 2, audio_settings.frameSize, &out_buffer);
            // 预分配用于提取单声道的输入缓冲区
            mono_input_buffer.resize(STEAM_AUDIO_FRAME_SIZE);

            return true;
        }

        size_t GetDurationInMilliseconds() override {
            if (sample_rate == 0) {
                return 0;
            }
            return (total_frames / sample_rate) * 1000;
        }

    private:
        bool is_ready = false;
        std::atomic<bool> is_playing_finished{false};

        float *sample_data = nullptr;
        drwav_uint64 total_frames = 0;
        size_t sample_frame_cursor = 0;
        uint32_t channels = 0;
        uint32_t sample_rate = 0;

        ma_device_config device_config = {};
        ma_device device = {};

        // Steam Audio 实例变量
        IPLContext context = nullptr;
        IPLAudioSettings audio_settings{};
        IPLHRTF hrtf = nullptr;
        IPLBinauralEffect effect = nullptr;

        // 预分配的实时缓冲区，防止 Callback 内发生 GC/Malloc 产生杂音
        IPLAudioBuffer out_buffer{};
        std::vector<float> mono_input_buffer;

        // 核心音频处理逻辑
        void ProcessSpatialAudio(float *output_stereo_buffer, size_t frame_count) {
            if (!effect || frame_count != audio_settings.frameSize) {
                return;
            }

            // 1. 从源音频数据提取单声道数据放入 mono_input_buffer
            for (size_t i = 0; i < frame_count; ++i) {
                size_t current_frame = sample_frame_cursor + i;
                if (current_frame < total_frames) {
                    // 如果原文件是立体声，取左声道作为单声道输入，若是单声道则直接取值
                    mono_input_buffer[i] = sample_data[current_frame * channels];
                } else {
                    mono_input_buffer[i] = 0.0f;
                }
            }

            // 2. 绑定 Steam Audio 输入结构体
            float *in_data_channels[] = {mono_input_buffer.data()};
            IPLAudioBuffer in_buffer{};
            in_buffer.numChannels = 1; // 空间化输入固定为 1（单声道）
            in_buffer.numSamples = static_cast<int32_t>(frame_count);
            in_buffer.data = in_data_channels;

            // 3. 配置空间参数
            IPLBinauralEffectParams effectParams{};
            effectParams.direction = IPLVector3{1.0f, 0.0f, 1.0f};
            effectParams.interpolation = IPL_HRTFINTERPOLATION_BILINEAR;
            effectParams.spatialBlend = 1.0f;
            effectParams.hrtf = hrtf;

            // 4. 应用 HRTF 空间化效果（输出到预分配的立体声 out_buffer 中）
            iplBinauralEffectApply(effect, &effectParams, &in_buffer, &out_buffer);

            // 5. 将 Steam Audio 的平铺（Planar）数据交织（Interleave）拷贝回 miniaudio 的立体声输出中
            // out_buffer.data[0] 是左声道，out_buffer.data[1] 是右声道
            for (size_t i = 0; i < frame_count; ++i) {
                output_stereo_buffer[i * 2 + 0] = out_buffer.data[0][i];
                output_stereo_buffer[i * 2 + 1] = out_buffer.data[1][i];
            }

            // 更新游标
            sample_frame_cursor += frame_count;
            if (sample_frame_cursor >= total_frames) {
                is_playing_finished = true;
            }
        }

        static void data_callback(ma_device *_device, void *_output, const void *_input, ma_uint32 _frame_count) {
            auto *const this_ptr = static_cast<RTAudio *>(_device->pUserData);
            if (!this_ptr || this_ptr->is_playing_finished) {
                memset(_output, 0, _frame_count * _device->playback.channels * sizeof(float));
                return;
            }

            // 此时由于设置了 periodSizeInFrames，_frame_count 必定等于预设的 512
            this_ptr->ProcessSpatialAudio(static_cast<float *>(_output), _frame_count);
        }
    };

}
