#pragma once

#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <deque>
#include "Chunk.hpp"
#include "WorldGenerator.hpp"
#include <string>

// Custom hash for glm::ivec3
namespace std {
    template<>
    struct hash<glm::ivec3> {
        size_t operator()(const glm::ivec3& vec) const {
            // Combine the hash of the components
            size_t h1 = hash<int>()(vec.x);
            size_t h2 = hash<int>()(vec.y);
            size_t h3 = hash<int>()(vec.z);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}

class World {
public:
    static const int CHUNK_SIZE = 16;
    static const int CHUNK_HEIGHT = 16; // Changed to match Chunk's height
    
    // Modified block tracking
    struct ModifiedBlock {
        glm::ivec3 position;
        int oldType;
        int newType;
        double timeModified;
    };
    
    // Raycast result struct
    struct RaycastResult {
        bool hit;
        glm::ivec3 blockPos;
        glm::vec3 hitPoint;
        glm::vec3 faceNormal;
        float distance;
    };

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
    bool serialize(const std::string& filename) const;
    bool deserialize(const std::string& filename);

    // Chunk management
    void updateChunks(const glm::vec3& playerPos);
    const std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>>& getChunks() const { return m_chunks; }
    
    // Coordinate conversion
    glm::ivec3 worldToChunkPos(const glm::vec3& worldPos) const;
    glm::ivec3 worldToLocalPos(const glm::vec3& worldPos) const;
    
    // Mesh management
    void updateChunkMeshes(const glm::ivec3& chunkPos, bool disableGreedyMeshing = false);
    
    // Greedy meshing control
    void setGreedyMeshingEnabled(bool enabled) { m_disableGreedyMeshing = !enabled; }
    bool isGreedyMeshingEnabled() const { return !m_disableGreedyMeshing; }

    // View distance
    void setViewDistance(int distance) { m_viewDistance = distance; }
    int getViewDistance() const { return m_viewDistance; }

    // Raycast functionality
    RaycastResult raycast(const glm::vec3& start, const glm::vec3& direction, float maxDistance) const;

    // Get seed
    uint64_t getSeed() const { return m_seed; }

    // Returns the approximate count of pending chunk operations
    int getPendingChunksCount() const;

private:
    Chunk* getChunkAt(const glm::ivec3& chunkPos);

    std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> m_chunks;
    std::unique_ptr<WorldGenerator> m_worldGenerator;
    uint64_t m_seed;
    
    // Track recently modified blocks for physics updates
    std::deque<ModifiedBlock> m_recentlyModifiedBlocks;
    
    // View distance (in chunks)
    int m_viewDistance;
    
    // Disable greedy meshing (for debugging)
    bool m_disableGreedyMeshing;
    
    // To track pending chunk operations
    mutable int m_pendingChunkOperations;
}; 