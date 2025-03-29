#pragma once

#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <deque>
#include "Chunk.hpp"
#include "WorldGenerator.hpp"

// Hash function for glm::ivec3 to use in unordered_map (for 3D chunk coordinates)
struct ChunkPosHash {
    size_t operator()(const glm::ivec3& pos) const {
        return std::hash<int>()(pos.x) ^ 
               (std::hash<int>()(pos.y) << 1) ^ 
               (std::hash<int>()(pos.z) << 2);
    }
};

class World {
public:
    static const int CHUNK_SIZE = 16;
    static const int CHUNK_HEIGHT = 16; // Changed to match Chunk's height
    
    World(uint64_t seed);
    ~World();

    // World generation and management
    void initialize();
    void generateChunk(const glm::ivec3& chunkPos);
    void loadChunk(const glm::ivec3& chunkPos);
    void unloadChunk(const glm::ivec3& chunkPos);
    void removeChunk(const glm::ivec3& chunkPos);
    
    // Block manipulation
    int getBlock(const glm::ivec3& worldPos) const;
    void setBlock(const glm::ivec3& worldPos, int blockType);
    
    // Physics interaction - check if player needs physics update due to block changes
    bool checkPlayerPhysicsUpdate(const glm::vec3& playerPosition, float playerWidth, float playerHeight);
    
    // Serialization
    bool saveToFile(const std::string& filename);
    bool loadFromFile(const std::string& filename);

    // Chunk management
    void updateChunks(const glm::vec3& playerPos);
    const std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, ChunkPosHash>& getChunks() const { return m_chunks; }
    
    // Coordinate conversion
    glm::ivec3 worldToChunkPos(const glm::vec3& worldPos) const;
    glm::ivec3 worldToLocalPos(const glm::vec3& worldPos) const;
    
    // Mesh management
    void updateChunkMeshes(const glm::ivec3& chunkPos, bool disableGreedyMeshing = false);
    
    // Greedy meshing control
    void setDisableGreedyMeshing(bool disable) { m_disableGreedyMeshing = disable; }
    bool isGreedyMeshingDisabled() const { return m_disableGreedyMeshing; }

    // Raycast functionality
    struct RaycastResult {
        glm::ivec3 blockPos;  // Position of the hit block
        glm::ivec3 faceNormal;  // Normal of the hit face
        float distance;  // Distance to the hit point
        bool hit;  // Whether the ray hit anything
    };
    RaycastResult raycast(const glm::vec3& start, const glm::vec3& direction, float maxDistance = 5.0f) const;

private:
    // Structure to track modified blocks for physics updates
    struct ModifiedBlock {
        glm::ivec3 position;    // World position of the block
        int oldType;            // Previous block type
        int newType;            // New block type
        double timeModified;    // Time when the block was modified
    };
    
    Chunk* getChunkAt(const glm::ivec3& chunkPos);

    std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, ChunkPosHash> m_chunks;
    std::unique_ptr<WorldGenerator> m_worldGenerator;
    uint64_t m_seed;
    
    // View distance in chunks
    int m_viewDistance;
    
    // Global greedy meshing setting
    bool m_disableGreedyMeshing = false;
    
    // List of recently modified blocks for physics updates
    std::deque<ModifiedBlock> m_recentlyModifiedBlocks;
}; 