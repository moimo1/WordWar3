cmake_minimum_required(VERSION 3.14)

# 1. FIX: Tell CMake 4.1 to treat all dependencies as at least version 3.5
# This stops the "Compatibility with CMake < 3.5 has been removed" error.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(WordWar3 LANGUAGES CXX)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure modern CMake policies for dependencies
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

# --------------------------
# Fetch dependencies
# --------------------------
include(FetchContent)

# Common settings for fetched libraries
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)

# 1. Fetch Raylib
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
)
FetchContent_MakeAvailable(raylib)

# 2. Fetch Nlohmann JSON (Industry standard C++ JSON library)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# --------------------------
# Project executable
# --------------------------
add_executable(WordWar3
        main.cpp

        combat/Effect.cpp
        combat/Effect.h

        manager/GameManager.cpp
        manager/GameManager.h
        manager/UIManager.cpp
        manager/UIManager.h

        player/Player.cpp
        player/Player.h
        player/Sentence.cpp
        player/Sentence.h

        word/Word.cpp
        word/Word.h
        word/WordDictionary.cpp
        word/WordDictionary.h
)

# Link Raylib to your executable
target_link_libraries(WordWar3 PRIVATE raylib nlohmann_json::nlohmann_json)

# --------------------------
# Windows linker settings
# --------------------------
if(WIN32)
    # Required for Raylib on Windows MinGW
    target_link_libraries(WordWar3 PRIVATE winmm gdi32 shell32)
endif()