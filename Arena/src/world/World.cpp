#include "world/World.hpp"
#include <fstream>
#include <cmath>
#include <iostream>
#include <algorithm>

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
    
    // Generate chunks around origin for a small area
    int chunkCount = 0;
    for (int x = -2; x <= 2; x++) {
        for (int z = -2; z <= 2; z++) {
            // Generate chunks for a reasonable height range
            for (int y = 0; y < 8; y++) {  // 0-128 in voxel height (8 chunks * 16 height)
                generateChunk(glm::ivec3(x, y, z));
                chunkCount++;
            }
            std::cout << "Generated chunks at XZ position (" << x << ", " << z << ")" << std::endl;
        }
    }
    
    std::cout << "World initialization complete. Generated " << chunkCount << " chunks." << std::endl;
}

void World::generateChunk(const glm::ivec3& chunkPos) {
    if (m_chunks.find(chunkPos) != m_chunks.end()) {
        return;
    }

    // Create a new chunk at the given position with proper 3D coordinates
    auto chunk = std::make_unique<Chunk>(chunkPos.x, chunkPos.y, chunkPos.z);
    
    // Set the world pointer so the chunk can query neighbor blocks
    chunk->setWorld(this);
    
    // Generate terrain for this chunk
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            int worldX = chunkPos.x * CHUNK_SIZE + x;
            int worldZ = chunkPos.z * CHUNK_SIZE + z;
            
            // Get height at this position
            int totalHeight = m_worldGenerator->getHeight(worldX, worldZ);
            
            // Calculate range for this chunk
            int chunkMinY = chunkPos.y * CHUNK_HEIGHT;
            int chunkMaxY = chunkMinY + CHUNK_HEIGHT;
            
            // Fill blocks within this chunk's y-range
            for (int localY = 0; localY < CHUNK_HEIGHT; localY++) {
                int worldY = chunkMinY + localY;
                
                if (worldY < totalHeight) {
                    int blockType = m_worldGenerator->getBlockType(worldX, worldY, worldZ);
                    if (blockType > 0) {
                        chunk->setBlock(x, localY, z, blockType);
                    }
                }
            }
        }
    }
    
    // Store the chunk before generating meshes
    m_chunks[chunkPos] = std::move(chunk);
    
    // Update meshes for this chunk and adjacent chunks
    updateChunkMeshes(chunkPos, false);
}

void World::loadChunk(const glm::ivec3& chunkPos) {
    if (m_chunks.find(chunkPos) != m_chunks.end()) {
        return;
    }

    auto chunk = std::make_unique<Chunk>(chunkPos.x, chunkPos.y, chunkPos.z);
    
    // Set the world pointer so the chunk can query neighbor blocks
    chunk->setWorld(this);
    
    std::string filename = "chunks/" + std::to_string(chunkPos.x) + "_" + 
                          std::to_string(chunkPos.y) + "_" + 
                          std::to_string(chunkPos.z) + ".chunk";
    
    if (chunk->deserialize(filename)) {
        m_chunks[chunkPos] = std::move(chunk);
    } else {
        generateChunk(chunkPos);
        return; // Return since generateChunk adds the chunk to m_chunks
    }
    
    // After loading/generating a chunk, update meshes for this chunk and adjacent chunks
    updateChunkMeshes(chunkPos, false);
}

void World::unloadChunk(const glm::ivec3& chunkPos) {
    auto it = m_chunks.find(chunkPos);
    if (it != m_chunks.end()) {
        std::string filename = "chunks/" + std::to_string(chunkPos.x) + "_" + 
                              std::to_string(chunkPos.y) + "_" + 
                              std::to_string(chunkPos.z) + ".chunk";
        it->second->serialize(filename);
        m_chunks.erase(it);
    }
}

int World::getBlock(const glm::ivec3& worldPos) const {
    glm::ivec3 chunkPos = worldToChunkPos(glm::vec3(worldPos));
    glm::ivec3 localPos = worldToLocalPos(glm::vec3(worldPos));
    
    auto it = m_chunks.find(chunkPos);
    if (it != m_chunks.end()) {
        return it->second->getBlock(localPos.x, localPos.y, localPos.z);
    }
    return 0; // Air for unloaded chunks
}

