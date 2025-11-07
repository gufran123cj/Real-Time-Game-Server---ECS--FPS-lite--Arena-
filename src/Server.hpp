#pragma once

#include "../ecs/World.hpp"
#include "../net/Socket.hpp"
#include "../include/common/types.hpp"
#include <unordered_map>
#include <memory>
#include <chrono>
#include <vector>

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
    
    void processPackets();
    void updateRooms(float deltaTime);
    void sendSnapshots();
    Room* getOrCreateRoom(RoomID roomID);

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

