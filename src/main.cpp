#include "Server.hpp"
#include <iostream>
#include <csignal>

using namespace game;

static GameServer* g_server = nullptr;

void signalHandler(int signal) {
    if (g_server) {
        std::cout << "\nShutting down server..." << std::endl;
        g_server->shutdown();
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=== Game Server (FPS-lite / Arena) ===" << std::endl;
    std::cout << "C++20 | ECS | Authoritative Server" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    // Parse arguments
    std::string bindIP = "0.0.0.0";
    uint16_t port = 7777;
    int tickRate = DEFAULT_TICK_RATE;
    
    if (argc > 1) port = static_cast<uint16_t>(std::atoi(argv[1]));
    if (argc > 2) tickRate = std::atoi(argv[2]);
    
    GameServer server(bindIP, port, tickRate);
    g_server = &server;
    
    // Setup signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    if (!server.initialize()) {
        std::cerr << "Failed to initialize server" << std::endl;
        return 1;
    }
    
    std::cout << "Server running. Press Ctrl+C to stop." << std::endl;
    server.run();
    
    return 0;
}

