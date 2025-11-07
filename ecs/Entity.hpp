#pragma once

#include "../include/common/types.hpp"
#include <bitset>
#include <array>

namespace game::ecs {

constexpr size_t MAX_COMPONENTS = 64;

class Entity {
public:
    EntityID id;
    std::bitset<MAX_COMPONENTS> componentMask;
    bool active = true;

    Entity(EntityID id) : id(id) {}
    
    bool hasComponent(ComponentTypeID type) const {
        return componentMask.test(type);
    }
    
    void addComponent(ComponentTypeID type) {
        componentMask.set(type);
    }
    
    void removeComponent(ComponentTypeID type) {
        componentMask.reset(type);
    }
    
    bool matches(const std::bitset<MAX_COMPONENTS>& mask) const {
        return (componentMask & mask) == mask;
    }
};

} // namespace game::ecs

