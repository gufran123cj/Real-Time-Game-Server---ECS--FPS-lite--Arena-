#pragma once

#include "../ecs/System.hpp"
#include "../components/Position.hpp"
#include "../components/Velocity.hpp"
#include "../components/CollisionComponent.hpp"
#include "../physics/Physics.hpp"
#include "../include/common/types.hpp"
#include <vector>
#include <unordered_map>

namespace game::systems {

// Physics System - Handles collision detection and response
class PhysicsSystem : public ecs::SystemBase<components::Position, components::CollisionComponent> {
private:
    physics::BVH bvh;
    std::vector<physics::AABB> entityBounds;
    std::unordered_map<EntityID, size_t> entityToIndex;
    
    // World boundaries (map limits)
    physics::AABB worldBounds;
    bool useWorldBounds;
    
    // Collision response settings
    const float COLLISION_EPSILON = 0.01f; // Small offset to prevent floating point errors
    
public:
    PhysicsSystem() : useWorldBounds(true) {
        // Default world bounds: -75 to +75 on X and Y axes (150x150 map, 2D top-down)
        worldBounds = physics::AABB(
            physics::Vec3(-75.0f, -75.0f, -50.0f),
            physics::Vec3(75.0f, 75.0f, 50.0f)
        );
    }
    
    PhysicsSystem(const physics::AABB& bounds) : worldBounds(bounds), useWorldBounds(true) {}
    
    int getPriority() const override {
        return 20; // Run after MovementSystem (priority 10) but before other systems
    }
    
    void process(ecs::World& world, float deltaTime, ecs::Entity& entity,
                 components::Position& position,
                 components::CollisionComponent& collision) override {
        
        // Update collision bounds based on position
        physics::Vec3 center = position.value;
        physics::Vec3 size = collision.bounds.size();
        collision.bounds.min = center - (size * 0.5f);
        collision.bounds.max = center + (size * 0.5f);
        
        // Check world boundaries
        if (useWorldBounds) {
            resolveWorldBoundaryCollision(position, collision);
        }
    }
    
    // Override update to handle BVH and collision resolution
    void update(ecs::World& world, float deltaTime) override {
        // First, update all collision bounds (via process)
        SystemBase<components::Position, components::CollisionComponent>::update(world, deltaTime);
        
        // Update BVH with current bounds
        updateCollisions(world);
        
        // Resolve collisions for entities with velocity
        auto movingEntities = world.queryEntities<components::Position, components::Velocity, components::CollisionComponent>();
        for (EntityID entityID : movingEntities) {
            auto* pos = world.getComponent<components::Position>(entityID);
            auto* vel = world.getComponent<components::Velocity>(entityID);
            auto* coll = world.getComponent<components::CollisionComponent>(entityID);
            
            if (pos && vel && coll && !coll->isStatic) {
                resolveCollisions(world, entityID, *pos, *vel, *coll);
            }
        }
    }
    
    // Update BVH and check collisions for all entities with Position + CollisionComponent
    void updateCollisions(ecs::World& world) {
        // Collect all collision bounds
        entityBounds.clear();
        entityToIndex.clear();
        
        auto entityIDs = world.queryEntities<components::Position, components::CollisionComponent>();
        for (EntityID entityID : entityIDs) {
            auto* pos = world.getComponent<components::Position>(entityID);
            auto* coll = world.getComponent<components::CollisionComponent>(entityID);
            
            if (pos && coll) {
                // Update bounds based on position
                physics::Vec3 center = pos->value;
                physics::Vec3 size = coll->bounds.size();
                physics::AABB bounds(
                    center - (size * 0.5f),
                    center + (size * 0.5f)
                );
                
                entityToIndex[entityID] = entityBounds.size();
                entityBounds.push_back(bounds);
            }
        }
        
        // Rebuild BVH
        if (!entityBounds.empty()) {
            bvh.build(entityBounds);
        }
    }
    
    // Check and resolve collisions for a moving entity
    bool resolveCollisions(ecs::World& world, EntityID entityID, 
                          components::Position& position, 
                          components::Velocity& velocity,
                          components::CollisionComponent& collision) {
        
        // Skip static objects
        if (collision.isStatic) {
            return false;
        }
        
        // Calculate new position
        physics::Vec3 newPosition = position.value + (velocity.value * (1.0f / 60.0f)); // Fixed timestep
        
        // Create AABB for new position
        physics::Vec3 size = collision.bounds.size();
        physics::AABB newBounds(
            newPosition - (size * 0.5f),
            newPosition + (size * 0.5f)
        );
        
        // Query BVH for potential collisions
        auto potentialCollisions = bvh.query(newBounds);
        
        bool hasCollision = false;
        physics::Vec3 correction(0.0f, 0.0f, 0.0f);
        
        for (uint32_t idx : potentialCollisions) {
            // Find entity ID from index
            EntityID otherEntityID = INVALID_ENTITY;
            for (const auto& [eid, index] : entityToIndex) {
                if (index == idx && eid != entityID) {
                    otherEntityID = eid;
                    break;
                }
            }
            
            if (otherEntityID == INVALID_ENTITY) continue;
            
            // Get other entity's collision component
            auto* otherColl = world.getComponent<components::CollisionComponent>(otherEntityID);
            if (!otherColl) continue;
            
            // Skip triggers (they don't block movement)
            if (otherColl->isTrigger) continue;
            
            // Check actual collision
            if (newBounds.intersects(otherColl->bounds)) {
                hasCollision = true;
                
                // Simple collision response: push away from collision
                physics::Vec3 direction = newPosition - otherColl->bounds.center();
                float distance = direction.length();
                
                if (distance > 0.0f) {
                    direction = direction.normalized();
                    float overlap = (size.length() + otherColl->bounds.size().length()) * 0.5f - distance;
                    if (overlap > 0.0f) {
                        correction = correction + (direction * (overlap + COLLISION_EPSILON));
                    }
                } else {
                    // Entities are exactly on top of each other - push in a random direction
                    correction = correction + physics::Vec3(0.1f, 0.0f, 0.0f);
                }
            }
        }
        
        // Apply correction
        if (hasCollision) {
            newPosition = newPosition + correction;
            
            // Update position and stop velocity in collision direction
            position.value = newPosition;
            
            // Dampen velocity (simple friction)
            velocity.value = velocity.value * 0.5f;
            
            return true;
        }
        
        return false;
    }
    
private:
    void resolveWorldBoundaryCollision(components::Position& position, 
                                      components::CollisionComponent& collision) {
        physics::Vec3 size = collision.bounds.size();
        physics::Vec3 halfSize = size * 0.5f;
        
        // Clamp position to world bounds
        if (position.value.x - halfSize.x < worldBounds.min.x) {
            position.value.x = worldBounds.min.x + halfSize.x;
        }
        if (position.value.x + halfSize.x > worldBounds.max.x) {
            position.value.x = worldBounds.max.x - halfSize.x;
        }
        
        if (position.value.y - halfSize.y < worldBounds.min.y) {
            position.value.y = worldBounds.min.y + halfSize.y;
        }
        if (position.value.y + halfSize.y > worldBounds.max.y) {
            position.value.y = worldBounds.max.y - halfSize.y;
        }
        
        if (position.value.z - halfSize.z < worldBounds.min.z) {
            position.value.z = worldBounds.min.z + halfSize.z;
        }
        if (position.value.z + halfSize.z > worldBounds.max.z) {
            position.value.z = worldBounds.max.z - halfSize.z;
        }
        
        // Update collision bounds
        collision.bounds.min = position.value - halfSize;
        collision.bounds.max = position.value + halfSize;
    }
};

} // namespace game::systems

