cmake_minimum_required(VERSION 3.20)
project(GameServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -march=native)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/ecs
    ${CMAKE_SOURCE_DIR}/net
    ${CMAKE_SOURCE_DIR}/physics
    ${CMAKE_SOURCE_DIR}/matchmaker
    ${CMAKE_SOURCE_DIR}/anti-cheat-lite
)

# Source files
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "ecs/*.cpp"
    "net/*.cpp"
    "physics/*.cpp"
    "matchmaker/*.cpp"
    "anti-cheat-lite/*.cpp"
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Platform-specific libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32)
endif()

