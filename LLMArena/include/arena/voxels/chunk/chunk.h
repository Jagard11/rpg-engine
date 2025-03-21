// include/arena/voxels/chunk/chunk.h
#ifndef CHUNK_H
#define CHUNK_H

#include <memory>
#include <vector>
#include <QDateTime>
#include "../octree/voxel_octree.h"
#include "chunk_coordinate.h"

/**
 * @brief A chunk is a 16x16x16 section of the world, stored in an octree.
 * 
 * Chunks are the basic unit of streaming for the voxel world. They can be loaded and
 * unloaded as needed, and are stored in a memory-efficient octree structure.
 */
class Chunk {
public:
    /**
     * @brief Construct an empty chunk
     * @param coordinate The 3D coordinate of this chunk
     */
    Chunk(const ChunkCoordinate& coordinate);
    
    /**
     * @brief Get the voxel at a specific position within the chunk
     * @param x Local X coordinate (0-15)
     * @param y Local Y coordinate (0-15)
     * @param z Local Z coordinate (0-15)
     * @return The voxel at the specified position
     */
    Voxel getVoxel(int x, int y, int z) const;
    
    /**
     * @brief Set a voxel at a specific position within the chunk
     * @param x Local X coordinate (0-15)
     * @param y Local Y coordinate (0-15)
     * @param z Local Z coordinate (0-15)
     * @param voxel The voxel to set
     * @return True if the voxel was changed, false otherwise
     */
    bool setVoxel(int x, int y, int z, const Voxel& voxel);
    
    /**
     * @brief Get the coordinate of this chunk
     * @return The chunk coordinate
     */
    const ChunkCoordinate& getCoordinate() const { return m_coordinate; }
    
    /**
     * @brief Check if a voxel at position is visible (has at least one transparent neighbor)
     * @param x Local X coordinate (0-15)
     * @param y Local Y coordinate (0-15)
     * @param z Local Z coordinate (0-15)
     * @return True if the voxel is visible, false otherwise
     */
    bool isVoxelVisible(int x, int y, int z) const;
    
    /**
     * @brief Get a list of all visible voxel positions
     * This is useful for mesh generation.
     * @return Vector of visible voxel positions (in local coordinates)
     */
    std::vector<VoxelPos> getVisibleVoxels() const;
    
    /**
     * @brief Calculate memory usage of this chunk
     * @return Memory usage in bytes
     */
    size_t calculateMemoryUsage() const;
    
    /**
     * @brief Check if the chunk has been modified since last save
     * @return True if modified, false otherwise
     */
    bool isModified() const { return m_modified; }
    
    /**
     * @brief Mark the chunk as modified or unmodified
     * @param modified Modified state
     */
    void setModified(bool modified) { m_modified = modified; }
    
    /**
     * @brief Get the last time this chunk was accessed
     * @return Last access time
     */
    const QDateTime& getLastAccessTime() const { return m_lastAccessTime; }
    
    /**
     * @brief Update the last access time to now
     */
    void updateAccessTime() { m_lastAccessTime = QDateTime::currentDateTime(); }
    
    /**
     * @brief Check if the chunk is empty (contains only air)
     * @return True if empty, false otherwise
     */
    bool isEmpty() const;
    
    /**
     * @brief Get the underlying octree
     * @return Reference to the octree
     */
    const VoxelOctree& getOctree() const { return m_octree; }
    
    /**
     * @brief Optimize the chunk's storage
     * Merges octree nodes where possible to reduce memory usage
     */
    void optimize();
    
    /**
     * @brief Set a voxel at a specific position with neighbor chunk awareness
     * This is used for operations that might affect neighboring chunks
     * @param x Local X coordinate (can be outside 0-15 range)
     * @param y Local Y coordinate (can be outside 0-15 range)
     * @param z Local Z coordinate (can be outside 0-15 range)
     * @param voxel The voxel to set
     * @return Chunk coordinate and whether that chunk needs update
     */
    std::pair<ChunkCoordinate, bool> setVoxelExtended(int x, int y, int z, const Voxel& voxel);
    
private:
    // The coordinate of this chunk in the world
    ChunkCoordinate m_coordinate;
    
    // The octree that stores the voxels
    VoxelOctree m_octree;
    
    // Has this chunk been modified since last save?
    bool m_modified;
    
    // When was this chunk last accessed?
    QDateTime m_lastAccessTime;
};

#endif // CHUNK_H