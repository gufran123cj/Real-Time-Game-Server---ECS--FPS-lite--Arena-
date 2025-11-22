#include "Server.hpp"
#include <iostream>
#include <string>
#include <csignal>
#include <exception>

namespace {
    game::GameServer* g_server = nullptr;
    
    void signalHandler(int signal) {
        if (g_server) {
            std::cout << "\n[Shutdown] Signal received (" << signal << "), shutting down server..." << std::endl;
            g_server->shutdown();
        }
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    std::string bindIP = "0.0.0.0";
    uint16_t port = 7777;
    int tickRate = game::DEFAULT_TICK_RATE;
    
    if (argc >= 2) {
        try {
            port = static_cast<uint16_t>(std::stoi(argv[1]));
        } catch (const std::exception& e) {
            std::cerr << "Invalid port number: " << argv[1] << std::endl;
            return 1;
        }
    }
    
    if (argc >= 3) {
        try {
            tickRate = std::stoi(argv[2]);
            if (tickRate <= 0 || tickRate > 240) {
                std::cerr << "Invalid tick rate: " << tickRate << " (must be between 1 and 240)" << std::endl;
                return 1;
            }
        } catch (const std::exception& e) {
            std::cerr << "Invalid tick rate: " << argv[2] << std::endl;
            return 1;
        }
    }
    
    // Print header
    std::cout << "=== Game Server (FPS-lite / Arena) ===" << std::endl;
    std::cout << "C++20 | ECS | Authoritative Server" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    try {
        // Create and initialize server
        game::GameServer server(bindIP, port, tickRate);
        g_server = &server;
        
        // Setup signal handlers for graceful shutdown
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        
        // Initialize server
        if (!server.initialize()) {
            std::cerr << "[ERROR] Failed to initialize server!" << std::endl;
            return 1;
        }
        
        std::cout << "Server running. Press Ctrl+C to stop." << std::endl;
        
        // Run server (blocks until shutdown)
        server.run();
        
    } catch (const std::exception& e) {
        std::cerr << "[FATAL ERROR] Exception caught: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "[FATAL ERROR] Unknown exception caught!" << std::endl;
        return 1;
    }
    
    return 0;
}