void World::setBlock(const glm::ivec3& worldPos, int blockType) {
    glm::ivec3 chunkPos = worldToChunkPos(glm::vec3(worldPos));
    glm::ivec3 localPos = worldToLocalPos(glm::vec3(worldPos));
    
    auto it = m_chunks.find(chunkPos);
    if (it != m_chunks.end()) {
        it->second->setBlock(localPos.x, localPos.y, localPos.z, blockType);
        it->second->generateMesh();
        
        // Update adjacent chunks if block is on chunk border
        if (localPos.x == 0) {
            auto chunk = getChunkAt(glm::ivec3(chunkPos.x - 1, chunkPos.y, chunkPos.z));
            if (chunk) chunk->generateMesh();
        }
        if (localPos.x == CHUNK_SIZE - 1) {
            auto chunk = getChunkAt(glm::ivec3(chunkPos.x + 1, chunkPos.y, chunkPos.z));
            if (chunk) chunk->generateMesh();
        }
        if (localPos.y == 0) {
            auto chunk = getChunkAt(glm::ivec3(chunkPos.x, chunkPos.y - 1, chunkPos.z));
            if (chunk) chunk->generateMesh();
        }
        if (localPos.y == CHUNK_HEIGHT - 1) {
            auto chunk = getChunkAt(glm::ivec3(chunkPos.x, chunkPos.y + 1, chunkPos.z));
            if (chunk) chunk->generateMesh();
        }
        if (localPos.z == 0) {
            auto chunk = getChunkAt(glm::ivec3(chunkPos.x, chunkPos.y, chunkPos.z - 1));
            if (chunk) chunk->generateMesh();
        }
        if (localPos.z == CHUNK_SIZE - 1) {
            auto chunk = getChunkAt(glm::ivec3(chunkPos.x, chunkPos.y, chunkPos.z + 1));
            if (chunk) chunk->generateMesh();
        }
    }
}

bool World::saveToFile(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return false;
    }
    
    // Write seed
    file.write(reinterpret_cast<const char*>(&m_seed), sizeof(m_seed));
    
    // Write number of chunks
    size_t numChunks = m_chunks.size();
    file.write(reinterpret_cast<const char*>(&numChunks), sizeof(numChunks));
    
    // Write each chunk's position
    for (const auto& pair : m_chunks) {
        const glm::ivec3& pos = pair.first;
        file.write(reinterpret_cast<const char*>(&pos), sizeof(pos));
    }
    
    // Save each chunk to its own file
    for (const auto& pair : m_chunks) {
        const glm::ivec3& pos = pair.first;
        const Chunk* chunk = pair.second.get();
        
        std::string chunkFile = "chunks/" + std::to_string(pos.x) + "_" + 
                               std::to_string(pos.y) + "_" + 
                               std::to_string(pos.z) + ".chunk";
        
        chunk->serialize(chunkFile);
    }
    
    return true;
}

bool World::loadFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for reading: " << filename << std::endl;
        return false;
    }
    
    // Clear existing chunks
    m_chunks.clear();
    
    // Read seed
    file.read(reinterpret_cast<char*>(&m_seed), sizeof(m_seed));
    
    // Recreate world generator
    m_worldGenerator = std::make_unique<WorldGenerator>(m_seed);
    
    // Read number of chunks
    size_t numChunks;
    file.read(reinterpret_cast<char*>(&numChunks), sizeof(numChunks));
    
    // Read each chunk's position and load it
    for (size_t i = 0; i < numChunks; i++) {
        glm::ivec3 pos;
        file.read(reinterpret_cast<char*>(&pos), sizeof(pos));
        
        // Load this chunk
        loadChunk(pos);
    }
    
    return true;
}

