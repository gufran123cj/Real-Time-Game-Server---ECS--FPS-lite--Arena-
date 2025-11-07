#pragma once

#include "../include/common/types.hpp"
#include "Packet.hpp"
#include <vector>
#include <unordered_map>
#include <memory>

namespace game::net {

struct Snapshot {
    Tick tick;
    std::vector<uint8_t> data;
    size_t entityCount;
    
    Snapshot(Tick tick) : tick(tick), entityCount(0) {}
};

struct DeltaSnapshot {
    Tick baseTick;
    Tick targetTick;
    std::vector<uint8_t> deltaData;
    
    DeltaSnapshot(Tick base, Tick target) : baseTick(base), targetTick(target) {}
};

class SnapshotManager {
private:
    std::unordered_map<Tick, std::unique_ptr<Snapshot>> snapshots;
    Tick oldestTick;
    Tick newestTick;
    const int MAX_SNAPSHOT_HISTORY = 64;
    
    void cleanupOldSnapshots();

public:
    SnapshotManager() : oldestTick(0), newestTick(0) {}
    
    void addSnapshot(Tick tick, std::unique_ptr<Snapshot> snapshot);
    Snapshot* getSnapshot(Tick tick);
    DeltaSnapshot createDelta(Tick baseTick, Tick targetTick);
    
    Tick getNewestTick() const { return newestTick; }
    Tick getOldestTick() const { return oldestTick; }
};

} // namespace game::net

