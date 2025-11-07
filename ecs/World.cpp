#include "World.hpp"
#include "System.hpp"
#include <algorithm>

namespace game::ecs {

EntityID World::createEntity() {
    EntityID id = nextEntityID++;
    entities.push_back(std::make_unique<Entity>(id));
    return id;
}

void World::destroyEntity(EntityID id) {
    Entity* entity = getEntity(id);
    if (entity) {
        entity->active = false;
        // Remove all components
        for (auto& [typeID, componentMap] : components) {
            componentMap.erase(id);
        }
    }
}

Entity* World::getEntity(EntityID id) {
    for (auto& entity : entities) {
        if (entity && entity->id == id && entity->active) {
            return entity.get();
        }
    }
    return nullptr;
}

void World::addSystem(std::unique_ptr<System> system) {
    systems.push_back(std::move(system));
    // Sort by priority
    std::sort(systems.begin(), systems.end(), 
        [](const std::unique_ptr<System>& a, const std::unique_ptr<System>& b) {
            return a->getPriority() < b->getPriority();
        });
}

void World::update(float deltaTime) {
    for (auto& system : systems) {
        system->update(*this, deltaTime);
    }
}

} // namespace game::ecs

