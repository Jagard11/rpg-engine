// ./include/World/Chunk.hpp
#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <GL/glew.h>
#include <vector>
#include <array>
#include "World/Block.hpp"
#include "Utils/CoordinateSystem.hpp"  // Include the new coordinate system

// Forward declaration
class World;

/**
 * Represents a 16×16×16 chunk of voxels in the world.
 * Optimized for Earth-scale rendering with origin rebasing.
 */
class Chunk {
public:
    // Size of a chunk in blocks (16×16×16)
    static const int SIZE = 16;
    
    /**
     * Construct a new chunk at the given chunk coordinates.
     */
    Chunk(int x, int y, int z, int mergeFactor = 1);
    
    /**
     * Link this chunk to its parent world.
     */
    void setWorld(const World* w);
    
    /**
     * Get a block at the given local coordinates within this chunk.
     */
    Block getBlock(int x, int y, int z) const;
    
    /**
     * Set a block at the given local coordinates within this chunk.
     */
    void setBlock(int x, int y, int z, BlockType type);
    
    /**
     * Generate terrain based on the chunk's position and world configuration.
     */
    void generateTerrain();
    
    /**
     * Regenerate the chunk's mesh with the given LOD level.
     */
    void regenerateMesh(int lodLevel);
    
    /**
     * Regenerate the chunk's mesh with default LOD level.
     */
    void regenerateMesh();
    
    /**
     * Initialize OpenGL buffers for rendering.
     */
    void initializeBuffers();
    
    /**
     * Update OpenGL buffers from current mesh data.
     */
    void updateBuffers();
    
    /**
     * Bind the chunk's VAO for rendering.
     */
    void bindVAO() const { glBindVertexArray(vao); }
    
    /**
     * Get the number of indices for rendering.
     */
    size_t getIndexCount() const { return indices.size(); }
    
    /**
     * Mark the mesh as dirty, needing regeneration.
     */
    void markMeshDirty();
    
    /**
     * Get the chunk's X coordinate.
     */
    int getChunkX() const { return chunkX; }
    
    /**
     * Get the chunk's Y coordinate.
     */
    int getChunkY() const { return chunkY; }
    
    /**
     * Get the chunk's Z coordinate.
     */
    int getChunkZ() const { return chunkZ; }
    
    /**
     * Get the chunk's merge factor (for LOD).
     */
    int getMergeFactor() const { return mergeFactor; }
    
    /**
     * Check if the mesh needs regeneration.
     */
    bool isMeshDirty() const { return meshDirty; }
    
    /**
     * Check if buffers have been initialized.
     */
    bool isBuffersInitialized() const { return buffersInitialized; }
    
    /**
     * Check if buffers need to be updated.
     */
    bool isBuffersDirty() const { return buffersDirty; }
    
    /**
     * Update the chunk's position relative to the origin.
     * This is part of the origin rebasing system.
     */
    void updateRelativePosition(int originX, int originY, int originZ);
    
    /**
     * Get the chunk's world-space center position.
     */
    glm::dvec3 getWorldCenter() const;
    
    /**
     * Get the chunk's offset from the current origin.
     */
    glm::vec3 getRelativeOffset() const { return relativeOffset; }
    
    /**
     * Get the mesh data for this chunk. Used for debugging and edges rendering.
     */
    const std::vector<float>& getMesh() const;

private:
    // Chunk coordinates in chunk space
    int chunkX, chunkY, chunkZ;
    
    // Block storage (16×16×16)
    std::vector<Block> blocks;
    
    // Pointer to parent world
    const World* world;
    
    // OpenGL rendering data
    GLuint vao, vbo, ebo;
    bool buffersInitialized;
    bool buffersDirty;
    bool meshDirty;
    
    // Chunk metadata
    int mergeFactor;      // For LOD (1, 2, 4, etc.)
    int currentLodLevel;  // Current level of detail
    
    // Origin rebasing data
    glm::vec3 relativeOffset;  // Offset from the current origin
    
    // Mesh data
    std::vector<float> mesh;           // Vertex data
    std::vector<unsigned int> indices; // Index data
};

#endif // CHUNK_HPP