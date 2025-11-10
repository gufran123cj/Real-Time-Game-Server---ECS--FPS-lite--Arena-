#pragma once

#include "../ecs/World.hpp"
#include "../ecs/Entity.hpp"
#include "../net/Socket.hpp"
#include "../net/Packet.hpp"
#include "../include/common/types.hpp"
#include "../anti-cheat-lite/AntiCheat.hpp"
#include <unordered_map>
#include <memory>
#include <chrono>
#include <vector>
#include <queue>
#include <set>

namespace game {

struct Player {
    PlayerID id;
    net::Address address;
    Tick lastSeenTick;
    bool connected;
    RoomID currentRoom;
    
    Player(PlayerID id, const net::Address& addr) 
        : id(id), address(addr), lastSeenTick(0), connected(true), currentRoom(INVALID_ROOM) {}
};

struct Room {
    RoomID id;
    std::vector<PlayerID> players;
    ecs::World world;
    Tick currentTick;
    int tickRate;
    bool isActive;
    
    Room(RoomID id, int tickRate = DEFAULT_TICK_RATE) 
        : id(id), currentTick(0), tickRate(tickRate), isActive(true) {}
};

class GameServer {
private:
    std::unique_ptr<net::UDPSocket> socket;
    net::Address serverAddress;
    
    std::unordered_map<PlayerID, std::unique_ptr<Player>> players;
    std::unordered_map<RoomID, std::unique_ptr<Room>> rooms;
    
    PlayerID nextPlayerID;
    RoomID nextRoomID;
    
    Tick serverTick;
    int serverTickRate;
    
    TimePoint lastTickTime;
    float accumulatedTime;
    
    // Simple matchmaking queue (no rating system)
    std::queue<PlayerID> matchmakingQueue;
    std::set<PlayerID> playersInQueue;  // Fast lookup to prevent duplicates
    const int PLAYERS_PER_MATCH = 2;    // 2 players per match (can be changed)
    
    // Anti-Cheat system
    anticheat::AntiCheat antiCheat;
    
    void processPackets();
    void updateRooms(float deltaTime);
    void sendSnapshots();
    void processMatchmaking();  // Process matchmaking queue
    Room* getOrCreateRoom(RoomID roomID);
    EntityID createPlayerEntity(Room* room, PlayerID playerID);
    void createMapObjects(Room* room);  // Harita görselleri (duvarlar, engeller)
    void processInputPacket(Player* player, net::PacketReader& reader, SequenceNumber sequence);
    EntityID getPlayerEntity(Room* room, PlayerID playerID);
    void handleFindMatch(Player* player);
    void handleCancelMatch(Player* player);
    void sendMatchFound(PlayerID playerID, RoomID roomID);

public:
    GameServer(const std::string& bindIP, uint16_t port, int tickRate = DEFAULT_TICK_RATE);
    ~GameServer();
    
    bool initialize();
    void run();
    void shutdown();
    
    PlayerID addPlayer(const net::Address& address);
    void removePlayer(PlayerID playerID);
    RoomID createRoom(int tickRate = DEFAULT_TICK_RATE);
};

} // namespace game