void World::updateChunks(const glm::vec3& playerPos) {
    // Convert player position to chunk coordinates
    glm::ivec3 playerChunkPos = worldToChunkPos(playerPos);
    
    // Keep track of chunks to load and unload
    std::vector<glm::ivec3> chunksToLoad;
    std::vector<glm::ivec3> chunksToUnload;
    
    // Determine chunks to load (within view distance)
    for (int x = playerChunkPos.x - m_viewDistance; x <= playerChunkPos.x + m_viewDistance; x++) {
        for (int z = playerChunkPos.z - m_viewDistance; z <= playerChunkPos.z + m_viewDistance; z++) {
            // Calculate squared distance to player chunk (in 2D for XZ plane)
            int dx = x - playerChunkPos.x;
            int dz = z - playerChunkPos.z;
            int squaredDist = dx * dx + dz * dz;
            
            if (squaredDist <= m_viewDistance * m_viewDistance) {
                // Calculate vertical range based on player height
                int minY = std::max(0, (playerChunkPos.y - 2));
                int maxY = playerChunkPos.y + 4; // Render more above than below
                
                // Add chunks in the vertical range
                for (int y = minY; y <= maxY; y++) {
                    glm::ivec3 chunkPos(x, y, z);
                    if (m_chunks.find(chunkPos) == m_chunks.end()) {
                        // Store distance for priority-based loading
                        chunksToLoad.push_back(chunkPos);
                    }
                }
            }
        }
    }
    
    // Sort chunks to load by distance to player (closest first)
    std::sort(chunksToLoad.begin(), chunksToLoad.end(), 
        [playerChunkPos](const glm::ivec3& a, const glm::ivec3& b) {
            // Calculate squared distances
            float distA = glm::length(glm::vec3(a - playerChunkPos));
            float distB = glm::length(glm::vec3(b - playerChunkPos));
            return distA < distB;
        }
    );
    
    // Determine chunks to unload (outside view distance)
    for (const auto& pair : m_chunks) {
        const glm::ivec3& chunkPos = pair.first;
        
        // Calculate squared distance from player chunk
        int dx = chunkPos.x - playerChunkPos.x;
        int dz = chunkPos.z - playerChunkPos.z;
        int squaredDist = dx * dx + dz * dz;
        
        // Keep chunks that are within an extended unload distance to prevent frequent loading/unloading
        int unloadDistance = m_viewDistance + 3;
        bool tooFarHorizontal = squaredDist > unloadDistance * unloadDistance;
        bool tooFarVertical = chunkPos.y < (playerChunkPos.y - 4) || chunkPos.y > (playerChunkPos.y + 6);
        
        if (tooFarHorizontal || tooFarVertical) {
            chunksToUnload.push_back(chunkPos);
        }
    }
    
    // Limit chunk operations per frame to avoid frame drops
    const int maxChunksToLoadPerFrame = 2;
    const int maxChunksToUnloadPerFrame = 2;
    
    // Always load the chunk the player is in first to prevent falling through ground
    for (auto it = chunksToLoad.begin(); it != chunksToLoad.end();) {
        if (*it == playerChunkPos || 
            *it == glm::ivec3(playerChunkPos.x, playerChunkPos.y - 1, playerChunkPos.z)) {
            loadChunk(*it);
            it = chunksToLoad.erase(it);
        } else {
            ++it;
        }
    }
    
    // Load limited number of chunks per frame
    int chunksLoaded = 0;
    for (const auto& chunkPos : chunksToLoad) {
        if (chunksLoaded >= maxChunksToLoadPerFrame) break;
        loadChunk(chunkPos);
        chunksLoaded++;
    }
    
    // Unload limited number of chunks per frame
    int chunksUnloaded = 0;
    for (const auto& chunkPos : chunksToUnload) {
        if (chunksUnloaded >= maxChunksToUnloadPerFrame) break;
        unloadChunk(chunkPos);
        chunksUnloaded++;
    }
    
    // Only log if operations were performed
    if (chunksLoaded > 0 || chunksUnloaded > 0) {
        std::cout << "Updated chunks: loaded " << chunksLoaded 
                  << ", unloaded " << chunksUnloaded
                  << ", total active: " << m_chunks.size() 
                  << ", remaining to load: " << (chunksToLoad.size() - chunksLoaded) << std::endl;
    }
}

