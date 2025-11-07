# Real-Time Game Server + ECS (FPS-lite Arena)

A real-time authoritative game server built with C++17, featuring an ECS (Entity Component System) architecture. Provides a foundation for FPS-style or arena-type games.

![Server and Client Connection](images/server-client-connection.png)

*Example of server and test client connection*

## 🎮 Features

- ✅ **ECS (Entity-Component-System)** architecture - Flexible and scalable game logic
- ✅ **Network layer** - UDP socket abstraction (Windows/Linux)
- ✅ **Room-based** server - Multiple game room support
- ✅ **60/120 tick** server loop - High-performance real-time simulation
- ✅ **Physics** layer - BVH spatial partitioning with collision detection
- ✅ **Matchmaker** - Rating-based game matching system
- ✅ **Anti-cheat-lite** - Basic cheat prevention controls
- ✅ **Snapshot** management - Infrastructure ready for delta compression
- ✅ **Test Client** - Simple client for testing server connections
- ⏳ Lag compensation (TODO)
- ⏳ Rollback/rewind (TODO)
- ⏳ Deterministic simulation (TODO)

## 📁 Project Structure

```
Real-Time Game Server + ECS (FPS-lite  Arena)/
├── include/
│   └── common/
│       └── types.hpp          # Basic type definitions
├── ecs/
│   ├── Entity.hpp             # Entity class
│   ├── Component.hpp/cpp      # Component base and registry
│   ├── System.hpp             # System base (template)
│   └── World.hpp/cpp          # ECS World
├── net/
│   ├── Socket.hpp/cpp         # UDP socket wrapper
│   ├── Packet.hpp             # Packet reader/writer
│   └── Snapshot.hpp/cpp       # Snapshot and delta compression
├── physics/
│   └── Physics.hpp/cpp        # Vec3, AABB, BVH
├── matchmaker/
│   └── Matchmaker.hpp/cpp     # Game matching
├── anti-cheat-lite/
│   └── AntiCheat.hpp/cpp      # Basic anti-cheat controls
├── src/
│   ├── Server.hpp/cpp         # Main server class
│   ├── main.cpp               # Server entry point
│   └── TestClient.cpp         # Test client
├── build.bat                  # Server build script
├── build-client.bat           # Client build script
└── README.md
```

## 🛠️ Installation and Building

### Requirements

- **Windows 10/11**
- **MinGW** (Minimalist GNU for Windows) - Must be installed in `D:\MinGW\bin` directory
- **C++17** compatible compiler (g++ 7.0+)

### MinGW Installation

Ensure MinGW is installed in the `D:\MinGW` directory. If it's installed in a different location, update the `MINGW_PATH` variable in `build.bat` and `build-client.bat` files.

### Building the Server

1. Navigate to the project directory:
```bash
cd "D:\Real-Time Game Server + ECS (FPS-lite  Arena)"
```

2. Run `build.bat` to build the server:
```bash
.\build.bat
```

This will create the `GameServer.exe` executable.

### Building the Client

1. Navigate to the project directory:
```bash
cd "D:\Real-Time Game Server + ECS (FPS-lite  Arena)"
```

2. Run `build-client.bat` to build the client:
```bash
.\build-client.bat
```

This will create the `TestClient.exe` executable.

## 🚀 Usage

### Running the Server

To start the server, run the `GameServer.exe` file:

```bash
# Default: port 7777, 60 tick
GameServer.exe

# Custom port and tick rate
GameServer.exe 7777 60
```

When the server starts successfully, you will see the following output:
```
=== Game Server (FPS-lite / Arena) ===
C++17 | ECS | Authoritative Server
=====================================
Game Server initialized on 0.0.0.0:7777 (Tick Rate: 60)
Server running. Press Ctrl+C to stop.
```

### Running the Client

While the server is running, open a **new terminal window** and run the client:

```bash
# Default: 127.0.0.1:7777
TestClient.exe

# For a different server address and port
TestClient.exe 127.0.0.1 7777
```

The client will:
1. Send a `CONNECT` packet to the server
2. Wait for server response
3. Send `HEARTBEAT` packets every 2 seconds
4. Listen for packets from the server

### Connection Test

You should see the following message in the server terminal:
```
Player 0 connected from 127.0.0.1:XXXXX
```

This indicates that the client has successfully connected.

## 🔧 Technical Details

### ECS Architecture

- **Entity**: ID + component mask
- **Component**: Type-safe component registry
- **System**: Template-based system with component filtering
- **World**: Entity/component/system management

### Network

- UDP socket abstraction (Windows/Linux compatible)
- Packet header: type, sequence, tick, playerID
- Snapshot history (64 snapshots)
- Infrastructure ready for delta compression (implementation TODO)

### Physics

- BVH (Bounding Volume Hierarchy) spatial partitioning
- AABB collision detection
- Vec3 math library

### Matchmaker

- Rating-based matching
- Configurable tolerance
- Team size support

### Anti-Cheat

- Packet rate limiting
- Movement speed validation
- Suspicious action tracking

## 📊 Development Status

**✅ Completed:**
- Basic architecture
- ECS framework
- Network layer
- Server framework
- Basic physics structure
- Test client

**⏳ In Progress:**
- Snapshot serialization
- Delta compression implementation
- Lag compensation
- Rollback/rewind

**📋 Planned:**
- Deterministic simulation
- Lua/AngelScript scripting
- Glicko-2 rating system
- Profiling tools

## ⚠️ Important Notes

- **C++ Standard**: The project uses C++17 standard (due to MinGW 13.2.0 C++20 incompatibility)
- **MinGW Version**: Tested with MinGW 13.2.0
- **Platform**: Currently optimized for Windows, Linux support is planned
- **Production Use**: This project is in active development. Additional tests and optimizations are required for production use

## 📝 License

This project is for educational/learning purposes.

## 🤝 Contributing

Contributions are welcome! Before submitting a pull request, please:
1. Ensure your code is clean and readable
2. Follow the existing code style
3. Test the connection with the test client

## 📧 Contact

Feel free to open an issue for questions or suggestions.
