#include "world/World.hpp"
#include <fstream>
#include <cmath>
#include <iostream>

World::World(uint64_t seed)
    : m_seed(seed)
    , m_viewDistance(8)
{
    m_worldGenerator = std::make_unique<WorldGenerator>(seed);
    std::cout << "World created with seed: " << seed << std::endl;
}

World::~World() {
}

void World::initialize() {
    std::cout << "Initializing world..." << std::endl;
    
    // Generate chunks around origin
    int chunkCount = 0;
    for (int x = -2; x <= 2; x++) {
        for (int z = -2; z <= 2; z++) {
            generateChunk(glm::ivec2(x, z));
            chunkCount++;
            std::cout << "Generated chunk at position (" << x << ", " << z << ")" << std::endl;
        }
    }
    
    std::cout << "World initialization complete. Generated " << chunkCount << " chunks." << std::endl;
}

void World::generateChunk(const glm::ivec2& chunkPos) {
    if (m_chunks.find(chunkPos) != m_chunks.end()) {
        return;
    }

    auto chunk = std::make_unique<Chunk>(chunkPos.x, chunkPos.y);
    
    // Generate terrain for this chunk
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            int worldX = chunkPos.x * CHUNK_SIZE + x;
            int worldZ = chunkPos.y * CHUNK_SIZE + z;
            
            int height = m_worldGenerator->getHeight(worldX, worldZ);
            
            for (int y = 0; y < height; y++) {
                int blockType = m_worldGenerator->getBlockType(worldX, y, worldZ);
                if (blockType > 0) {
                    chunk->setBlock(x, y, z, blockType);
                }
            }
        }
    }
    
    chunk->generateMesh();
    m_chunks[chunkPos] = std::move(chunk);
}

void World::loadChunk(const glm::ivec2& chunkPos) {
    if (m_chunks.find(chunkPos) != m_chunks.end()) {
        return;
    }

    auto chunk = std::make_unique<Chunk>(chunkPos.x, chunkPos.y);
    std::string filename = "chunks/" + std::to_string(chunkPos.x) + "_" + std::to_string(chunkPos.y) + ".chunk";
    
    if (chunk->deserialize(filename)) {
        chunk->generateMesh();
        m_chunks[chunkPos] = std::move(chunk);
    } else {
        generateChunk(chunkPos);
    }
}

void World::unloadChunk(const glm::ivec2& chunkPos) {
    auto it = m_chunks.find(chunkPos);
    if (it != m_chunks.end()) {
        std::string filename = "chunks/" + std::to_string(chunkPos.x) + "_" + std::to_string(chunkPos.y) + ".chunk";
        it->second->serialize(filename);
        m_chunks.erase(it);
    }
}

int World::getBlock(const glm::ivec3& worldPos) const {
    glm::ivec2 chunkPos = worldToChunkPos(glm::vec3(worldPos));
    glm::ivec3 localPos = worldToLocalPos(glm::vec3(worldPos));
    
    auto it = m_chunks.find(chunkPos);
    if (it != m_chunks.end()) {
        return it->second->getBlock(localPos.x, localPos.y, localPos.z);
    }
    return 0; // Air for unloaded chunks
}

void World::setBlock(const glm::ivec3& worldPos, int blockType) {
    glm::ivec2 chunkPos = worldToChunkPos(glm::vec3(worldPos));
    glm::ivec3 localPos = worldToLocalPos(glm::vec3(worldPos));
    
    auto it = m_chunks.find(chunkPos);
    if (it != m_chunks.end()) {
        it->second->setBlock(localPos.x, localPos.y, localPos.z, blockType);
        it->second->generateMesh();
        
        // Update adjacent chunks if block is on chunk border
        if (localPos.x == 0) {
            auto chunk = getChunkAt(glm::ivec2(chunkPos.x - 1, chunkPos.y));
            if (chunk) chunk->generateMesh();
        }
        if (localPos.x == CHUNK_SIZE - 1) {
            auto chunk = getChunkAt(glm::ivec2(chunkPos.x + 1, chunkPos.y));
            if (chunk) chunk->generateMesh();
        }
        if (localPos.z == 0) {
            auto chunk = getChunkAt(glm::ivec2(chunkPos.x, chunkPos.y - 1));
            if (chunk) chunk->generateMesh();
        }
        if (localPos.z == CHUNK_SIZE - 1) {
            auto chunk = getChunkAt(glm::ivec2(chunkPos.x, chunkPos.y + 1));
            if (chunk) chunk->generateMesh();
        }
    }
}

