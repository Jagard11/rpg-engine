// include/arena/voxels/octree/voxel_octree.h
#ifndef VOXEL_OCTREE_H
#define VOXEL_OCTREE_H

#include <memory>
#include <array>
#include "../../voxels/types/voxel_types.h"

// Maximum octree depth (0 = just root node, 8 = maximum subdivision)
// With 16x16x16 chunks, a depth of 4 gives us individual voxel access
#define MAX_OCTREE_DEPTH 4

// Forward declarations
class VoxelOctreeNode;
class VoxelRaycastResult;

/**
 * @brief An octree-based data structure for efficiently storing and accessing voxels.
 * 
 * This octree implementation subdivides space recursively, allocating memory only
 * where voxels actually exist. This is much more memory efficient than a flat array
 * for sparse data, which is common in voxel worlds.
 */
class VoxelOctree {
public:
    VoxelOctree();
    ~VoxelOctree();
    
    /**
     * @brief Get the voxel at a specific position
     * @param x X coordinate (0-15 within chunk)
     * @param y Y coordinate (0-15 within chunk)
     * @param z Z coordinate (0-15 within chunk)
     * @return The voxel at the specified position, or air if empty
     */
    Voxel getVoxel(int x, int y, int z) const;
    
    /**
     * @brief Set a voxel at a specific position
     * @param x X coordinate (0-15 within chunk)
     * @param y Y coordinate (0-15 within chunk)
     * @param z Z coordinate (0-15 within chunk)
     * @param voxel The voxel to set
     * @return True if the voxel was changed, false otherwise
     */
    bool setVoxel(int x, int y, int z, const Voxel& voxel);
    
    /**
     * @brief Check if a voxel at position is visible (has at least one transparent neighbor)
     * @param x X coordinate (0-15 within chunk)
     * @param y Y coordinate (0-15 within chunk)
     * @param z Z coordinate (0-15 within chunk)
     * @return True if the voxel is visible, false otherwise
     */
    bool isVoxelVisible(int x, int y, int z) const;
    
    /**
     * @brief Get a list of all visible voxel positions
     * This is useful for mesh generation.
     * @return Vector of visible voxel positions
     */
    std::vector<VoxelPos> getVisibleVoxels() const;
    
    /**
     * @brief Calculate memory usage of this octree
     * @return Memory usage in bytes
     */
    size_t calculateMemoryUsage() const;
    
    /**
     * @brief Clear all voxels in the octree
     */
    void clear();
    
    /**
     * @brief Raycast through the octree
     * @param origin Ray origin
     * @param direction Ray direction (normalized)
     * @param maxDistance Maximum ray distance
     * @return Result of the raycast
     */
    VoxelRaycastResult raycast(const QVector3D& origin, const QVector3D& direction, float maxDistance = 100.0f) const;
    
    /**
     * @brief Check if the octree is empty (contains only air)
     * @return True if empty, false otherwise
     */
    bool isEmpty() const;
    
    /**
     * @brief Optimize the octree structure
     * Merges nodes where possible to reduce memory usage
     */
    void optimize();
    
private:
    // Root node of the octree
    std::unique_ptr<VoxelOctreeNode> m_rootNode;
    
    // Utility functions for coordinate conversion
    int posToIndex(int x, int y, int z, int level) const;
    void indexToPos(int index, int level, int& x, int& y, int& z) const;
    
    // Helper for visible voxel collection
    void collectVisibleVoxels(std::vector<VoxelPos>& visibleVoxels) const;
};

/**
 * @brief An individual node in the voxel octree
 * 
 * Each node either contains 8 child nodes or voxel data if it's a leaf node.
 */
class VoxelOctreeNode {
public:
    VoxelOctreeNode();
    ~VoxelOctreeNode();
    
    // Is this node a leaf (contains voxel data) or branch (contains child nodes)?
    bool isLeaf() const { return m_isLeaf; }
    
    // If this is a leaf, get/set the voxel data
    Voxel getVoxel() const;
    void setVoxel(const Voxel& voxel);
    
    // If this is a branch, get a child node (0-7)
    VoxelOctreeNode* getChild(int index) const;
    
    // Split this node from a leaf into a branch with 8 children
    void split();
    
    // Attempt to merge children if they all contain the same voxel
    bool tryMerge();
    
    // Calculate memory usage of this node and its children
    size_t calculateMemoryUsage() const;
    
    // Check if all voxels in this node and its children are air
    bool isEmpty() const;
    
private:
    // Is this a leaf node?
    bool m_isLeaf;
    
    // Voxel data (only valid if isLeaf is true)
    Voxel m_voxel;
    
    // Child nodes (only valid if isLeaf is false)
    std::array<std::unique_ptr<VoxelOctreeNode>, 8> m_children;
};

/**
 * @brief Result of a raycast through the octree
 */
class VoxelRaycastResult {
public:
    // Did the ray hit anything?
    bool hit;
    
    // Position of the hit voxel
    VoxelPos position;
    
    // The face that was hit (0-5 for +x, -x, +y, -y, +z, -z)
    int face;
    
    // The exact hit point
    QVector3D hitPoint;
    
    // Distance from ray origin to hit point
    float distance;
    
    // The voxel that was hit
    Voxel voxel;
    
    // Normal vector of the hit face
    QVector3D normal;
    
    // Default constructor (no hit)
    VoxelRaycastResult() : hit(false), face(-1), distance(0.0f) {}
};

#endif // VOXEL_OCTREE_H
