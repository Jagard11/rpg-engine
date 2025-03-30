#pragma once

#include <vector>
#include <array>
#include <memory>
#include <string>
#include <glm/glm.hpp>

// A simple struct to represent an axis-aligned bounding box for collision
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
    
    AABB() : min(0.0f), max(0.0f) {}
    AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}
};

class World; // Forward declaration

class Chunk {
public:
    static const int CHUNK_SIZE = 16;
    static const int CHUNK_HEIGHT = 16; // Changed from 256 to 16 for cubic chunks

    // Updated constructor to take y coordinate
    Chunk(int x, int y, int z);
    ~Chunk();

    void setBlock(int x, int y, int z, int blockType);
    int getBlock(int x, int y, int z) const;
    
    // Add parameter to allow disabling greedy meshing
    void generateMesh(bool disableGreedyMeshing = false);
    
    const std::vector<float>& getMeshVertices() const { return m_meshVertices; }
    const std::vector<unsigned int>& getMeshIndices() const { return m_meshIndices; }
    bool isDirty() const { return m_isDirty; }
    void setDirty(bool dirty) { m_isDirty = dirty; }
    
    // Track if the chunk has been modified by the player
    bool isModified() const { return m_isModified; }
    void setModified(bool modified) { m_isModified = modified; }
    
    // Check if chunk is empty (contains only air blocks)
    bool isEmpty() const;
    
    bool serialize(const std::string& filename) const;
    bool deserialize(const std::string& filename);

    // Update to return 3D position
    glm::ivec3 getPosition() const { return glm::ivec3(m_x, m_y, m_z); }
    
    // Generate a collision mesh using greedy meshing algorithm
    std::vector<AABB> buildColliderMesh() const;
    
    // Check if a block type is solid (for collision purposes)
    bool isBlockSolid(int blockType) const { return blockType > 0; }
    
    // Check if a block type is transparent (for face culling)
    bool isBlockTransparent(int blockType) const;
    
    // Set the world pointer for cross-chunk queries
    void setWorld(World* world) { m_world = world; }

private:
    int getAdjacentBlock(int x, int y, int z, const glm::vec3& normal) const;
    void addFace(const std::vector<float>& vertices, const glm::vec3& position, const glm::vec3& normal);
    // Helper method for greedy meshing
    void addGreedyFace(const std::vector<float>& faceTemplate, const glm::ivec3& normal, 
                       int uStart, int vStart, int wPos, int width, int height, 
                       int uAxis, int vAxis, int wAxis);

    // Helper function to calculate face normal from vertices
    glm::vec3 calculateFaceNormal(const std::vector<float>& vertices, int startIndex);

    std::array<std::array<std::array<int, CHUNK_SIZE>, CHUNK_HEIGHT>, CHUNK_SIZE> m_blocks;
    std::vector<float> m_meshVertices;
    std::vector<unsigned int> m_meshIndices;
    int m_x, m_y, m_z; // Updated to store 3D position
    bool m_isDirty;
    bool m_isModified; // Track whether player has modified this chunk
    
    // Caching collision mesh for performance
    mutable std::vector<AABB> m_collisionMesh;
    mutable bool m_collisionMeshDirty = true;
    
    World* m_world;
}; 