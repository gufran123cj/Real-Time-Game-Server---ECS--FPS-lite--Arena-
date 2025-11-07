#include "Socket.hpp"
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

namespace game::net {

UDPSocket::UDPSocket() : socketHandle(-1), bound(false) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#else
    socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int flags = fcntl(socketHandle, F_GETFL, 0);
    fcntl(socketHandle, F_SETFL, flags | O_NONBLOCK);
#endif
}

UDPSocket::~UDPSocket() {
    close();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool UDPSocket::bind(const Address& address) {
    if (socketHandle < 0) return false;
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(address.port);
    
    if (address.ip.empty() || address.ip == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, address.ip.c_str(), &addr.sin_addr);
    }
    
    int result = ::bind(socketHandle, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (result == 0) {
        bound = true;
        return true;
    }
    return false;
}

bool UDPSocket::send(const Address& to, const void* data, size_t size) {
    if (!bound || socketHandle < 0) return false;
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(to.port);
    inet_pton(AF_INET, to.ip.c_str(), &addr.sin_addr);
    
    int sent = sendto(socketHandle, reinterpret_cast<const char*>(data), 
                      static_cast<int>(size), 0,
                      reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    return sent == static_cast<int>(size);
}

bool UDPSocket::receive(Packet& packet, int timeoutMs) {
    if (!bound || socketHandle < 0) return false;
    
    sockaddr_in fromAddr{};
    socklen_t fromLen = sizeof(fromAddr);
    
    packet.data.resize(MAX_PACKET_SIZE);
    
    int received = recvfrom(socketHandle, 
                           reinterpret_cast<char*>(packet.data.data()),
                           MAX_PACKET_SIZE, 0,
                           reinterpret_cast<sockaddr*>(&fromAddr), &fromLen);
    
    if (received > 0) {
        packet.size = received;
        packet.data.resize(received);
        
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &fromAddr.sin_addr, ipStr, INET_ADDRSTRLEN);
        packet.from.ip = ipStr;
        packet.from.port = ntohs(fromAddr.sin_port);
        packet.timestamp = std::chrono::steady_clock::now();
        
        return true;
    }
    
    return false;
}

void UDPSocket::close() {
    if (socketHandle >= 0) {
#ifdef _WIN32
        closesocket(socketHandle);
#else
        ::close(socketHandle);
#endif
        socketHandle = -1;
        bound = false;
    }
}

bool UDPSocket::isOpen() const {
    return socketHandle >= 0 && bound;
}

} // namespace game::net

