#pragma once

#include "../include/common/types.hpp"
#include "../net/Socket.hpp"
#include "../net/Packet.hpp"
#include "../physics/Physics.hpp"
#include <unordered_map>
#include <vector>
#include <deque>
#include <chrono>

namespace game::anticheat {

struct PlayerStats {
    PlayerID playerID;
    int packetsPerSecond;
    int suspiciousActions;
    float movementSpeed;
    physics::Vec3 lastPosition;
    TimePoint lastUpdate;
    
    PlayerStats(PlayerID id) 
        : playerID(id), packetsPerSecond(0), suspiciousActions(0),
          movementSpeed(0.0f), lastPosition(), 
          lastUpdate(std::chrono::steady_clock::now()) {}
};

class AntiCheat {
private:
    std::unordered_map<PlayerID, PlayerStats> playerStats;
    std::unordered_map<PlayerID, std::deque<TimePoint>> packetTimestamps;
    
    const int MAX_PACKETS_PER_SECOND = 60;
    const float MAX_MOVEMENT_SPEED = 1000.0f; // units per second
    const int SUSPICIOUS_THRESHOLD = 10;
    
    void updatePacketRate(PlayerID playerID);
    bool checkMovementSpeed(PlayerID playerID, const physics::Vec3& newPosition, float deltaTime);

public:
    AntiCheat() = default;
    
    bool validateInput(PlayerID playerID, const net::Packet& packet);
    bool validateMovement(PlayerID playerID, const physics::Vec3& position, float deltaTime);
    void recordPacket(PlayerID playerID);
    bool checkPacketRate(PlayerID playerID);  // Public for server access
    
    int getSuspiciousCount(PlayerID playerID) const;
    bool shouldKick(PlayerID playerID) const;
    
    void resetPlayer(PlayerID playerID);
};

} // namespace game::anticheat

