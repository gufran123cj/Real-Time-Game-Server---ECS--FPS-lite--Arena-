#pragma once

#include "../include/common/types.hpp"
#include "Entity.hpp"
#include "Component.hpp"
#include "World.hpp"
#include <bitset>
#include <vector>
#include <memory>
#include <tuple>

namespace game::ecs {

class System {
public:
    virtual ~System() = default;
    virtual void update(World& world, float deltaTime) = 0;
    virtual std::bitset<MAX_COMPONENTS> getRequiredComponents() const = 0;
    virtual int getPriority() const { return 0; } // Lower = earlier execution
};

// Base system with component requirements
template<typename... Components>
class SystemBase : public System {
private:
    std::bitset<MAX_COMPONENTS> requiredComponents;

public:
    SystemBase() {
        (requiredComponents.set(ComponentRegistry::getTypeID<Components>()), ...);
    }
    
    std::bitset<MAX_COMPONENTS> getRequiredComponents() const override {
        return requiredComponents;
    }
    
    virtual void process(World& world, float deltaTime, Entity& entity, Components&... components) = 0;
    
    void update(World& world, float deltaTime) override {
        auto entityIDs = world.queryEntities<Components...>();
        
        for (EntityID id : entityIDs) {
            Entity* entity = world.getEntity(id);
            if (!entity) continue;
            
            // Check if all components exist
            if ((world.getComponent<Components>(id) != nullptr && ...)) {
                // Get all required components and call process
                auto processComponents = [&](auto*... comps) {
                    process(world, deltaTime, *entity, *comps...);
                };
                processComponents(world.getComponent<Components>(id)...);
            }

        }
    }
};

} // namespace game::ecs