bool World::saveToFile(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Write seed
    file.write(reinterpret_cast<const char*>(&m_seed), sizeof(m_seed));
    
    // Write number of chunks
    size_t numChunks = m_chunks.size();
    file.write(reinterpret_cast<const char*>(&numChunks), sizeof(numChunks));
    
    // Write each chunk
    for (const auto& pair : m_chunks) {
        // Write chunk position
        file.write(reinterpret_cast<const char*>(&pair.first), sizeof(pair.first));
        
        // Write chunk data
        std::string chunkFilename = filename + "_chunk_" + 
            std::to_string(pair.first.x) + "_" + 
            std::to_string(pair.first.y);
        pair.second->serialize(chunkFilename);
    }
    
    return true;
}

bool World::loadFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Clear existing chunks
    m_chunks.clear();

    // Read seed
    file.read(reinterpret_cast<char*>(&m_seed), sizeof(m_seed));
    m_worldGenerator = std::make_unique<WorldGenerator>(m_seed);
    
    // Read number of chunks
    size_t numChunks;
    file.read(reinterpret_cast<char*>(&numChunks), sizeof(numChunks));
    
    // Read each chunk
    for (size_t i = 0; i < numChunks; i++) {
        // Read chunk position
        glm::ivec2 chunkPos;
        file.read(reinterpret_cast<char*>(&chunkPos), sizeof(chunkPos));
        
        // Create and load chunk
        auto chunk = std::make_unique<Chunk>(chunkPos.x, chunkPos.y);
        std::string chunkFilename = filename + "_chunk_" + 
            std::to_string(chunkPos.x) + "_" + 
            std::to_string(chunkPos.y);
        
        if (chunk->deserialize(chunkFilename)) {
            chunk->generateMesh();
            m_chunks[chunkPos] = std::move(chunk);
        }
    }
    
    return true;
}

void World::updateChunks(const glm::vec3& playerPos) {
    glm::ivec2 playerChunkPos = worldToChunkPos(playerPos);
    
    // Unload distant chunks
    std::vector<glm::ivec2> chunksToUnload;
    for (const auto& pair : m_chunks) {
        glm::ivec2 offset = pair.first - playerChunkPos;
        if (std::abs(offset.x) > m_viewDistance || std::abs(offset.y) > m_viewDistance) {
            chunksToUnload.push_back(pair.first);
        }
    }
    
    for (const auto& pos : chunksToUnload) {
        unloadChunk(pos);
    }
    
    // Load nearby chunks
    for (int x = -m_viewDistance; x <= m_viewDistance; x++) {
        for (int z = -m_viewDistance; z <= m_viewDistance; z++) {
            glm::ivec2 chunkPos = playerChunkPos + glm::ivec2(x, z);
            if (m_chunks.find(chunkPos) == m_chunks.end()) {
                loadChunk(chunkPos);
            }
        }
    }
}

glm::ivec2 World::worldToChunkPos(const glm::vec3& worldPos) const {
    return glm::ivec2(
        static_cast<int>(std::floor(worldPos.x)) / CHUNK_SIZE,
        static_cast<int>(std::floor(worldPos.z)) / CHUNK_SIZE
    );
}

glm::ivec3 World::worldToLocalPos(const glm::vec3& worldPos) const {
    return glm::ivec3(
        static_cast<int>(std::floor(worldPos.x)) % CHUNK_SIZE,
        static_cast<int>(std::floor(worldPos.y)),
        static_cast<int>(std::floor(worldPos.z)) % CHUNK_SIZE
    );
}

Chunk* World::getChunkAt(const glm::ivec2& chunkPos) {
    auto it = m_chunks.find(chunkPos);
    return it != m_chunks.end() ? it->second.get() : nullptr;
} 