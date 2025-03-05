// ./include/World/Chunk.hpp
#ifndef CHUNK_HPP
#define CHUNK_HPP

// Always include GLEW first, before any other OpenGL headers
#include <GL/glew.h>
#include <vector>
#include "World/Block.hpp"
#include "Graphics/MeshGenerator.hpp" // Added

// Forward declaration to avoid circular dependency
class World;

class Chunk {
public:
    // Size of a chunk in blocks (16x16x16)
    static const int SIZE = 16;
    
    // Constructor
    Chunk(int x, int y, int z, int mergeFactor = 1);
    
    // Set pointer to parent world
    void setWorld(const World* w);
    
    // Get a block at local coordinates
    Block getBlock(int x, int y, int z) const;
    
    // Set a block at local coordinates
    void setBlock(int x, int y, int z, BlockType type);
    
    // Generate terrain for this chunk
    void generateTerrain();
    
    // Regenerate mesh with specified LOD level
    void regenerateMesh(int lodLevel);
    
    // Regenerate mesh with default LOD
    void regenerateMesh();
    
    // Get the mesh vertex data
    const std::vector<float>& getMesh() const;
    
    // Initialize OpenGL buffers
    void initializeBuffers();
    
    // Update OpenGL buffers from mesh data
    void updateBuffers();
    
    // Bind VAO for rendering
    void bindVAO() const { glBindVertexArray(vao); }
    
    // Get number of indices for rendering
    size_t getIndexCount() const { return indices.size(); }
    
    // Mark mesh as needing regeneration
    void markMeshDirty();

    // Chunk coordinates in chunk space
    int chunkX, chunkY, chunkZ;
    
    // Get merge factor (for LOD purposes)
    int getMergeFactor() const { return mergeFactor; }
    
    // Check if mesh needs regeneration
    bool isMeshDirty() const { return meshDirty; }
    
    // Check if buffers have been initialized
    bool isBuffersInitialized() const { return buffersInitialized; }
    
    // Check if buffers need to be updated
    bool isBuffersDirty() const { return buffersDirty; }

private:
    // Block data storage (only populated for mergeFactor == 1)
    std::vector<Block> blocks;
    
    // Mesh vertex data (position, uv coordinates)
    std::vector<float> mesh;
    
    // Indices for indexed rendering
    std::vector<GLuint> indices;
    
    // Pointer to parent world
    const World* world;
    
    // OpenGL buffer objects
    GLuint vao, vbo, ebo;
    
    // State flags
    bool buffersDirty;           // Indicates if GPU buffers need updating
    bool meshDirty;              // Indicates if the mesh needs regeneration due to block changes
    bool buffersInitialized;     // Prevents reinitialization of OpenGL buffers
    
    // LOD tracking
    int currentLodLevel;         // Tracks the LOD level of the current mesh
    int mergeFactor;             // Size multiplier (1, 2, 4, 8, 16)
};

#endif // CHUNK_HPP