glm::ivec3 World::worldToChunkPos(const glm::vec3& worldPos) const {
    return glm::ivec3(
        static_cast<int>(std::floor(worldPos.x / CHUNK_SIZE)),
        static_cast<int>(std::floor(worldPos.y / CHUNK_HEIGHT)),
        static_cast<int>(std::floor(worldPos.z / CHUNK_SIZE))
    );
}

glm::ivec3 World::worldToLocalPos(const glm::vec3& worldPos) const {
    return glm::ivec3(
        static_cast<int>(std::floor(worldPos.x)) % CHUNK_SIZE + (worldPos.x < 0 ? CHUNK_SIZE : 0),
        static_cast<int>(std::floor(worldPos.y)) % CHUNK_HEIGHT + (worldPos.y < 0 ? CHUNK_HEIGHT : 0),
        static_cast<int>(std::floor(worldPos.z)) % CHUNK_SIZE + (worldPos.z < 0 ? CHUNK_SIZE : 0)
    );
}

Chunk* World::getChunkAt(const glm::ivec3& chunkPos) {
    auto it = m_chunks.find(chunkPos);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

// Add this new method to update meshes for a chunk and its neighbors
void World::updateChunkMeshes(const glm::ivec3& chunkPos, bool disableGreedyMeshing) {
    // Force regenerate this chunk's mesh and mark it for update
    Chunk* chunk = getChunkAt(chunkPos);
    if (chunk) {
        std::cout << "Regenerating mesh for chunk " << chunkPos.x << "," << chunkPos.y << "," << chunkPos.z << std::endl;
        chunk->setDirty(true);
        chunk->generateMesh(disableGreedyMeshing);
        
        // Log confirmation that mesh was generated
        const auto& vertices = chunk->getMeshVertices();
        const auto& indices = chunk->getMeshIndices();
        std::cout << "Generated mesh for chunk (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z 
                  << ") with " << vertices.size() / 5 << " vertices and " << indices.size() << " indices" << std::endl;
    }
    
    // Update meshes for all 6 adjacent chunks
    const glm::ivec3 neighbors[6] = {
        glm::ivec3(chunkPos.x + 1, chunkPos.y, chunkPos.z), // +X
        glm::ivec3(chunkPos.x - 1, chunkPos.y, chunkPos.z), // -X
        glm::ivec3(chunkPos.x, chunkPos.y + 1, chunkPos.z), // +Y
        glm::ivec3(chunkPos.x, chunkPos.y - 1, chunkPos.z), // -Y
        glm::ivec3(chunkPos.x, chunkPos.y, chunkPos.z + 1), // +Z
        glm::ivec3(chunkPos.x, chunkPos.y, chunkPos.z - 1)  // -Z
    };
    
    for (const auto& neighborPos : neighbors) {
        Chunk* neighbor = getChunkAt(neighborPos);
        if (neighbor) {
            std::cout << "Regenerating mesh for neighbor chunk " << neighborPos.x << "," << neighborPos.y << "," << neighborPos.z << std::endl;
            neighbor->setDirty(true);
            neighbor->generateMesh(disableGreedyMeshing);
            
            // Log confirmation that mesh was generated
            const auto& vertices = neighbor->getMeshVertices();
            const auto& indices = neighbor->getMeshIndices();
            std::cout << "Generated mesh for chunk (" << neighborPos.x << ", " << neighborPos.y << ", " << neighborPos.z 
                      << ") with " << vertices.size() / 5 << " vertices and " << indices.size() << " indices" << std::endl;
        }
    }
}

void World::removeChunk(const glm::ivec3& chunkPos) {
    auto it = m_chunks.find(chunkPos);
    if (it != m_chunks.end()) {
        m_chunks.erase(it);
        
        // Update adjacent chunks' meshes
        updateChunkMeshes(chunkPos, false);
    }
} 