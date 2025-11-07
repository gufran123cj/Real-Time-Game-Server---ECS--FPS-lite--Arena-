#include "Matchmaker.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace game::matchmaker {

void Matchmaker::addPlayer(PlayerID playerID, float rating, int teamSize) {
    if (queuedPlayers.find(playerID) != queuedPlayers.end()) {
        return; // Already in queue
    }
    
    PlayerQueueEntry entry(playerID, rating, teamSize);
    queuedPlayers.emplace(playerID, entry);
    soloQueue.push(entry);
}

void Matchmaker::removePlayer(PlayerID playerID) {
    queuedPlayers.erase(playerID);
    // Note: Queue removal is lazy (checked during processQueue)
}

bool Matchmaker::canMatch(const PlayerQueueEntry& a, const PlayerQueueEntry& b) const {
    float ratingDiff = std::abs(a.rating - b.rating);
    return ratingDiff <= ratingTolerance;
}

std::vector<PlayerID> Matchmaker::findMatch(int teamSize) {
    std::vector<PlayerID> match;
    
    // Simple implementation: find players with similar ratings
    std::vector<PlayerQueueEntry> candidates;
    for (const auto& [id, entry] : queuedPlayers) {
        if (entry.preferredTeamSize == teamSize || teamSize == 1) {
            candidates.push_back(entry);
        }
    }
    
    if (candidates.size() < static_cast<size_t>(teamSize)) {
        return match;
    }
    
    // Sort by rating
    std::sort(candidates.begin(), candidates.end(),
        [](const PlayerQueueEntry& a, const PlayerQueueEntry& b) {
            return a.rating < b.rating;
        });
    
    // Try to form a match
    for (size_t i = 0; i < candidates.size() && match.size() < static_cast<size_t>(teamSize); ++i) {
        bool canAdd = true;
        for (PlayerID existingID : match) {
            auto it = queuedPlayers.find(existingID);
            if (it != queuedPlayers.end() && 
                !canMatch(it->second, candidates[i])) {
                canAdd = false;
                break;
            }
        }
        
        if (canAdd) {
            match.push_back(candidates[i].playerID);
        }
    }
    
    return match;
}

std::vector<Match> Matchmaker::processQueue() {
    std::vector<Match> matches;
    
    // Remove players no longer in queue
    while (!soloQueue.empty()) {
        const auto& entry = soloQueue.front();
        if (queuedPlayers.find(entry.playerID) == queuedPlayers.end()) {
            soloQueue.pop();
            continue;
        }
        
        // Check if player has been waiting too long
        auto waitTime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - entry.joinTime).count();
        
        if (waitTime > maxMatchmakingTime) {
            // Increase tolerance for this player
            // TODO: Implement dynamic tolerance adjustment
        }
        
        // Try to find a match
        std::vector<PlayerID> matchPlayers = findMatch(entry.preferredTeamSize);
        if (matchPlayers.size() >= static_cast<size_t>(entry.preferredTeamSize)) {
            // Create match (roomID will be assigned by server)
            float avgRating = 0.0f;
            for (PlayerID id : matchPlayers) {
                auto it = queuedPlayers.find(id);
                if (it != queuedPlayers.end()) {
                    avgRating += it->second.rating;
                    queuedPlayers.erase(id);
                }
            }
            avgRating /= matchPlayers.size();
            
            // Remove matched players from queue
            soloQueue.pop();
            matches.emplace_back(INVALID_ROOM, matchPlayers, avgRating);
        } else {
            break; // No match found yet
        }
    }
    
    return matches;
}

} // namespace game::matchmaker

