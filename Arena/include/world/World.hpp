#pragma once

#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <deque>
#include "Chunk.hpp"
#include "WorldGenerator.hpp"
#include <string>
#include <unordered_set>
#include <atomic>
#include <map>

// Forward declarations
class ChunkVisibilityManager;
class Player;

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

// Hash function for glm::ivec3 to use with unordered_set
struct Vec3Hash {
    size_t operator()(const glm::ivec3& vec) const {
        // Combine the hash of the three components
        size_t hash = std::hash<int>()(vec.x);
        hash ^= std::hash<int>()(vec.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<int>()(vec.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        return hash;
    }
};

// Helper for 2D column coordinates
typedef std::pair<int, int> ColumnXZ;

// Metadata for a terrain column
struct ColumnMetadata {
    int topExposedHeight;    // Y-coordinate of the top exposed chunk
    int bottomExposedHeight; // Y-coordinate of the bottom exposed chunk
    
    ColumnMetadata() : topExposedHeight(0), bottomExposedHeight(0) {}
    ColumnMetadata(int top, int bottom) : topExposedHeight(top), bottomExposedHeight(bottom) {}
};

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
    void generateInitialArea(const glm::vec3& spawnPosition);
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

    // Chunk management with exposure-based loading
    void evaluateChunksNeeded(const Player& player);
    void evaluateChunksNeeded(const glm::vec3& playerPos);
    void processChunkQueues();
    const std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>>& getChunks() const { return m_chunks; }
    
    // Coordinate conversion
    glm::ivec3 worldToChunkPos(const glm::vec3& worldPos) const;
    glm::ivec3 worldToLocalPos(const glm::vec3& worldPos) const;
    
    // Mesh management
    void updateChunkMeshes(const glm::ivec3& chunkPos, bool disableGreedyMeshing = false);
    void updateDirtyChunkMeshes(int maxUpdatesPerFrame);
    
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

    // Chunk visibility system
    bool isChunkVisible(const glm::ivec3& chunkPos, const glm::vec3& playerPos, const glm::vec3& playerForward) const;
    void updateVisibleChunks(const glm::vec3& playerPos, const glm::vec3& playerForward);
    bool shouldLoadChunk(const glm::ivec3& chunkPos, const glm::ivec3& playerChunkPos) const;
    size_t getVisibleChunksCount() const { return m_visibleChunks.size(); }
    int getDirtyChunkCount() const;
    
    // Add a chunk directly to the visible chunks set (for ChunkVisibilityManager)
    void addToVisibleChunks(const glm::ivec3& chunkPos) { m_visibleChunks.insert(chunkPos); }
    
    // Clear the visible chunks set (for ChunkVisibilityManager)
    void clearVisibleChunks() { m_visibleChunks.clear(); }

    // Add a method to update chunk visibility using the new manager
    void updateChunkVisibilityForPlayer(const Player& player);

    // Add method to get last player position
    glm::vec3 getLastPlayerPosition() const { return m_lastPlayerPosition; }

    // Add this method
    bool isInitialized() const { return m_initialized; }
    
    // Exposure-based chunk management methods
    bool isChunkExposed(const glm::ivec3& chunkPos) const;
    bool isAdjacentToExposedChunk(const glm::ivec3& chunkPos) const;
    void updateColumnMetadata(const glm::ivec3& chunkPos);
    void updateAllColumnMetadata();
    void dumpColumnDebugInfo() const;

    // Method to periodically reset chunk visibility and dirty states
    void resetChunkStates();
    
    // Method to reset the visible chunks list
    void resetVisibleChunks();
    
    // Flag to control greedy meshing

private:
    Chunk* getChunkAt(const glm::ivec3& chunkPos);

    uint64_t m_seed;
    std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, std::hash<glm::ivec3>> m_chunks;
    std::unordered_set<glm::ivec3, Vec3Hash> m_visibleChunks;
    std::unique_ptr<WorldGenerator> m_worldGenerator;
    std::unique_ptr<ChunkVisibilityManager> m_visibilityManager;
    
    // Track player position for chunk management
    glm::vec3 m_lastPlayerPosition = glm::vec3(0, 0, 0);
    
    int m_viewDistance;
    bool m_disableGreedyMeshing;
    
    // Queue for chunk loading
    std::deque<glm::ivec3> m_chunksToLoadQueue;
    std::deque<glm::ivec3> m_chunksToUnloadQueue;
    
    // Track pending chunk operations
    std::atomic<int> m_pendingChunkOperations;
    
    // Track recently modified blocks for physics updates
    std::deque<ModifiedBlock> m_recentlyModifiedBlocks;
    
    // Column tracking for exposure-based loading (key = x,z coordinates)
    std::map<ColumnXZ, ColumnMetadata> m_columnMetadata;
    
    // Statistics
    std::atomic<int> m_maxSimultaneousChunksLoaded;

    // Helper method to check if a chunk is potentially visible from the air
    bool isVisibleFromAbove(const glm::ivec3& chunkPos, const glm::ivec3& playerChunkPos) const;
    
    // Helper methods for chunk visibility propagation
    void markChunkVisible(const glm::ivec3& chunkPos);
    void propagateVisibilityDownward(const glm::ivec3& chunkPos);
    
    // Helper methods for exposure-based loading
    void updateExposureOnBlockChange(const glm::ivec3& blockPos);
    bool shouldLoadBasedOnExposure(const glm::ivec3& chunkPos) const;
    
    // Add this member variable
    bool m_initialized = false;

    // Maximum number of visible chunks to render
    size_t m_maxVisibleChunks;
}; 