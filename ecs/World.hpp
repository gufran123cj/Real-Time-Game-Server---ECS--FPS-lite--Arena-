#pragma once

#include "Entity.hpp"
#include "Component.hpp"
#include "System.hpp"
#include "../include/common/types.hpp"
#include <vector>
#include <unordered_map>
#include <memory>
#include <bitset>

namespace game::ecs {

class World {
private:
    std::vector<std::unique_ptr<Entity>> entities;
    std::unordered_map<ComponentTypeID, std::unordered_map<EntityID, std::unique_ptr<Component>>> components;
    std::vector<std::unique_ptr<System>> systems;
    EntityID nextEntityID = 0;

public:
    EntityID createEntity();
    void destroyEntity(EntityID id);
    Entity* getEntity(EntityID id);
    
    template<typename T>
    T* addComponent(EntityID entityID, std::unique_ptr<T> component) {
        static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
        ComponentTypeID typeID = ComponentRegistry::getTypeID<T>();
        
        Entity* entity = getEntity(entityID);
        if (!entity) return nullptr;
        
        entity->addComponent(typeID);
        components[typeID][entityID] = std::move(component);
        return static_cast<T*>(components[typeID][entityID].get());
    }
    
    template<typename T>
    T* getComponent(EntityID entityID) {
        static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
        ComponentTypeID typeID = ComponentRegistry::getTypeID<T>();
        
        auto& componentMap = components[typeID];
        auto it = componentMap.find(entityID);
        if (it == componentMap.end()) return nullptr;
        
        return static_cast<T*>(it->second.get());
    }
    
    template<typename T>
    void removeComponent(EntityID entityID) {
        static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
        ComponentTypeID typeID = ComponentRegistry::getTypeID<T>();
        
        Entity* entity = getEntity(entityID);
        if (entity) {
            entity->removeComponent(typeID);
        }
        components[typeID].erase(entityID);
    }
    
    void addSystem(std::unique_ptr<System> system);
    void update(float deltaTime);
    
    // Query entities with specific components
    template<typename... Components>
    std::vector<EntityID> queryEntities() {
        std::bitset<MAX_COMPONENTS> mask;
        (mask.set(ComponentRegistry::getTypeID<Components>()), ...);
        
        std::vector<EntityID> result;
        for (auto& entity : entities) {
            if (entity && entity->active && entity->matches(mask)) {
                result.push_back(entity->id);
            }
        }
        return result;
    }
};

} // namespace game::ecs

