#pragma once

#include <glm/glm.hpp>
#include "Chunk.hpp"

// Forward declarations
class World;
class Player;

class ChunkVisibilityManager {
public:
    ChunkVisibilityManager(World* world);
    ~ChunkVisibilityManager();

    // Update visible chunks based on player position
    void updateChunkVisibility(const Player& player);

private:
    // Check if any voxel in the chunk is adjacent to an open (air) voxel
    bool checkForOpenVoxel(const glm::ivec3& chunkPos);
    
    // Check if a specific voxel is adjacent to an open (air) voxel
    bool isAdjacentToOpenVoxel(const glm::ivec3& chunkPos, int x, int y, int z);
    
    // Get the voxel at the specified position (handles chunk boundaries)
    int getVoxel(const glm::ivec3& chunkPos, int x, int y, int z);

    // Mark a chunk for rendering in the visibility system
    void markChunkForRendering(const glm::ivec3& chunkPos);
    
    // Load a chunk into memory
    void loadChunkIntoMemory(const glm::ivec3& chunkPos);
    
    // Check if a chunk should be skipped for rendering (fully covered or empty)
    bool shouldSkipChunk(const glm::ivec3& chunkPos);

    // The world this manager works with
    World* m_world;
}; 