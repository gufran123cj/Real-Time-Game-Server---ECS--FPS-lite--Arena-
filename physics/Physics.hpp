#pragma once

#include "../include/common/types.hpp"
#include <vector>
#include <memory>
#include <cmath>

namespace game::physics {

// Simple 3D vector (glm alternative if not available)
struct Vec3 {
    float x, y, z;
    
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    Vec3 operator*(float scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
    float length() const { return sqrtf(x*x + y*y + z*z); }
    float lengthSq() const { return x*x + y*y + z*z; }
    Vec3 normalized() const { float len = length(); return len > 0 ? Vec3(x/len, y/len, z/len) : Vec3(); }
};

struct AABB {
    Vec3 min;
    Vec3 max;
    
    AABB() {}
    AABB(const Vec3& min, const Vec3& max) : min(min), max(max) {}
    
    bool intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }
    
    Vec3 center() const { return Vec3((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, (min.z + max.z) * 0.5f); }
    Vec3 size() const { return Vec3(max.x - min.x, max.y - min.y, max.z - min.z); }
};

// BVH Node for spatial partitioning
class BVHNode {
public:
    AABB bounds;
    std::vector<uint32_t> entities; // Entity indices
    std::unique_ptr<BVHNode> left;
    std::unique_ptr<BVHNode> right;
    bool isLeaf;
    
    BVHNode() : isLeaf(true) {}
};

class BVH {
private:
    std::unique_ptr<BVHNode> root;
    std::vector<AABB> entityBounds;
    
    void buildRecursive(BVHNode* node, std::vector<uint32_t>& indices, int depth);
    void queryRecursive(BVHNode* node, const AABB& query, std::vector<uint32_t>& results) const;

public:
    void build(const std::vector<AABB>& bounds);
    std::vector<uint32_t> query(const AABB& bounds) const;
    void clear();
};

} // namespace game::physics

