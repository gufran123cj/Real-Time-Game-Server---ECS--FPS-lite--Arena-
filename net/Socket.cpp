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

UDPSocket::UDPSocket() : bound(false) {
#ifdef _WIN32
    socketHandle = nullptr;
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        // WSAStartup failed - socket operations will fail
        socketHandle = reinterpret_cast<void*>(INVALID_SOCKET);
        return;
    }
    socketHandle = reinterpret_cast<void*>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
#else
    socketHandle = -1;
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
#ifdef _WIN32
    if (socketHandle == nullptr || socketHandle == reinterpret_cast<void*>(INVALID_SOCKET)) return false;
    
    // Set socket to non-blocking mode on Windows
    u_long mode = 1; // 1 = non-blocking, 0 = blocking
    if (ioctlsocket(reinterpret_cast<SOCKET>(socketHandle), FIONBIO, &mode) != 0) {
        return false;
    }
#else
    if (socketHandle < 0) return false;
#endif
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(address.port);
    
    if (address.ip.empty() || address.ip == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, address.ip.c_str(), &addr.sin_addr);
    }
    
#ifdef _WIN32
    int result = ::bind(reinterpret_cast<SOCKET>(socketHandle), reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
#else
    int result = ::bind(socketHandle, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
#endif
    if (result == 0) {
        bound = true;
        return true;
    }
    return false;
}

bool UDPSocket::send(const Address& to, const void* data, size_t size) {
#ifdef _WIN32
    if (!bound || socketHandle == nullptr || socketHandle == reinterpret_cast<void*>(INVALID_SOCKET)) return false;
#else
    if (!bound || socketHandle < 0) return false;
#endif
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(to.port);
    inet_pton(AF_INET, to.ip.c_str(), &addr.sin_addr);
    
#ifdef _WIN32
    int sent = sendto(reinterpret_cast<SOCKET>(socketHandle), reinterpret_cast<const char*>(data),
#else
    int sent = sendto(socketHandle, reinterpret_cast<const char*>(data),
#endif 
                      static_cast<int>(size), 0,
                      reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    return sent == static_cast<int>(size);
}

bool UDPSocket::receive(Packet& packet, int timeoutMs) {
#ifdef _WIN32
    if (!bound || socketHandle == nullptr || socketHandle == reinterpret_cast<void*>(INVALID_SOCKET)) return false;
    
    // Check for WSAEWOULDBLOCK error (non-blocking socket, no data available)
    WSASetLastError(0);
#else
    if (!bound || socketHandle < 0) return false;
#endif
    
    sockaddr_in fromAddr{};
    socklen_t fromLen = sizeof(fromAddr);
    
    packet.data.resize(MAX_PACKET_SIZE);
    
#ifdef _WIN32
    int received = recvfrom(reinterpret_cast<SOCKET>(socketHandle),
#else
    int received = recvfrom(socketHandle,
#endif 
                           reinterpret_cast<char*>(packet.data.data()),
                           MAX_PACKET_SIZE, 0,
                           reinterpret_cast<sockaddr*>(&fromAddr), &fromLen);
    
#ifdef _WIN32
    // Check for WSAEWOULDBLOCK (no data available in non-blocking mode)
    if (received == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK || error == WSAECONNRESET) {
            return false; // No data available, not an error
        }
        return false; // Other error
    }
#endif
    
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
#ifdef _WIN32
    if (socketHandle != nullptr && socketHandle != reinterpret_cast<void*>(INVALID_SOCKET)) {
        closesocket(reinterpret_cast<SOCKET>(socketHandle));
        socketHandle = nullptr;
        bound = false;
    }
#else
    if (socketHandle >= 0) {
        ::close(socketHandle);
        socketHandle = -1;
        bound = false;
    }
#endif
}

bool UDPSocket::isOpen() const {
#ifdef _WIN32
    return socketHandle != nullptr && socketHandle != reinterpret_cast<void*>(INVALID_SOCKET) && bound;
#else
    return socketHandle >= 0 && bound;
#endif
}

} // namespace game::net

