#include "Server.hpp"
#include "../net/Packet.hpp"
#include <iostream>
#include <thread>
#include <algorithm>

namespace game {

GameServer::GameServer(const std::string& bindIP, uint16_t port, int tickRate)
    : nextPlayerID(0), nextRoomID(0), serverTick(0), serverTickRate(tickRate),
      accumulatedTime(0.0f) {
    serverAddress = net::Address(bindIP, port);
    socket = std::make_unique<net::UDPSocket>();
}

GameServer::~GameServer() {
    shutdown();
}

bool GameServer::initialize() {
    if (!socket->bind(serverAddress)) {
        std::cerr << "Failed to bind server socket to " 
                  << serverAddress.ip << ":" << serverAddress.port << std::endl;
        return false;
    }
    
    lastTickTime = std::chrono::steady_clock::now();
    std::cout << "Game Server initialized on " 
              << serverAddress.ip << ":" << serverAddress.port 
              << " (Tick Rate: " << serverTickRate << ")" << std::endl;
    
    return true;
}

void GameServer::run() {
    const float fixedDeltaTime = 1.0f / serverTickRate;
    
    while (true) {
        auto currentTime = std::chrono::steady_clock::now();
        auto frameTime = std::chrono::duration<float>(currentTime - lastTickTime).count();
        lastTickTime = currentTime;
        
        // Clamp frame time
        frameTime = std::min(frameTime, MAX_DELTA_TIME);
        accumulatedTime += frameTime;
        
        // Process network packets
        processPackets();
        
        // Fixed timestep update
        while (accumulatedTime >= fixedDeltaTime) {
            updateRooms(fixedDeltaTime);
            serverTick++;
            accumulatedTime -= fixedDeltaTime;
        }
        
        // Send snapshots
        sendSnapshots();
        
        // Sleep to prevent 100% CPU usage
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void GameServer::processPackets() {
    net::Packet packet;
    while (socket->receive(packet, 0)) {
        if (packet.size < sizeof(net::PacketHeader)) {
            continue;
        }
        
        net::PacketReader reader(packet.data.data(), packet.size);
        net::PacketHeader header;
        if (!reader.read(header)) {
            continue;
        }
        
        // Find or create player
        Player* player = nullptr;
        for (auto& [id, p] : players) {
            if (p->address == packet.from) {
                player = p.get();
                break;
            }
        }
        
        if (!player && header.type == net::PacketType::CONNECT) {
            PlayerID newID = addPlayer(packet.from);
            player = players[newID].get();
            std::cout << "Player " << newID << " connected from " 
                      << packet.from.ip << ":" << packet.from.port << std::endl;
        }
        
        if (player) {
            player->lastSeenTick = serverTick;
            
            // Handle different packet types
            switch (header.type) {
                case net::PacketType::INPUT:
                    // Process player input
                    break;
                case net::PacketType::HEARTBEAT:
                    // Send heartbeat response
                    break;
                default:
                    break;
            }
        }
    }
}

void GameServer::updateRooms(float deltaTime) {
    for (auto& [roomID, room] : rooms) {
        if (room->isActive) {
            room->world.update(deltaTime);
            room->currentTick++;
        }
    }
}

void GameServer::sendSnapshots() {
    // TODO: Implement snapshot compression and delta encoding
    for (auto& [roomID, room] : rooms) {
        for (PlayerID playerID : room->players) {
            auto it = players.find(playerID);
            if (it != players.end() && it->second->connected) {
                // Send snapshot to player
                // TODO: Implement actual snapshot sending
            }
        }
    }
}

Room* GameServer::getOrCreateRoom(RoomID roomID) {
    auto it = rooms.find(roomID);
    if (it != rooms.end()) {
        return it->second.get();
    }
    
    RoomID newRoomID = createRoom();
    return rooms[newRoomID].get();
}

PlayerID GameServer::addPlayer(const net::Address& address) {
    PlayerID id = nextPlayerID++;
    players[id] = std::make_unique<Player>(id, address);
    return id;
}

void GameServer::removePlayer(PlayerID playerID) {
    auto it = players.find(playerID);
    if (it != players.end()) {
        // Remove from room
        RoomID roomID = it->second->currentRoom;
        if (roomID != INVALID_ROOM) {
            auto roomIt = rooms.find(roomID);
            if (roomIt != rooms.end()) {
                auto& roomPlayers = roomIt->second->players;
                roomPlayers.erase(
                    std::remove(roomPlayers.begin(), roomPlayers.end(), playerID),
                    roomPlayers.end()
                );
            }
        }
        players.erase(it);
        std::cout << "Player " << playerID << " disconnected" << std::endl;
    }
}

RoomID GameServer::createRoom(int tickRate) {
    RoomID id = nextRoomID++;
    rooms[id] = std::make_unique<Room>(id, tickRate);
    std::cout << "Room " << id << " created (Tick Rate: " << tickRate << ")" << std::endl;
    return id;
}

void GameServer::shutdown() {
    if (socket && socket->isOpen()) {
        socket->close();
    }
}

} // namespace game

