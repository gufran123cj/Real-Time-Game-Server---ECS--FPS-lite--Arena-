#include "Physics.hpp"
#include <algorithm>
#include <cmath>
#include <cfloat>

namespace game::physics {

void BVH::build(const std::vector<AABB>& bounds) {
    entityBounds = bounds;
    root = std::make_unique<BVHNode>();
    
    std::vector<uint32_t> indices;
    for (size_t i = 0; i < bounds.size(); ++i) {
        indices.push_back(static_cast<uint32_t>(i));
    }
    
    buildRecursive(root.get(), indices, 0);
}

void BVH::buildRecursive(BVHNode* node, std::vector<uint32_t>& indices, int depth) {
    if (indices.empty()) {
        node->isLeaf = true;
        return;
    }
    
    if (indices.size() <= 4 || depth > 16) { // Leaf node
        node->isLeaf = true;
        node->entities = indices;
        
        // Calculate bounds
        if (!indices.empty()) {
            node->bounds = entityBounds[indices[0]];
            for (size_t i = 1; i < indices.size(); ++i) {
                const AABB& b = entityBounds[indices[i]];
                node->bounds.min.x = std::min(node->bounds.min.x, b.min.x);
                node->bounds.min.y = std::min(node->bounds.min.y, b.min.y);
                node->bounds.min.z = std::min(node->bounds.min.z, b.min.z);
                node->bounds.max.x = std::max(node->bounds.max.x, b.max.x);
                node->bounds.max.y = std::max(node->bounds.max.y, b.max.y);
                node->bounds.max.z = std::max(node->bounds.max.z, b.max.z);
            }
        }
        return;
    }
    
    // Calculate centroid bounds
    Vec3 centroidMin(FLT_MAX, FLT_MAX, FLT_MAX);
    Vec3 centroidMax(FLT_MIN, FLT_MIN, FLT_MIN);
    
    for (uint32_t idx : indices) {
        Vec3 center = entityBounds[idx].center();
        centroidMin.x = std::min(centroidMin.x, center.x);
        centroidMin.y = std::min(centroidMin.y, center.y);
        centroidMin.z = std::min(centroidMin.z, center.z);
        centroidMax.x = std::max(centroidMax.x, center.x);
        centroidMax.y = std::max(centroidMax.y, center.y);
        centroidMax.z = std::max(centroidMax.z, center.z);
    }
    
    // Split on longest axis
    Vec3 size = centroidMax - centroidMin;
    int axis = 0;
    float maxSize = size.x;
    if (size.y > maxSize) { axis = 1; maxSize = size.y; }
    if (size.z > maxSize) { axis = 2; }
    
    float splitPos = 0.0f;
    if (axis == 0) splitPos = (centroidMin.x + centroidMax.x) * 0.5f;
    else if (axis == 1) splitPos = (centroidMin.y + centroidMax.y) * 0.5f;
    else splitPos = (centroidMin.z + centroidMax.z) * 0.5f;
    
    // Partition
    std::vector<uint32_t> leftIndices, rightIndices;
    for (uint32_t idx : indices) {
        Vec3 center = entityBounds[idx].center();
        float centerAxis = (axis == 0) ? center.x : ((axis == 1) ? center.y : center.z);
        if (centerAxis < splitPos) {
            leftIndices.push_back(idx);
        } else {
            rightIndices.push_back(idx);
        }
    }
    
    // Ensure both sides have elements
    if (leftIndices.empty()) {
        leftIndices.push_back(rightIndices.back());
        rightIndices.pop_back();
    }
    if (rightIndices.empty()) {
        rightIndices.push_back(leftIndices.back());
        leftIndices.pop_back();
    }
    
    node->isLeaf = false;
    node->left = std::make_unique<BVHNode>();
    node->right = std::make_unique<BVHNode>();
    
    buildRecursive(node->left.get(), leftIndices, depth + 1);
    buildRecursive(node->right.get(), rightIndices, depth + 1);
    
    // Calculate parent bounds
    node->bounds = node->left->bounds;
    if (!node->right->entities.empty() || !node->right->isLeaf) {
        const AABB& rightBounds = node->right->bounds;
        node->bounds.min.x = std::min(node->bounds.min.x, rightBounds.min.x);
        node->bounds.min.y = std::min(node->bounds.min.y, rightBounds.min.y);
        node->bounds.min.z = std::min(node->bounds.min.z, rightBounds.min.z);
        node->bounds.max.x = std::max(node->bounds.max.x, rightBounds.max.x);
        node->bounds.max.y = std::max(node->bounds.max.y, rightBounds.max.y);
        node->bounds.max.z = std::max(node->bounds.max.z, rightBounds.max.z);
    }
}

std::vector<uint32_t> BVH::query(const AABB& bounds) const {
    std::vector<uint32_t> results;
    if (root) {
        queryRecursive(root.get(), bounds, results);
    }
    return results;
}

void BVH::queryRecursive(BVHNode* node, const AABB& query, std::vector<uint32_t>& results) const {
    if (!node->bounds.intersects(query)) {
        return;
    }
    
    if (node->isLeaf) {
        for (uint32_t idx : node->entities) {
            if (entityBounds[idx].intersects(query)) {
                results.push_back(idx);
            }
        }
    } else {
        if (node->left) queryRecursive(node->left.get(), query, results);
        if (node->right) queryRecursive(node->right.get(), query, results);
    }
}

void BVH::clear() {
    root.reset();
    entityBounds.clear();
}

} // namespace game::physics

