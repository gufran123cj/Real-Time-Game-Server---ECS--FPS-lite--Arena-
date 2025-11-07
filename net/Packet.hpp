#pragma once

#include "../include/common/types.hpp"
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstddef>

namespace game::net {

enum class PacketType : uint8_t {
    CONNECT = 0,
    DISCONNECT,
    HEARTBEAT,
    INPUT,
    SNAPSHOT,
    DELTA,
    ACK,
    RPC
};

struct PacketHeader {
    PacketType type;
    SequenceNumber sequence;
    Tick serverTick;
    PlayerID playerID;
    
    PacketHeader() : type(PacketType::HEARTBEAT), sequence(0), serverTick(0), playerID(INVALID_PLAYER) {}
};

class PacketReader {
private:
    const uint8_t* data;
    size_t size;
    size_t offset;

public:
    PacketReader(const uint8_t* data, size_t size) : data(data), size(size), offset(0) {}
    
    template<typename T>
    bool read(T& value) {
        if (offset + sizeof(T) > size) return false;
        std::memcpy(&value, data + offset, sizeof(T));
        offset += sizeof(T);
        return true;
    }
    
    bool readBytes(void* dest, size_t count) {
        if (offset + count > size) return false;
        std::memcpy(dest, data + offset, count);
        offset += count;
        return true;
    }
    
    size_t remaining() const { return size - offset; }
    size_t getOffset() const { return offset; }
};

class PacketWriter {
private:
    std::vector<uint8_t> buffer;
    size_t writeOffset;  // 'offset' MinGW makrosu ile çakışıyor

public:
    PacketWriter() : writeOffset(0) {
        buffer.reserve(MAX_PACKET_SIZE);
    }
    
    template<typename T>
    bool write(const T& value) {
        if (writeOffset + sizeof(T) > MAX_PACKET_SIZE) return false;
        size_t oldSize = buffer.size();
        if (writeOffset + sizeof(T) > oldSize) {
            buffer.resize(writeOffset + sizeof(T));
        }
        std::memcpy(buffer.data() + writeOffset, &value, sizeof(T));
        writeOffset += sizeof(T);
        return true;
    }
    
    bool writeBytes(const void* src, size_t count) {
        if (writeOffset + count > MAX_PACKET_SIZE) return false;
        size_t oldSize = buffer.size();
        if (writeOffset + count > oldSize) {
            buffer.resize(writeOffset + count);
        }
        std::memcpy(buffer.data() + writeOffset, src, count);
        writeOffset += count;
        return true;
    }
    
    const uint8_t* getData() const { return buffer.data(); }
    size_t getSize() const { return writeOffset; }
    void reset() { writeOffset = 0; buffer.clear(); }
};

} // namespace game::net

