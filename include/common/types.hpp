#pragma once

#include <cstdint>
#include <chrono>
#include <limits>

namespace game {

// Core types
using EntityID = uint32_t;
using ComponentTypeID = uint32_t;
using SystemTypeID = uint32_t;
using PlayerID = uint32_t;
using RoomID = uint32_t;
using Tick = uint64_t;

// Network types
using PacketID = uint16_t;
using SequenceNumber = uint32_t;

// Constants
constexpr EntityID INVALID_ENTITY = std::numeric_limits<EntityID>::max();
constexpr PlayerID INVALID_PLAYER = std::numeric_limits<PlayerID>::max();
constexpr RoomID INVALID_ROOM = std::numeric_limits<RoomID>::max();

// Time types
using TimePoint = std::chrono::steady_clock::time_point;
using Duration = std::chrono::steady_clock::duration;
using Milliseconds = std::chrono::milliseconds;
using Microseconds = std::chrono::microseconds;

// Tick rates
constexpr int TICK_RATE_60 = 60;
constexpr int TICK_RATE_120 = 120;
constexpr int DEFAULT_TICK_RATE = TICK_RATE_60;

// Network constants
constexpr int MAX_PLAYERS_PER_ROOM = 128;
constexpr int MAX_PACKET_SIZE = 1400; // MTU-safe
constexpr int MAX_SNAPSHOT_SIZE = 1024;

// Physics constants
constexpr float FIXED_TIMESTEP = 1.0f / DEFAULT_TICK_RATE;
constexpr float MAX_DELTA_TIME = 0.1f; // Prevent spiral of death

} // namespace game

