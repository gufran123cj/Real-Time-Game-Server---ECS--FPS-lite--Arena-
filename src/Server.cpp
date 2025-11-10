#include "Server.hpp"
#include "../net/Packet.hpp"
#include "../components/Components.hpp"
#include "../components/CollisionComponent.hpp"
#include "../systems/MovementSystem.hpp"
#include "../systems/PhysicsSystem.hpp"
#include "../physics/Physics.hpp"
#include <iostream>
#include <thread>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <random>
#include <ctime>

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
    
    std::cout << "[DEBUG] GameServer::run() started, tickRate=" << serverTickRate 
              << ", fixedDeltaTime=" << fixedDeltaTime << std::endl;
    
    int loopCount = 0;
    while (true) {
        loopCount++;
        
        // Debug: Log first few loop iterations
        if (loopCount <= 10) {
            std::cout << "[DEBUG] run() loop iteration #" << loopCount << std::endl;
        }
        
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
            
            if (loopCount <= 10) {
                std::cout << "[DEBUG] Fixed timestep update: serverTick=" << serverTick << std::endl;
            }
        }
        
        // Process matchmaking queue (every tick)
        processMatchmaking();
        
        // Send snapshots
        if (loopCount <= 10) {
            std::cout << "[DEBUG] About to call sendSnapshots()..." << std::endl;
        }
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
        
        // Debug: Log all received packets (only first few times)
        static int debugPacketCount = 0;
        if (debugPacketCount < 5) {
            std::cout << "[DEBUG] Received packet type=" << (int)header.type 
                      << " from " << packet.from.ip << ":" << packet.from.port << std::endl;
            debugPacketCount++;
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
            
            // Create default room and add player entity
            Room* defaultRoom = getOrCreateRoom(0);
            createPlayerEntity(defaultRoom, newID);
            player->currentRoom = defaultRoom->id;
            defaultRoom->players.push_back(newID);
            
            std::cout << "Player " << newID << " connected from " 
                      << packet.from.ip << ":" << packet.from.port << std::endl;
        }
        
        // HEARTBEAT from unknown players is ignored - only CONNECT creates new players
        // (This prevents creating duplicate players from HEARTBEAT packets)
        
        if (player) {
            player->lastSeenTick = serverTick;
            
            // Handle different packet types
            switch (header.type) {
                case net::PacketType::INPUT:
                    processInputPacket(player, reader, header.sequence);
                    break;
                case net::PacketType::HEARTBEAT:
                    // Heartbeat received - snapshot will be sent in sendSnapshots()
                    // Also handle viewer connections (players not in room)
                    if (player->currentRoom == INVALID_ROOM) {
                        // This might be a viewer - add to default room for snapshot access
                        Room* defaultRoom = getOrCreateRoom(0);
                        player->currentRoom = defaultRoom->id;
                        std::cout << "[DEBUG] HEARTBEAT from Player " << player->id 
                                  << " - added to room " << defaultRoom->id << std::endl;
                    } else {
                        // Debug: Log HEARTBEAT from known player
                        static int debugHeartbeatKnownCount = 0;
                        if (debugHeartbeatKnownCount < 3) {
                            std::cout << "[DEBUG] HEARTBEAT from Player " << player->id 
                                      << " (room=" << player->currentRoom << ")" << std::endl;
                            debugHeartbeatKnownCount++;
                        }
                    }
                    break;
                case net::PacketType::FIND_MATCH:
                    handleFindMatch(player);
                    break;
                case net::PacketType::CANCEL_MATCH:
                    handleCancelMatch(player);
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
            
            // Anti-Cheat: Validate movement speed for all players in room
            // Also: Clear stale input (if no input received for 60 ticks = 1 second)
            Room* roomPtr = room.get();
            const Tick INPUT_TIMEOUT_TICKS = 60;  // 1 saniye timeout
            
            for (PlayerID playerID : roomPtr->players) {
                auto playerIt = players.find(playerID);
                if (playerIt == players.end() || !playerIt->second->connected) {
                    continue;
                }
                
                EntityID entityID = getPlayerEntity(roomPtr, playerID);
                if (entityID == INVALID_ENTITY) continue;
                
                // Clear stale input: Eğer son input 60 tick'ten eskiyse, flags'ı sıfırla
                auto* inputComp = roomPtr->world.getComponent<components::InputComponent>(entityID);
                if (inputComp) {
                    Tick ticksSinceLastInput = roomPtr->currentTick - inputComp->inputTick;
                    if (ticksSinceLastInput > INPUT_TIMEOUT_TICKS) {
                        // Input çok eski, flags'ı sıfırla (karakterin durması için)
                        inputComp->flags = components::INPUT_NONE;
                    }
                }
                
                // Anti-Cheat: Movement validation TEMPORARILY DISABLED for testing
                /*
                auto* posComp = roomPtr->world.getComponent<components::Position>(entityID);
                if (posComp) {
                    // Validate movement speed
                    if (!antiCheat.validateMovement(playerID, posComp->value, deltaTime)) {
                        std::cout << "[Anti-Cheat] Player " << playerID 
                                  << " exceeded movement speed limit (suspicious: " 
                                  << antiCheat.getSuspiciousCount(playerID) << ")" << std::endl;
                        
                        // Check if should kick
                        if (antiCheat.shouldKick(playerID)) {
                            std::cout << "[Anti-Cheat] Kicking Player " << playerID 
                                      << " for suspicious movement" << std::endl;
                            removePlayer(playerID);
                            continue;
                        }
                    }
                }
                */
            }
            
            // Mini Game map rendering moved to MiniGameViewer.exe
            // (Will use snapshot system in Phase 4)
        }
    }
}

