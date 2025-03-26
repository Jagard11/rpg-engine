#pragma once

#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include "Chunk.hpp"
#include "WorldGenerator.hpp"

// Hash function for glm::ivec2 to use in unordered_map
struct ChunkPosHash {
    size_t operator()(const glm::ivec2& pos) const {
        return std::hash<int>()(pos.x) ^ (std::hash<int>()(pos.y) << 1);
    }
};

class World {
public:
    static const int CHUNK_SIZE = 16;
    static const int WORLD_HEIGHT = 256;
    
    World(uint64_t seed);
    ~World();

    // World generation and management
    void initialize();
    void generateChunk(const glm::ivec2& chunkPos);
    void loadChunk(const glm::ivec2& chunkPos);
    void unloadChunk(const glm::ivec2& chunkPos);
    
    // Block manipulation
    int getBlock(const glm::ivec3& worldPos) const;
    void setBlock(const glm::ivec3& worldPos, int blockType);
    
    // Serialization
    bool saveToFile(const std::string& filename);
    bool loadFromFile(const std::string& filename);

    // Chunk management
    void updateChunks(const glm::vec3& playerPos);
    const std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>, ChunkPosHash>& getChunks() const { return m_chunks; }

private:
    glm::ivec2 worldToChunkPos(const glm::vec3& worldPos) const;
    glm::ivec3 worldToLocalPos(const glm::vec3& worldPos) const;
    Chunk* getChunkAt(const glm::ivec2& chunkPos);

    std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>, ChunkPosHash> m_chunks;
    std::unique_ptr<WorldGenerator> m_worldGenerator;
    uint64_t m_seed;
    
    // View distance in chunks
    int m_viewDistance;
}; 