#pragma once

#include "../include/common/types.hpp"
#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <chrono>

namespace game::net {

struct Address {
    std::string ip;
    uint16_t port;
    
    Address() : ip("0.0.0.0"), port(0) {}
    Address(const std::string& ip, uint16_t port) : ip(ip), port(port) {}
    
    bool operator==(const Address& other) const {
        return ip == other.ip && port == other.port;
    }
};

struct Packet {
    Address from;
    std::vector<uint8_t> data;
    size_t size;
    TimePoint timestamp;
    
    Packet() : size(0) {}
};

class Socket {
public:
    virtual ~Socket() = default;
    virtual bool bind(const Address& address) = 0;
    virtual bool send(const Address& to, const void* data, size_t size) = 0;
    virtual bool receive(Packet& packet, int timeoutMs = 0) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
};

class UDPSocket : public Socket {
private:
#ifdef _WIN32
    void* socketHandle;
#else
    int socketHandle;
#endif
    bool bound;

public:
    UDPSocket();
    ~UDPSocket() override;
    
    bool bind(const Address& address) override;
    bool send(const Address& to, const void* data, size_t size) override;
    bool receive(Packet& packet, int timeoutMs = 0) override;
    void close() override;
    bool isOpen() const override;
};

} // namespace game::net