void GameServer::sendSnapshots() {
    // Phase 4: Component-based snapshot serialization
    // Send snapshot to all connected players/viewers from default room (room 0)
    
    // Debug: Log function entry (always log first 50 calls)
    static int functionEntryCount = 0;
    functionEntryCount++;
    if (functionEntryCount <= 50) {
        std::cout << "[DEBUG] >>> sendSnapshots() ENTRY #" << functionEntryCount 
                  << " at tick " << serverTick << std::endl;
    }
    
    Room* defaultRoom = nullptr;
    auto roomIt = rooms.find(0);
    if (roomIt != rooms.end()) {
        defaultRoom = roomIt->second.get();
    }
    
    if (!defaultRoom || !defaultRoom->isActive) {
        // Debug: Log why snapshot is not sent
        static int debugNoRoomCount = 0;
        if (debugNoRoomCount < 3) {
            if (!defaultRoom) {
                std::cout << "[DEBUG] sendSnapshots: Room 0 not found, rooms.size()=" << rooms.size() << std::endl;
            } else if (!defaultRoom->isActive) {
                std::cout << "[DEBUG] sendSnapshots: Room 0 is not active" << std::endl;
            }
            debugNoRoomCount++;
        }
        return; // No default room yet
    }
    
    // Get all player entities in default room
    auto playerEntities = defaultRoom->world.queryEntities<components::PlayerComponent>();
    
    // Get all static objects (walls) - entities with Position + CollisionComponent (static)
    auto allEntities = defaultRoom->world.queryEntities<components::Position, components::CollisionComponent>();
    std::vector<EntityID> wallEntities;
    for (EntityID entityID : allEntities) {
        // Check if it's a static object (wall) and not a player
        auto* collision = defaultRoom->world.getComponent<components::CollisionComponent>(entityID);
        auto* playerComp = defaultRoom->world.getComponent<components::PlayerComponent>(entityID);
        if (collision && collision->isStatic && !playerComp) {
            wallEntities.push_back(entityID);
        }
    }
    
    // Debug: Log room state (always log first 20 times)
    static int debugRoomStateCount = 0;
    if (debugRoomStateCount < 20) {
        std::cout << "[DEBUG] sendSnapshots: Room 0 active, " << playerEntities.size() 
                  << " player entities, " << wallEntities.size() << " wall entities, " 
                  << players.size() << " connected players" << std::endl;
        debugRoomStateCount++;
    }
    
    // Send snapshot to all connected players/viewers (every 10 ticks = ~6 times per second at 60fps)
    static Tick lastSnapshotTick = 0;
    static bool firstCall = true;
    
    // Debug: Log EVERY call to sendSnapshots (for debugging)
    static int totalCalls = 0;
    totalCalls++;
    if (totalCalls <= 20) {
        std::cout << "[DEBUG] sendSnapshots called #" << totalCalls << " at tick " << serverTick 
                  << ", players.size()=" << players.size() << ", rooms.size()=" << rooms.size() << std::endl;
    }
    
    // Always send on first call, then every 10 ticks
    if (!firstCall && (serverTick - lastSnapshotTick) < 10) {
        if (totalCalls <= 20) {
            std::cout << "[DEBUG] sendSnapshots: Throttled (lastTick=" << lastSnapshotTick 
                      << ", currentTick=" << serverTick << ", diff=" << (serverTick - lastSnapshotTick) << ")" << std::endl;
        }
        return; // Throttle snapshot sending
    }
    
    firstCall = false;
    lastSnapshotTick = serverTick;
    
    if (totalCalls <= 20) {
        std::cout << "[DEBUG] sendSnapshots: Proceeding to send snapshot..." << std::endl;
    }
    
    if (players.empty()) {
        static int debugEmptyPlayersCount = 0;
        if (debugEmptyPlayersCount < 2) {
            std::cout << "[DEBUG] sendSnapshots: No players connected yet" << std::endl;
            debugEmptyPlayersCount++;
        }
        return;
    }
    
    for (auto& [playerID, player] : players) {
        if (!player->connected) {
            static int debugDisconnectedCount = 0;
            if (debugDisconnectedCount < 2) {
                std::cout << "[DEBUG] Player " << playerID << " is not connected" << std::endl;
                debugDisconnectedCount++;
            }
            continue;
        }
        
        // Build snapshot packet
        net::PacketWriter writer;
        net::PacketHeader header;
        header.type = net::PacketType::SNAPSHOT;
        header.sequence = 0;
        header.serverTick = serverTick;
        header.playerID = playerID;
        
        writer.write(header);
        
        // Phase 4: Component-based snapshot serialization
        // Write entity count (players + walls)
        uint8_t entityCount = static_cast<uint8_t>(playerEntities.size() + wallEntities.size());
        writer.write(entityCount);
        
        // Write player entities first
        for (EntityID entityID : playerEntities) {
            // Write Entity ID
            writer.write(entityID);
            
            // Get components
            auto* pos = defaultRoom->world.getComponent<components::Position>(entityID);
            auto* playerComp = defaultRoom->world.getComponent<components::PlayerComponent>(entityID);
            auto* input = defaultRoom->world.getComponent<components::InputComponent>(entityID);
            auto* transform = defaultRoom->world.getComponent<components::Transform>(entityID);
            auto* health = defaultRoom->world.getComponent<components::Health>(entityID);
            
            // Count components to serialize (only relevant ones for snapshot)
            uint8_t componentCount = 0;
            if (pos) componentCount++;
            if (playerComp) componentCount++;
            if (input) componentCount++;
            if (transform) componentCount++;
            if (health) componentCount++;
            
            writer.write(componentCount);
            
            // Serialize each component (Type ID + Size + Data)
            // Size is needed so viewer can skip unknown components
            // Debug: Log component type IDs (always log first 20 components)
            static int debugServerComponentCount = 0;
            bool shouldLog = (debugServerComponentCount < 20);
            
            // Log entity info
            if (shouldLog && playerEntities.size() > 0 && playerEntities[0] == entityID) {
                std::cout << "[DEBUG SERVER] Entity " << entityID << " components: pos=" << (pos ? "YES" : "NO")
                          << ", player=" << (playerComp ? "YES" : "NO")
                          << ", input=" << (input ? "YES" : "NO")
                          << ", transform=" << (transform ? "YES" : "NO")
                          << ", health=" << (health ? "YES" : "NO") << std::endl;
            }
            
            if (pos) {
                ComponentTypeID typeID = pos->getTypeID();
                size_t actualSize = pos->getSerializedSize();
                uint16_t size = static_cast<uint16_t>(actualSize);
                if (shouldLog) {
                    std::cout << "[DEBUG SERVER] Serializing Position: typeID=" << typeID 
                              << ", calculatedSize=" << actualSize << ", writtenSize=" << size << std::endl;
                }
                writer.write(typeID);
                writer.write(size);
                size_t beforeWrite = writer.getSize();
                pos->serialize(writer);
                size_t afterWrite = writer.getSize();
                if (shouldLog) {
                    std::cout << "[DEBUG SERVER] Position written: " << (afterWrite - beforeWrite) << " bytes" << std::endl;
                }
            }
            if (playerComp) {
                ComponentTypeID typeID = playerComp->getTypeID();
                size_t actualSize = playerComp->getSerializedSize();
                uint16_t size = static_cast<uint16_t>(actualSize);
                if (shouldLog) {
                    std::cout << "[DEBUG SERVER] Serializing PlayerComponent: typeID=" << typeID 
                              << ", calculatedSize=" << actualSize << ", writtenSize=" << size 
                              << ", playerID=" << playerComp->playerID << std::endl;
                }
                writer.write(typeID);
                writer.write(size);
                size_t beforeWrite = writer.getSize();
                playerComp->serialize(writer);
                size_t afterWrite = writer.getSize();
                if (shouldLog) {
                    std::cout << "[DEBUG SERVER] PlayerComponent written: " << (afterWrite - beforeWrite) << " bytes" << std::endl;
                }
            }
            if (input) {
                ComponentTypeID typeID = input->getTypeID();
                size_t actualSize = input->getSerializedSize();
                uint16_t size = static_cast<uint16_t>(actualSize);
                if (shouldLog) {
                    std::cout << "[DEBUG SERVER] Serializing InputComponent: typeID=" << typeID 
                              << ", calculatedSize=" << actualSize << ", writtenSize=" << size << std::endl;
                }
                writer.write(typeID);
                writer.write(size);
                size_t beforeWrite = writer.getSize();
                input->serialize(writer);
                size_t afterWrite = writer.getSize();
                if (shouldLog) {
                    std::cout << "[DEBUG SERVER] InputComponent written: " << (afterWrite - beforeWrite) << " bytes" << std::endl;
                }
            }
            if (transform) {
                ComponentTypeID typeID = transform->getTypeID();
                size_t actualSize = transform->getSerializedSize();
                uint16_t size = static_cast<uint16_t>(actualSize);
                if (shouldLog) {
                    std::cout << "[DEBUG SERVER] Serializing Transform: typeID=" << typeID 
                              << ", calculatedSize=" << actualSize << ", writtenSize=" << size << std::endl;
                }
                writer.write(typeID);
                writer.write(size);
                size_t beforeWrite = writer.getSize();
                transform->serialize(writer);
                size_t afterWrite = writer.getSize();
                if (shouldLog) {
                    std::cout << "[DEBUG SERVER] Transform written: " << (afterWrite - beforeWrite) << " bytes" << std::endl;
                }
            }
            if (health) {
                ComponentTypeID typeID = health->getTypeID();
                size_t actualSize = health->getSerializedSize();
                uint16_t size = static_cast<uint16_t>(actualSize);
                if (shouldLog) {
                    std::cout << "[DEBUG SERVER] Serializing Health: typeID=" << typeID 
                              << ", calculatedSize=" << actualSize << ", writtenSize=" << size << std::endl;
                }
                writer.write(typeID);
                writer.write(size);
                size_t beforeWrite = writer.getSize();
                health->serialize(writer);
                size_t afterWrite = writer.getSize();
                if (shouldLog) {
                    std::cout << "[DEBUG SERVER] Health written: " << (afterWrite - beforeWrite) << " bytes" << std::endl;
                }
            }
            
            if (shouldLog) {
                debugServerComponentCount++;
            }
        }
        
        // Write wall entities (static objects)
        for (EntityID entityID : wallEntities) {
            // Write Entity ID
            writer.write(entityID);
            
            // Get components for walls
            auto* pos = defaultRoom->world.getComponent<components::Position>(entityID);
            auto* collision = defaultRoom->world.getComponent<components::CollisionComponent>(entityID);
            
            // Count components to serialize
            uint8_t componentCount = 0;
            if (pos) componentCount++;
            if (collision) componentCount++;
            
            writer.write(componentCount);
            
            // Serialize Position
            if (pos) {
                ComponentTypeID typeID = pos->getTypeID();
                size_t actualSize = pos->getSerializedSize();
                uint16_t size = static_cast<uint16_t>(actualSize);
                writer.write(typeID);
                writer.write(size);
                pos->serialize(writer);
            }
            
            // Serialize CollisionComponent
            if (collision) {
                ComponentTypeID typeID = collision->getTypeID();
                size_t actualSize = collision->getSerializedSize();
                uint16_t size = static_cast<uint16_t>(actualSize);
                writer.write(typeID);
                writer.write(size);
                collision->serialize(writer);
            }
        }
        
        // Send snapshot (even if empty - viewer needs to know there are no players)
        if (writer.getSize() > sizeof(net::PacketHeader)) {
            if (socket->send(player->address, writer.getData(), writer.getSize())) {
                // Debug: Log snapshot sending (always log first 10)
                static int debugSnapshotCount = 0;
                if (debugSnapshotCount < 10) {
                    std::cout << "[DEBUG] Snapshot sent to Player " << playerID 
                              << " at " << player->address.ip << ":" << player->address.port
                              << " (" << (int)entityCount << " entities, " << writer.getSize() << " bytes)" << std::endl;
                    debugSnapshotCount++;
                }
            } else {
                std::cout << "[ERROR] Failed to send snapshot to Player " << playerID << std::endl;
            }
        } else {
            // Debug: Log empty snapshot (always log first few)
            static int debugEmptySnapshotCount = 0;
            if (debugEmptySnapshotCount < 5) {
                std::cout << "[DEBUG] Snapshot too small to send (only header, size=" << writer.getSize() << ")" << std::endl;
                debugEmptySnapshotCount++;
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
        
        // Remove from matchmaking queue
        playersInQueue.erase(playerID);
        
        // Reset anti-cheat stats
        antiCheat.resetPlayer(playerID);
        
        players.erase(it);
        std::cout << "Player " << playerID << " disconnected" << std::endl;
    }
}

RoomID GameServer::createRoom(int tickRate) {
    RoomID id = nextRoomID++;
    rooms[id] = std::make_unique<Room>(id, tickRate);
    
    // Add Movement System to room's world
    auto movementSystem = std::make_unique<systems::MovementSystem>();
    rooms[id]->world.addSystem(std::move(movementSystem));
    
    // Add Physics System to room's world (Phase 5)
    auto physicsSystem = std::make_unique<systems::PhysicsSystem>();
    rooms[id]->world.addSystem(std::move(physicsSystem));
    
    // Create map objects (walls, obstacles)
    createMapObjects(rooms[id].get());
    
    std::cout << "Room " << id << " created (Tick Rate: " << tickRate << ") - Movement & Physics Systems added" << std::endl;
    std::cout << "Map size: 150x150 (-75 to +75)" << std::endl;
    return id;
}

EntityID GameServer::createPlayerEntity(Room* room, PlayerID playerID) {
    if (!room) return INVALID_ENTITY;
    
    // Harita sınırları (PhysicsSystem'deki worldBounds ile aynı) - 150x150 map
    const float MAP_MIN = -75.0f;
    const float MAP_MAX = 75.0f;
    const float MIN_SPAWN_DISTANCE = 5.0f;  // Mevcut oyunculara minimum mesafe
    const int MAX_SPAWN_ATTEMPTS = 50;  // Maksimum deneme sayısı
    
    // Mevcut oyuncuların konumlarını topla
    auto existingPlayers = room->world.queryEntities<components::PlayerComponent>();
    std::vector<physics::Vec3> existingPositions;
    for (EntityID existingEntityID : existingPlayers) {
        auto* existingPos = room->world.getComponent<components::Position>(existingEntityID);
        if (existingPos) {
            existingPositions.push_back(existingPos->value);
        }
    }
    
    // Random spawn konumu bul (mevcut oyunculara çok yakın olmayan)
    static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_real_distribution<float> distX(MAP_MIN, MAP_MAX);
    std::uniform_real_distribution<float> distY(MAP_MIN, MAP_MAX);
    
    float spawnX = 0.0f;
    float spawnY = 0.0f;
    float spawnZ = 0.0f;
    bool foundValidSpawn = false;
    
    // Uygun spawn konumu bul
    for (int attempt = 0; attempt < MAX_SPAWN_ATTEMPTS; ++attempt) {
        spawnX = distX(rng);
        spawnY = distY(rng);
        spawnZ = 0.0f;
        
        // Mevcut oyunculara minimum mesafede mi kontrol et
        bool tooClose = false;
        for (const auto& existingPos : existingPositions) {
            float dx = spawnX - existingPos.x;
            float dy = spawnY - existingPos.y;
            float distance = std::sqrt(dx * dx + dy * dy);
            
            if (distance < MIN_SPAWN_DISTANCE) {
                tooClose = true;
                break;
            }
        }
        
        if (!tooClose) {
            foundValidSpawn = true;
            break;
        }
    }
    
    // Eğer uygun konum bulunamazsa, merkeze yakın bir yere spawn et
    if (!foundValidSpawn) {
        spawnX = distX(rng) * 0.3f;  // Merkeze yakın
        spawnY = distY(rng) * 0.3f;
        std::cout << "[Spawn] Warning: Could not find ideal spawn position for Player " 
                  << playerID << ", using fallback position (" << spawnX << ", " << spawnY << ")" << std::endl;
    } else {
        std::cout << "[Spawn] Player " << playerID << " spawned at (" << spawnX << ", " << spawnY << ")" << std::endl;
    }
    
    // Create entity
    EntityID entityID = room->world.createEntity();
    
    // Add components
    auto position = std::make_unique<components::Position>(spawnX, spawnY, spawnZ);
    auto velocity = std::make_unique<components::Velocity>(0.0f, 0.0f, 0.0f);
    auto health = std::make_unique<components::Health>(100.0f);
    auto playerComp = std::make_unique<components::PlayerComponent>(playerID);
    auto transform = std::make_unique<components::Transform>();
    
    room->world.addComponent<components::Position>(entityID, std::move(position));
    room->world.addComponent<components::Velocity>(entityID, std::move(velocity));
    room->world.addComponent<components::Health>(entityID, std::move(health));
    room->world.addComponent<components::PlayerComponent>(entityID, std::move(playerComp));
    room->world.addComponent<components::Transform>(entityID, std::move(transform));
    
    // Add InputComponent for player input handling
    auto input = std::make_unique<components::InputComponent>();
    room->world.addComponent<components::InputComponent>(entityID, std::move(input));
    
    // Add CollisionComponent for physics (Phase 5)
    auto collision = components::CollisionComponent::fromCenterSize(
        physics::Vec3(spawnX, spawnY, spawnZ),  // Spawn konumunda collision
        physics::Vec3(1.0f, 2.0f, 1.0f), // Player size: 1x2x1 (width x height x depth)
        false, // Not static
        false  // Not a trigger
    );
    room->world.addComponent<components::CollisionComponent>(entityID, 
        std::make_unique<components::CollisionComponent>(collision));
    
    // Verify components were added correctly (PHASE 1 TEST)
    auto* pos = room->world.getComponent<components::Position>(entityID);
    auto* vel = room->world.getComponent<components::Velocity>(entityID);
    auto* hp = room->world.getComponent<components::Health>(entityID);
    auto* player = room->world.getComponent<components::PlayerComponent>(entityID);
    auto* trans = room->world.getComponent<components::Transform>(entityID);
    
    std::cout << "\n=== PHASE 1 TEST: Player Entity Created ===" << std::endl;
    std::cout << "Entity ID: " << entityID << " | Player ID: " << playerID << std::endl;
    
    if (pos) {
        std::cout << "  [OK] Position: (" << pos->value.x << ", " << pos->value.y << ", " << pos->value.z << ")" << std::endl;
    } else {
        std::cout << "  [FAIL] Position: FAILED" << std::endl;
    }
    
    if (vel) {
        std::cout << "  [OK] Velocity: (" << vel->value.x << ", " << vel->value.y << ", " << vel->value.z << ")" << std::endl;
    } else {
        std::cout << "  [FAIL] Velocity: FAILED" << std::endl;
    }
    
    if (hp) {
        std::cout << "  [OK] Health: " << hp->current << "/" << hp->maximum << " (" << (hp->isAlive ? "Alive" : "Dead") << ")" << std::endl;
    } else {
        std::cout << "  [FAIL] Health: FAILED" << std::endl;
    }
    
    if (player) {
        std::cout << "  [OK] PlayerComponent: ID=" << player->playerID << ", Rating=" << player->rating << std::endl;
    } else {
        std::cout << "  [FAIL] PlayerComponent: FAILED" << std::endl;
    }
    
    if (trans) {
        std::cout << "  [OK] Transform: Pos(" << trans->position.x << ", " << trans->position.y << ", " << trans->position.z << ")" << std::endl;
    } else {
        std::cout << "  [FAIL] Transform: FAILED" << std::endl;
    }
    
    // Test ECS query system
    auto playerEntities = room->world.queryEntities<components::PlayerComponent>();
    std::cout << "  [OK] ECS Query: Found " << playerEntities.size() << " player entity/entities in world" << std::endl;
    
    std::cout << "==========================================\n" << std::endl;
    
    return entityID;
}

void GameServer::createMapObjects(Room* room) {
    if (!room) return;
    
    // Harita sınırları (150x150)
    const float MAP_MIN = -75.0f;
    const float MAP_MAX = 75.0f;
    
    // Köşe duvarları ve engeller
    std::vector<std::pair<physics::Vec3, physics::Vec3>> wallPositions = {
        // Köşe blokları (büyük)
        {physics::Vec3(-70.0f, -70.0f, 0.0f), physics::Vec3(8.0f, 8.0f, 2.0f)},  // Sol-alt köşe
        {physics::Vec3(70.0f, -70.0f, 0.0f), physics::Vec3(8.0f, 8.0f, 2.0f)},   // Sağ-alt köşe
        {physics::Vec3(-70.0f, 70.0f, 0.0f), physics::Vec3(8.0f, 8.0f, 2.0f)},   // Sol-üst köşe
        {physics::Vec3(70.0f, 70.0f, 0.0f), physics::Vec3(8.0f, 8.0f, 2.0f)},    // Sağ-üst köşe
        
        // Orta bölge engelleri (küçük bloklar)
        {physics::Vec3(0.0f, 0.0f, 0.0f), physics::Vec3(6.0f, 6.0f, 2.0f)},      // Merkez
        {physics::Vec3(-30.0f, -30.0f, 0.0f), physics::Vec3(4.0f, 4.0f, 2.0f)},  // Sol-alt bölge
        {physics::Vec3(30.0f, -30.0f, 0.0f), physics::Vec3(4.0f, 4.0f, 2.0f)},   // Sağ-alt bölge
        {physics::Vec3(-30.0f, 30.0f, 0.0f), physics::Vec3(4.0f, 4.0f, 2.0f)},   // Sol-üst bölge
        {physics::Vec3(30.0f, 30.0f, 0.0f), physics::Vec3(4.0f, 4.0f, 2.0f)},    // Sağ-üst bölge
        
        // Ek engeller (rastgele dağılım)
        {physics::Vec3(-50.0f, 0.0f, 0.0f), physics::Vec3(3.0f, 3.0f, 2.0f)},
        {physics::Vec3(50.0f, 0.0f, 0.0f), physics::Vec3(3.0f, 3.0f, 2.0f)},
        {physics::Vec3(0.0f, -50.0f, 0.0f), physics::Vec3(3.0f, 3.0f, 2.0f)},
        {physics::Vec3(0.0f, 50.0f, 0.0f), physics::Vec3(3.0f, 3.0f, 2.0f)},
    };
    
    int wallCount = 0;
    for (const auto& [center, size] : wallPositions) {
        EntityID wallEntity = room->world.createEntity();
        
        // Position component
        auto position = std::make_unique<components::Position>(center.x, center.y, center.z);
        room->world.addComponent<components::Position>(wallEntity, std::move(position));
        
        // CollisionComponent (static wall)
        auto collision = components::CollisionComponent::fromCenterSize(
            center,
            size,
            true,  // isStatic = true (duvarlar hareket etmez)
            false  // isTrigger = false (duvarlar collision yapar)
        );
        room->world.addComponent<components::CollisionComponent>(wallEntity,
            std::make_unique<components::CollisionComponent>(collision));
        
        wallCount++;
    }
    
    std::cout << "[Map] Created " << wallCount << " map objects (walls/obstacles)" << std::endl;
}

EntityID GameServer::getPlayerEntity(Room* room, PlayerID playerID) {
    if (!room) return INVALID_ENTITY;
    
    // Find entity with matching PlayerComponent
    auto playerEntities = room->world.queryEntities<components::PlayerComponent>();
    for (EntityID entityID : playerEntities) {
        auto* playerComp = room->world.getComponent<components::PlayerComponent>(entityID);
        if (playerComp && playerComp->playerID == playerID) {
            return entityID;
        }
    }
    
    return INVALID_ENTITY;
}

void GameServer::processInputPacket(Player* player, net::PacketReader& reader, SequenceNumber sequence) {
    if (!player || player->currentRoom == INVALID_ROOM) return;
    
    // Anti-Cheat: TEMPORARILY DISABLED for testing
    // TODO: Re-enable and fix rate limiting (should not count HEARTBEAT packets)
    /*
    antiCheat.recordPacket(player->id);
    
    if (!antiCheat.checkPacketRate(player->id)) {
        std::cout << "[Anti-Cheat] Player " << player->id 
                  << " exceeded packet rate limit (suspicious: " 
                  << antiCheat.getSuspiciousCount(player->id) << ")" << std::endl;
        
        // Check if should kick
        if (antiCheat.shouldKick(player->id)) {
            std::cout << "[Anti-Cheat] Kicking Player " << player->id << " for suspicious activity" << std::endl;
            removePlayer(player->id);
            return;
        }
    }
    */
    
    // Find player's room
    auto roomIt = rooms.find(player->currentRoom);
    if (roomIt == rooms.end()) return;
    
    Room* room = roomIt->second.get();
    
    // Find player's entity
    EntityID entityID = getPlayerEntity(room, player->id);
    if (entityID == INVALID_ENTITY) return;
    
    // Read input packet
    net::InputPacket inputPacket;
    if (!reader.read(inputPacket)) {
        return; // Invalid packet
    }
    
    // Get or create InputComponent
    auto* inputComp = room->world.getComponent<components::InputComponent>(entityID);
    if (!inputComp) {
        // Create InputComponent if it doesn't exist
        auto input = std::make_unique<components::InputComponent>();
        inputComp = room->world.addComponent<components::InputComponent>(entityID, std::move(input));
    }
    
    if (inputComp) {
        // Update input component
        inputComp->flags = inputPacket.flags;
        inputComp->mouseYaw = inputPacket.mouseYaw;
        inputComp->mousePitch = inputPacket.mousePitch;
        inputComp->sequence = sequence;
        inputComp->inputTick = serverTick;
        
        // Debug output - her 10 pakette bir göster (daha sık)
        static Tick lastDebugTick = 0;
        static int packetCount = 0;
        packetCount++;
        
        if (serverTick - lastDebugTick >= 10 || packetCount % 10 == 0) { // Her 10 paket veya 10 tick'te bir
            std::cout << "[Player " << player->id << "] Input received: flags=" << inputPacket.flags 
                      << " yaw=" << inputPacket.mouseYaw << " pitch=" << inputPacket.mousePitch 
                      << " (seq=" << sequence << ", tick=" << serverTick << ")" << std::endl;
            lastDebugTick = serverTick;
        }
    } else {
        std::cout << "[WARNING] Player " << player->id << " - InputComponent not found!" << std::endl;
    }
}

void GameServer::shutdown() {
    if (socket && socket->isOpen()) {
        socket->close();
    }
}

// Simple Matchmaking Implementation (no rating system)
void GameServer::handleFindMatch(Player* player) {
    if (!player || !player->connected) {
        return;
    }
    
    // Check if player is already in queue
    if (playersInQueue.find(player->id) != playersInQueue.end()) {
        std::cout << "[Matchmaking] Player " << player->id << " is already in queue" << std::endl;
        return;
    }
    
    // Add player to queue
    matchmakingQueue.push(player->id);
    playersInQueue.insert(player->id);
    
    std::cout << "[Matchmaking] Player " << player->id << " added to matchmaking queue (queue size: " 
              << matchmakingQueue.size() << ")" << std::endl;
}

void GameServer::handleCancelMatch(Player* player) {
    if (!player) {
        return;
    }
    
    // Remove from set (queue will be cleaned up during processing)
    playersInQueue.erase(player->id);
    
    std::cout << "[Matchmaking] Player " << player->id << " cancelled matchmaking" << std::endl;
}

void GameServer::processMatchmaking() {
    // Try to form matches from queue
    while (matchmakingQueue.size() >= static_cast<size_t>(PLAYERS_PER_MATCH)) {
        std::vector<PlayerID> matchPlayers;
        
        // Collect players for match
        for (int i = 0; i < PLAYERS_PER_MATCH; ++i) {
            if (matchmakingQueue.empty()) {
                break;
            }
            
            PlayerID playerID = matchmakingQueue.front();
            matchmakingQueue.pop();
            
            // Check if player is still in queue set (might have cancelled)
            if (playersInQueue.find(playerID) == playersInQueue.end()) {
                continue; // Player cancelled, skip
            }
            
            // Check if player still exists and is connected
            auto it = players.find(playerID);
            if (it == players.end() || !it->second->connected) {
                playersInQueue.erase(playerID);
                continue; // Player disconnected, skip
            }
            
            matchPlayers.push_back(playerID);
            playersInQueue.erase(playerID);
        }
        
        // If we have enough players, create match
        if (matchPlayers.size() >= static_cast<size_t>(PLAYERS_PER_MATCH)) {
            // Create new room for match
            RoomID newRoomID = createRoom();
            Room* newRoom = rooms[newRoomID].get();
            
            std::cout << "[Matchmaking] Match found! Room " << newRoomID 
                      << " created with " << matchPlayers.size() << " players" << std::endl;
            
            // Add players to room and create entities
            for (PlayerID playerID : matchPlayers) {
                auto it = players.find(playerID);
                if (it != players.end() && it->second->connected) {
                    Player* player = it->second.get();
                    
                    // Remove from old room if any
                    if (player->currentRoom != INVALID_ROOM) {
                        auto oldRoomIt = rooms.find(player->currentRoom);
                        if (oldRoomIt != rooms.end()) {
                            auto& oldRoomPlayers = oldRoomIt->second->players;
                            oldRoomPlayers.erase(
                                std::remove(oldRoomPlayers.begin(), oldRoomPlayers.end(), playerID),
                                oldRoomPlayers.end()
                            );
                        }
                    }
                    
                    // Add to new room
                    player->currentRoom = newRoomID;
                    newRoom->players.push_back(playerID);
                    
                    // Create player entity in new room
                    createPlayerEntity(newRoom, playerID);
                    
                    // Notify player
                    sendMatchFound(playerID, newRoomID);
                    
                    std::cout << "[Matchmaking] Player " << playerID << " assigned to room " << newRoomID << std::endl;
                }
            }
        } else {
            // Not enough players, put them back in queue
            for (PlayerID playerID : matchPlayers) {
                matchmakingQueue.push(playerID);
                playersInQueue.insert(playerID);
            }
            break; // Can't form more matches
        }
    }
}

void GameServer::sendMatchFound(PlayerID playerID, RoomID roomID) {
    auto it = players.find(playerID);
    if (it == players.end() || !it->second->connected) {
        return;
    }
    
    Player* player = it->second.get();
    
    // Build MATCH_FOUND packet
    net::PacketWriter writer;
    net::PacketHeader header;
    header.type = net::PacketType::MATCH_FOUND;
    header.sequence = 0;
    header.serverTick = serverTick;
    header.playerID = playerID;
    
    if (!writer.write(header)) {
        return;
    }
    
    // Write room ID
    if (!writer.write(roomID)) {
        return;
    }
    
    // Send packet
    socket->send(player->address, writer.getData(), writer.getSize());
    
    std::cout << "[Matchmaking] MATCH_FOUND sent to Player " << playerID << " (Room " << roomID << ")" << std::endl;
}

} // namespace game

