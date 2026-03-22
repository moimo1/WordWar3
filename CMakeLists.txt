cmake_minimum_required(VERSION 3.14)

# 1. Hotfix for modern CMake 4+ rejecting Raylib's internal legacy CMakeLists
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

project(WordWar3)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

# Common settings for fetched libraries
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)

# 1. Fetch Raylib
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
)
FetchContent_MakeAvailable(raylib)

# 2. Fetch Nlohmann JSON
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# 3. Fetch Asio Standalone (Industry Standard Network Library)
# We fetch it directly from the GitHub releases to guarantee no compiling errors on Windows
FetchContent_Declare(
    asio
    URL https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-30-2.tar.gz
)
FetchContent_MakeAvailable(asio)

# Force ASIO to be standalone (meaning it doesn't require the massive Boost libraries)
add_compile_definitions(ASIO_STANDALONE _WIN32_WINNT=0x0A00)

# Core logic files used strictly by both the Client AND Server
set(CORE_SOURCES
    combat/Effect.cpp
    combat/Effect.h
    player/Player.cpp
    player/Player.h
    player/Sentence.cpp
    player/Sentence.h
    word/Word.cpp
    word/Word.h
    word/WordDictionary.cpp
    word/WordDictionary.h
)

# --------------------------
# The Headless Server Target
# --------------------------
add_executable(WordWar3_Server 
    server_main.cpp
    network/Server.cpp
    network/Server.h
    network/Packet.h
    ${CORE_SOURCES}
)
# We explicitly link Ws2_32 on Windows since basic sockets require it internally
target_link_libraries(WordWar3_Server PRIVATE nlohmann_json::nlohmann_json wsock32 ws2_32)
target_include_directories(WordWar3_Server PRIVATE ${asio_SOURCE_DIR}/asio/include)

# --------------------------
# The Raylib Client Target
# --------------------------
add_executable(WordWar3_Client 
    client_main.cpp
    network/Client.cpp
    network/Client.h
    manager/UIManager.cpp
    manager/UIManager.h
    manager/GameManager.cpp
    manager/GameManager.h
    ${CORE_SOURCES}
)
target_link_libraries(WordWar3_Client PRIVATE raylib nlohmann_json::nlohmann_json wsock32 ws2_32)
target_include_directories(WordWar3_Client PRIVATE ${asio_SOURCE_DIR}/asio/include ${raylib_SOURCE_DIR}/src)