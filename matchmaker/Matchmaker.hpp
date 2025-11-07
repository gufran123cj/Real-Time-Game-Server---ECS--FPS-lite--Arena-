#pragma once

#include "../include/common/types.hpp"
#include <vector>
#include <unordered_map>
#include <queue>
#include <memory>
#include <cstddef>

namespace game::matchmaker {

struct PlayerQueueEntry {
    PlayerID playerID;
    float rating;
    TimePoint joinTime;
    int preferredTeamSize;
    
    PlayerQueueEntry(PlayerID id, float rating, int teamSize = 1)
        : playerID(id), rating(rating), joinTime(std::chrono::steady_clock::now()), preferredTeamSize(teamSize) {}
};

struct Match {
    RoomID roomID;
    std::vector<PlayerID> players;
    float averageRating;
    TimePoint createdTime;
    
    Match(RoomID room, const std::vector<PlayerID>& players, float avgRating)
        : roomID(room), players(players), averageRating(avgRating),
          createdTime(std::chrono::steady_clock::now()) {}
};

class Matchmaker {
private:
    std::queue<PlayerQueueEntry> soloQueue;
    std::unordered_map<PlayerID, PlayerQueueEntry> queuedPlayers;
    
    float ratingTolerance = 100.0f; // Initial tolerance
    int maxMatchmakingTime = 30; // seconds
    
    bool canMatch(const PlayerQueueEntry& a, const PlayerQueueEntry& b) const;
    std::vector<PlayerID> findMatch(int teamSize);

public:
    Matchmaker() = default;
    
    void addPlayer(PlayerID playerID, float rating, int teamSize = 1);
    void removePlayer(PlayerID playerID);
    std::vector<Match> processQueue();
    
    void setRatingTolerance(float tolerance) { ratingTolerance = tolerance; }
    float getRatingTolerance() const { return ratingTolerance; }
    
    size_t getQueueSize() const { return queuedPlayers.size(); }
};

} // namespace game::matchmaker

