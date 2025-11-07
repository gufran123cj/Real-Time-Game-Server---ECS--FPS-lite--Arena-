#include "AntiCheat.hpp"
#include "../physics/Physics.hpp"
#include "../net/Packet.hpp"
#include <algorithm>
#include <cmath>

namespace game::anticheat {

void AntiCheat::recordPacket(PlayerID playerID) {
    auto now = std::chrono::steady_clock::now();
    packetTimestamps[playerID].push_back(now);
    
    // Keep only last second of timestamps
    auto oneSecondAgo = now - std::chrono::seconds(1);
    auto& timestamps = packetTimestamps[playerID];
    timestamps.erase(
        std::remove_if(timestamps.begin(), timestamps.end(),
            [oneSecondAgo](const TimePoint& tp) { return tp < oneSecondAgo; }),
        timestamps.end()
    );
    
    updatePacketRate(playerID);
}

void AntiCheat::updatePacketRate(PlayerID playerID) {
    auto it = playerStats.find(playerID);
    if (it == playerStats.end()) {
        playerStats.emplace(playerID, PlayerStats(playerID));
        it = playerStats.find(playerID);
    }
    
    it->second.packetsPerSecond = static_cast<int>(packetTimestamps[playerID].size());
}

bool AntiCheat::checkPacketRate(PlayerID playerID) {
    auto it = playerStats.find(playerID);
    if (it == playerStats.end()) return true;
    
    if (it->second.packetsPerSecond > MAX_PACKETS_PER_SECOND) {
        it->second.suspiciousActions++;
        return false;
    }
    return true;
}

bool AntiCheat::checkMovementSpeed(PlayerID playerID, const physics::Vec3& newPosition, float deltaTime) {
    auto it = playerStats.find(playerID);
    if (it == playerStats.end()) {
        playerStats.emplace(playerID, PlayerStats(playerID));
        it = playerStats.find(playerID);
        it->second.lastPosition = newPosition;
        return true;
    }
    
    if (deltaTime <= 0.0f) return true;
    
    physics::Vec3 delta = newPosition - it->second.lastPosition;
    float distance = delta.length();
    float speed = distance / deltaTime;
    
    it->second.movementSpeed = speed;
    it->second.lastPosition = newPosition;
    it->second.lastUpdate = std::chrono::steady_clock::now();
    
    if (speed > MAX_MOVEMENT_SPEED) {
        it->second.suspiciousActions++;
        return false;
    }
    
    return true;
}

bool AntiCheat::validateInput(PlayerID playerID, const net::Packet& packet) {
    recordPacket(playerID);
    return checkPacketRate(playerID);
}

bool AntiCheat::validateMovement(PlayerID playerID, const physics::Vec3& position, float deltaTime) {
    return checkMovementSpeed(playerID, position, deltaTime);
}

int AntiCheat::getSuspiciousCount(PlayerID playerID) const {
    auto it = playerStats.find(playerID);
    if (it == playerStats.end()) return 0;
    return it->second.suspiciousActions;
}

bool AntiCheat::shouldKick(PlayerID playerID) const {
    return getSuspiciousCount(playerID) >= SUSPICIOUS_THRESHOLD;
}

void AntiCheat::resetPlayer(PlayerID playerID) {
    playerStats.erase(playerID);
    packetTimestamps.erase(playerID);
}

} // namespace game::anticheat

