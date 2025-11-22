cmake_minimum_required(VERSION 3.20)

# vcpkg'yi tamamen devre disi birak
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    unset(ENV{VCPKG_ROOT})
endif()

project(GameServer VERSION 1.0.0 LANGUAGES CXX)

# Generate compile_commands.json for IDE IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -O3 -march=native)
    add_compile_options(-D__USE_MINGW_ANSI_STDIO=1)
    add_compile_options(-fext-numeric-literals)
    add_compile_options(-fpermissive)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/ecs
    ${CMAKE_SOURCE_DIR}/net
    ${CMAKE_SOURCE_DIR}/physics
    ${CMAKE_SOURCE_DIR}/matchmaker
    ${CMAKE_SOURCE_DIR}/anti-cheat-lite
    ${CMAKE_SOURCE_DIR}/components
    ${CMAKE_SOURCE_DIR}/systems
    ${CMAKE_SOURCE_DIR}/src
)

# GameServer does not require SFML or LDtkLoader
# (These were only needed for the old LDtk test code)

# Source files - GameServer için gerekli dosyalar
set(SOURCES
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_SOURCE_DIR}/src/Server.cpp"
    "${CMAKE_SOURCE_DIR}/ecs/World.cpp"
    "${CMAKE_SOURCE_DIR}/ecs/Component.cpp"
    "${CMAKE_SOURCE_DIR}/net/Socket.cpp"
    "${CMAKE_SOURCE_DIR}/net/Snapshot.cpp"
    "${CMAKE_SOURCE_DIR}/physics/Physics.cpp"
    "${CMAKE_SOURCE_DIR}/matchmaker/Matchmaker.cpp"
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Platform-specific libraries
target_link_libraries(${PROJECT_NAME} PRIVATE)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()

message(STATUS "GameServer executable configured")

# Assets klasörünü kopyala (eğer varsa)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/assets")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory 
        ${CMAKE_CURRENT_LIST_DIR}/assets/ 
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/
    )
endif()

# No external DLLs required for GameServer

