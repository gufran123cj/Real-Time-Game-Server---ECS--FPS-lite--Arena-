#include "Snapshot.hpp"
#include <algorithm>

namespace game::net {

void SnapshotManager::addSnapshot(Tick tick, std::unique_ptr<Snapshot> snapshot) {
    snapshots[tick] = std::move(snapshot);
    newestTick = std::max(newestTick, tick);
    
    if (oldestTick == 0 || tick < oldestTick) {
        oldestTick = tick;
    }
    
    cleanupOldSnapshots();
}

void SnapshotManager::cleanupOldSnapshots() {
    if (snapshots.size() <= static_cast<size_t>(MAX_SNAPSHOT_HISTORY)) {
        return;
    }
    
    // Remove oldest snapshots
    std::vector<Tick> ticks;
    for (const auto& [tick, snapshot] : snapshots) {
        ticks.push_back(tick);
    }
    std::sort(ticks.begin(), ticks.end());
    
    size_t toRemove = snapshots.size() - MAX_SNAPSHOT_HISTORY;
    for (size_t i = 0; i < toRemove; ++i) {
        snapshots.erase(ticks[i]);
    }
    
    if (!snapshots.empty()) {
        oldestTick = ticks[toRemove];
    } else {
        oldestTick = 0;
        newestTick = 0;
    }
}

Snapshot* SnapshotManager::getSnapshot(Tick tick) {
    auto it = snapshots.find(tick);
    if (it != snapshots.end()) {
        return it->second.get();
    }
    return nullptr;
}

DeltaSnapshot SnapshotManager::createDelta(Tick baseTick, Tick targetTick) {
    DeltaSnapshot delta(baseTick, targetTick);
    
    Snapshot* base = getSnapshot(baseTick);
    Snapshot* target = getSnapshot(targetTick);
    
    if (!base || !target) {
        return delta; // Empty delta
    }
    
    // Simple delta: just send target if different
    // TODO: Implement proper binary delta compression
    if (base->data != target->data) {
        delta.deltaData = target->data;
    }
    
    return delta;
}

} // namespace game::net

