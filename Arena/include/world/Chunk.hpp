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

// Exposure mask structure - 6 bits for 6 faces of a chunk
struct ExposureMask {
    bool posX : 1;  // Positive X face has any voxels missing
    bool negX : 1;  // Negative X face has any voxels missing
    bool posY : 1;  // Positive Y face has any voxels missing
    bool negY : 1;  // Negative Y face has any voxels missing
    bool posZ : 1;  // Positive Z face has any voxels missing
    bool negZ : 1;  // Negative Z face has any voxels missing
    
    // Default constructor - initialize all faces as unexposed
    ExposureMask() : posX(false), negX(false), posY(false), negY(false), posZ(false), negZ(false) {}
    
    // Constructor with explicit values
    ExposureMask(bool px, bool nx, bool py, bool ny, bool pz, bool nz) 
        : posX(px), negX(nx), posY(py), negY(ny), posZ(pz), negZ(nz) {}
    
    // Check if any face is exposed
    bool isExposed() const {
        return posX || negX || posY || negY || posZ || negZ;
    }
    
    // Get total number of exposed faces
    int countExposedFaces() const {
        return (posX ? 1 : 0) + (negX ? 1 : 0) + (posY ? 1 : 0) + 
               (negY ? 1 : 0) + (posZ ? 1 : 0) + (negZ ? 1 : 0);
    }
    
    // Set all faces to a single value
    void setAll(bool value) {
        posX = negX = posY = negY = posZ = negZ = value;
    }
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
    
    // Track if this chunk has visible faces and should be rendered
    bool hasVisibleFaces() const { return m_hasVisibleFaces; }
    void setHasVisibleFaces(bool hasVisibleFaces) { m_hasVisibleFaces = hasVisibleFaces; }
    
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
    
    // Get the current exposure mask
    const ExposureMask& getExposureMask() const { return m_exposureMask; }
    
    // Check if this chunk is exposed at all
    bool isExposed() const { return m_exposureMask.isExposed(); }
    
    // Check if this chunk has a mesh already
    bool hasMesh() const { return !m_meshVertices.empty(); }
    
    // Calculate exposure mask based on current blocks
    void calculateExposureMask();
    
    // Calculate exposure for a specific face (useful for updating after changes)
    void calculateFaceExposure(int faceIndex);
    
    // Check if a specific face (0-5) has any holes
    bool isFaceExposed(int faceIndex) const;
    
    // Utility method to get exposure status of face between this chunk and an adjacent one
    bool isFaceExposedToChunk(const glm::ivec3& adjacentChunkPos) const;

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
    bool m_hasVisibleFaces = true; // Track if this chunk has any visible faces and should be rendered
    
    // Caching collision mesh for performance
    mutable std::vector<AABB> m_collisionMesh;
    mutable bool m_collisionMeshDirty = true;
    
    // Exposure mask for the 6 faces of the chunk
    ExposureMask m_exposureMask;
    
    World* m_world;
}; 