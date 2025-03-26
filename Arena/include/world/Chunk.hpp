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

class Chunk {
public:
    static const int CHUNK_SIZE = 16;
    static const int CHUNK_HEIGHT = 256;

    Chunk(int x, int z);
    ~Chunk();

    void setBlock(int x, int y, int z, int blockType);
    int getBlock(int x, int y, int z) const;
    void generateMesh();
    const std::vector<float>& getMeshVertices() const { return m_meshVertices; }
    const std::vector<unsigned int>& getMeshIndices() const { return m_meshIndices; }
    bool isDirty() const { return m_isDirty; }
    void setDirty(bool dirty) { m_isDirty = dirty; }
    
    bool serialize(const std::string& filename) const;
    bool deserialize(const std::string& filename);

    glm::ivec2 getPosition() const { return glm::ivec2(m_x, m_z); }
    
    // Generate a collision mesh using greedy meshing algorithm
    std::vector<AABB> buildColliderMesh() const;
    
    // Check if a block type is solid (for collision purposes)
    bool isBlockSolid(int blockType) const { return blockType > 0; }

private:
    bool shouldRenderFace(int x, int y, int z, const glm::vec3& normal) const;
    void addFace(const std::vector<float>& vertices, const glm::vec3& position, const glm::vec3& normal);

    std::array<std::array<std::array<int, CHUNK_SIZE>, CHUNK_HEIGHT>, CHUNK_SIZE> m_blocks;
    std::vector<float> m_meshVertices;
    std::vector<unsigned int> m_meshIndices;
    int m_x, m_z;
    bool m_isDirty;
    
    // Caching collision mesh for performance
    mutable std::vector<AABB> m_collisionMesh;
    mutable bool m_collisionMeshDirty = true;
}; 