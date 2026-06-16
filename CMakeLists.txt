cmake_minimum_required(VERSION 4.2)
project(RTAudio)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(RTAudio main.cpp
        RTAudio.h
)

include_directories(${CMAKE_SOURCE_DIR}/thirdpart/steamaudio/include)
include_directories(${CMAKE_SOURCE_DIR}/thirdpart/openal-soft/include)
include_directories(${CMAKE_SOURCE_DIR}/thirdpart/miniaudio)

target_link_libraries(${PROJECT_NAME}
        ${CMAKE_SOURCE_DIR}/thirdpart/openal-soft/libs/Win64/OpenAL32.lib
        ${CMAKE_SOURCE_DIR}/thirdpart/steamaudio/lib/windows-x64/phonon.lib
)

set(NEED_DLLS
        "${CMAKE_SOURCE_DIR}/thirdpart/steamaudio/lib/windows-x64/phonon.dll"
        "${CMAKE_SOURCE_DIR}/thirdpart/steamaudio/lib/windows-x64/GPUUtilities.dll"
        "${CMAKE_SOURCE_DIR}/thirdpart/steamaudio/lib/windows-x64/TrueAudioNext.dll"
        "${CMAKE_SOURCE_DIR}/thirdpart/openal-soft/router/Win64/OpenAL32.dll"
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${NEED_DLLS}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
