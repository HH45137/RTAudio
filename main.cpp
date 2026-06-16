#include <cassert>
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include "RTAudio.h"

// 定义常数 PI
constexpr float PI = 3.1415926535f;

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <wav_path_1> <wav_path_2>\n";
        return -1;
    }

    RTA::RTAudio sound1;
    RTA::RTAudio sound2;

    sound1.Initialize();
    sound2.Initialize();

    if (!sound1.LoadDataFromFile(argv[1]) || !sound2.LoadDataFromFile(argv[2])) {
        std::cout << "Failed to load audio files.\n";
        return -1;
    }

    sound1.Play();
    sound2.Play();

    std::cout << "Streaming spatial audio... Press Ctrl+C to exit.\n";

    // 旋转控制参数
    float angle = 0.0f; // 初始角度
    float rotation_speed = 3.0f; // 旋转速度（弧度/秒），大约 6 秒转一整圈

    // 获取初始时间点
    auto last_time = std::chrono::high_resolution_clock::now();

    // 替换原有的 std::cin.get()，改为动态更新的旋转主循环
    while (true) {
        // 1. 计算两帧之间的时间差（Delta Time）
        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = current_time - last_time;
        last_time = current_time;

        float dt = elapsed.count();

        // 2. 更新角度
        angle += rotation_speed * dt;
        if (angle > 2.0f * PI) {
            angle -= 2.0f * PI;
        }

        // 3. 计算 3D 空间直角坐标
        // 在 XZ 平面（水平面）上绕中心旋转：X = cos(angle), Z = sin(angle)
        float x1 = std::cos(angle);
        float z1 = std::sin(angle);

        // 让声源 2 和声源 1 保持相反方向（相差 180 度，即加负号），形成双星环绕效果
        float x2 = -x1;
        float z2 = -z1;

        // 4. 将计算好的新方向安全地赋值给音源
        // Steam Audio 的坐标系通常为：X是右，Y是上，Z是前/后（具体取决于配置，这里在水平面上转动）
        sound1.source_direction = IPLVector3{x1, 0.0f, z1};
        sound2.source_direction = IPLVector3{x2, 0.0f, z2};

        // 5. 降低 CPU 占用，每秒更新约 60 次 (16 毫秒)
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}
