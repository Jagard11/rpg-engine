#include "world/World.hpp"
#include <fstream>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <GLFW/glfw3.h>

World::World(uint64_t seed)
    : m_seed(seed)
    , m_viewDistance(8)
    , m_disableGreedyMeshing(false)  // Enable greedy meshing by default
    , m_pendingChunkOperations(0)
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

    // First, check if there's a saved version of this chunk
    std::string filename = "chunks/" + std::to_string(chunkPos.x) + "_" + 
                           std::to_string(chunkPos.y) + "_" + 
                           std::to_string(chunkPos.z) + ".chunk";
    
    if (std::filesystem::exists(filename)) {
        // If a saved version exists, load it instead of generating a new one
        loadChunk(chunkPos);
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
    
    // Mark this as a procedurally generated chunk, not modified by the player
    chunk->setModified(false);
    
    // Store the chunk before generating meshes
    m_chunks[chunkPos] = std::move(chunk);
    
    // Update meshes for this chunk and adjacent chunks
    updateChunkMeshes(chunkPos, m_disableGreedyMeshing);
}

void World::loadChunk(const glm::ivec3& chunkPos) {
    if (m_chunks.find(chunkPos) != m_chunks.end()) {
        return;
    }

    // Create the chunk directory if it doesn't exist
    std::filesystem::create_directories("chunks");
    
    // Try to load the chunk from file first
    std::string filename = "chunks/" + std::to_string(chunkPos.x) + "_" + 
                           std::to_string(chunkPos.y) + "_" + 
                           std::to_string(chunkPos.z) + ".chunk";
    
    auto chunk = std::make_unique<Chunk>(chunkPos.x, chunkPos.y, chunkPos.z);
    chunk->setWorld(this);
    
    // First try to load from file
    if (std::filesystem::exists(filename) && chunk->deserialize(filename)) {
        std::cout << "Loaded existing chunk from file: " << filename << std::endl;
        m_chunks[chunkPos] = std::move(chunk);
    } else {
        // If file doesn't exist or can't be loaded, generate a new chunk
        generateChunk(chunkPos);
        return; // Return since generateChunk adds the chunk to m_chunks
    }
    
    // After loading/generating a chunk, update meshes for this chunk and adjacent chunks
    updateChunkMeshes(chunkPos, m_disableGreedyMeshing);
}

void World::unloadChunk(const glm::ivec3& chunkPos) {
    auto it = m_chunks.find(chunkPos);
    if (it != m_chunks.end()) {
        // Create the chunk directory if it doesn't exist
        std::filesystem::create_directories("chunks");
        
        // Save the chunk data to file
        std::string filename = "chunks/" + std::to_string(chunkPos.x) + "_" + 
                              std::to_string(chunkPos.y) + "_" + 
                              std::to_string(chunkPos.z) + ".chunk";
        
        if (it->second->serialize(filename)) {
            std::cout << "Saved chunk to file: " << filename << std::endl;
        } else {
            std::cerr << "Failed to save chunk to file: " << filename << std::endl;
        }
        
        // Remove the chunk from memory
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
        Chunk* chunk = it->second.get();
        
        // Get current block type
        int currentBlock = chunk->getBlock(localPos.x, localPos.y, localPos.z);
        
        // Only update if the block is actually changing
        if (currentBlock != blockType) {
            std::cout << "World::setBlock - Setting block at world:(" 
                      << worldPos.x << "," << worldPos.y << "," << worldPos.z 
                      << ") chunk:(" << chunkPos.x << "," << chunkPos.y << "," << chunkPos.z
                      << ") local:(" << localPos.x << "," << localPos.y << "," << localPos.z
                      << ") from " << currentBlock << " to " << blockType << std::endl;
            
            // Track modified block for physics updates before changing the block
            // This is especially important for supporting blocks turning into air
            m_recentlyModifiedBlocks.push_back({worldPos, currentBlock, blockType, glfwGetTime()});
            
            // Set the block
            chunk->setBlock(localPos.x, localPos.y, localPos.z, blockType);
            
            // Mark the chunk as modified by the player
            chunk->setModified(true);
            
            // Maintain a reasonable size for the recently modified blocks list
            if (m_recentlyModifiedBlocks.size() > 100) {
                m_recentlyModifiedBlocks.pop_front();
            }
            
            // Instead of handling each boundary separately, use the updateChunkMeshes
            // method which will update this chunk and all 6 neighboring chunks
            bool isChunkBoundary = 
                localPos.x == 0 || localPos.x == (CHUNK_SIZE - 1) ||
                localPos.y == 0 || localPos.y == (CHUNK_HEIGHT - 1) ||
                localPos.z == 0 || localPos.z == (CHUNK_SIZE - 1);
                
            if (isChunkBoundary) {
                // If this is a boundary voxel, update this chunk and all neighbors
                // to ensure proper mesh updates on all sides
                std::cout << "Updating all chunks due to boundary voxel modification" << std::endl;
                
                // CRITICAL FIX: Use a more conservative approach for chunk boundaries
                // First, update the current chunk explicitly
                chunk->generateMesh();
                chunk->setDirty(true);
                
                // Then update all adjacent chunks to ensure proper rendering at boundaries
                updateChunkMeshes(chunkPos);
            } else {
                // If it's not a boundary voxel, just update this chunk directly
                std::cout << "Only updating current chunk for non-boundary voxel" << std::endl;
                chunk->generateMesh();
            }
        }
    } else {
        std::cout << "World::setBlock - Failed: No chunk at position ("
                  << chunkPos.x << "," << chunkPos.y << "," << chunkPos.z << ")" << std::endl;
    }
}

bool World::serialize(const std::string& filename) const {
    // Create directories for the save file and chunk data
    std::filesystem::path saveDir = std::filesystem::path(filename).parent_path();
    std::filesystem::create_directories(saveDir);
    std::filesystem::create_directories("chunks");
    
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
    int chunksSuccessfullySaved = 0;
    for (const auto& pair : m_chunks) {
        const glm::ivec3& pos = pair.first;
        const Chunk* chunk = pair.second.get();
        
        std::string chunkFile = "chunks/" + std::to_string(pos.x) + "_" + 
                               std::to_string(pos.y) + "_" + 
                               std::to_string(pos.z) + ".chunk";
        
        if (chunk->serialize(chunkFile)) {
            chunksSuccessfullySaved++;
        } else {
            std::cerr << "Failed to save chunk to file: " << chunkFile << std::endl;
        }
    }
    
    std::cout << "World saved to " << filename << ". " 
              << chunksSuccessfullySaved << "/" << numChunks 
              << " chunks successfully saved." << std::endl;
    
    return true;
}

bool World::deserialize(const std::string& filename) {
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
    
    // Create the chunks directory if it doesn't exist
    std::filesystem::create_directories("chunks");
    
    // Read each chunk's position and load it
    int chunksSuccessfullyLoaded = 0;
    for (size_t i = 0; i < numChunks; i++) {
        glm::ivec3 pos;
        file.read(reinterpret_cast<char*>(&pos), sizeof(pos));
        
        // Check if the chunk file exists
        std::string chunkFile = "chunks/" + std::to_string(pos.x) + "_" + 
                               std::to_string(pos.y) + "_" + 
                               std::to_string(pos.z) + ".chunk";
        
        try {
            if (std::filesystem::exists(chunkFile)) {
                // Load this chunk
                loadChunk(pos);
                chunksSuccessfullyLoaded++;
            } else {
                // Generate if chunk file doesn't exist
                generateChunk(pos);
                chunksSuccessfullyLoaded++;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading/generating chunk at position " 
                     << pos.x << "," << pos.y << "," << pos.z << ": " << e.what() << std::endl;
        }
    }
    
    std::cout << "World loaded from " << filename << ". " 
              << chunksSuccessfullyLoaded << "/" << numChunks 
              << " chunks successfully loaded/generated." << std::endl;
    
    return true;
}

void World::updateChunks(const glm::vec3& playerPos) {
    // Convert player position to chunk coordinates
    glm::ivec3 playerChunkPos = worldToChunkPos(playerPos);
    
    // Track chunks to load and unload
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
        // Increase unload distance to reduce chunk loading/unloading jitter
        int unloadDistance = m_viewDistance + 4; // Increased from 3 to 4
        bool tooFarHorizontal = squaredDist > unloadDistance * unloadDistance;
        bool tooFarVertical = chunkPos.y < (playerChunkPos.y - 4) || chunkPos.y > (playerChunkPos.y + 6);
        
        if (tooFarHorizontal || tooFarVertical) {
            // Add to unload list, but prioritize unmodified chunks
            Chunk* chunk = pair.second.get();
            
            // If the chunk is modified by the player, make sure it's saved properly
            if (chunk && chunk->isModified()) {
                std::string filename = "chunks/" + std::to_string(chunkPos.x) + "_" + 
                                      std::to_string(chunkPos.y) + "_" + 
                                      std::to_string(chunkPos.z) + ".chunk";
                std::filesystem::create_directories("chunks");
                
                if (chunk->serialize(filename)) {
                    std::cout << "Saved player-modified chunk to file: " << filename << std::endl;
                }
            }
            
            chunksToUnload.push_back(chunkPos);
        }
    }
    
    // Update the pending chunk operations count
    m_pendingChunkOperations = chunksToLoad.size();
    
    // Increase the number of chunks loaded per frame to reduce jittering
    const int maxChunksToLoadPerFrame = 3; // Increased from 2 to 3
    const int maxChunksToUnloadPerFrame = 2; // Keep this the same
    
    // Always load the chunk the player is in and adjacent chunks first to prevent falling through ground
    // Add additional safety by loading more surrounding chunks
    std::vector<glm::ivec3> criticalChunks = {
        playerChunkPos, // Player's current chunk
        glm::ivec3(playerChunkPos.x, playerChunkPos.y - 1, playerChunkPos.z), // Below player
        glm::ivec3(playerChunkPos.x + 1, playerChunkPos.y, playerChunkPos.z), // +X
        glm::ivec3(playerChunkPos.x - 1, playerChunkPos.y, playerChunkPos.z), // -X
        glm::ivec3(playerChunkPos.x, playerChunkPos.y, playerChunkPos.z + 1), // +Z
        glm::ivec3(playerChunkPos.x, playerChunkPos.y, playerChunkPos.z - 1)  // -Z
    };
    
    // Process critical chunks first
    for (const auto& chunkPos : criticalChunks) {
        if (m_chunks.find(chunkPos) == m_chunks.end()) {
            loadChunk(chunkPos);
            
            // Remove from chunksToLoad if it was there
            auto it = std::find(chunksToLoad.begin(), chunksToLoad.end(), chunkPos);
            if (it != chunksToLoad.end()) {
                chunksToLoad.erase(it);
            }
        }
    }
    
    // Load limited number of chunks per frame
    int chunksLoaded = 0;
    auto it = chunksToLoad.begin();
    while (it != chunksToLoad.end() && chunksLoaded < maxChunksToLoadPerFrame) {
        loadChunk(*it);
        chunksLoaded++;
        it = chunksToLoad.erase(it);
    }
    
    // Delay unloading chunks to reduce jitter - only unload if we're not actively loading
    // This prioritizes loading over unloading to keep the game smoother
    int chunksUnloaded = 0;
    if (chunksLoaded < maxChunksToLoadPerFrame) {
        for (const auto& chunkPos : chunksToUnload) {
            if (chunksUnloaded >= maxChunksToUnloadPerFrame) break;
            
            // Don't unload critical chunks around the player
            if (std::find(criticalChunks.begin(), criticalChunks.end(), chunkPos) == criticalChunks.end()) {
                unloadChunk(chunkPos);
                chunksUnloaded++;
            }
        }
    }
    
    // Update pending chunk operations count after processing
    m_pendingChunkOperations = chunksToLoad.size() - chunksLoaded;
    
    // Only log if operations were performed
    if (chunksLoaded > 0 || chunksUnloaded > 0) {
        std::cout << "Updated chunks: loaded " << chunksLoaded 
                  << ", unloaded " << chunksUnloaded
                  << ", total active: " << m_chunks.size() 
                  << ", remaining to load: " << m_pendingChunkOperations << std::endl;
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
    // More robust handling of coordinate conversion that reduces jitter at boundaries
    
    // First, calculate the chunk coordinate
    glm::ivec3 chunkPos = worldToChunkPos(worldPos);
    
    // Calculate local coordinates within the chunk
    int localX = static_cast<int>(std::floor(worldPos.x)) - (chunkPos.x * CHUNK_SIZE);
    int localY = static_cast<int>(std::floor(worldPos.y)) - (chunkPos.y * CHUNK_HEIGHT);
    int localZ = static_cast<int>(std::floor(worldPos.z)) - (chunkPos.z * CHUNK_SIZE);
    
    // Handle negative coordinates more robustly
    if (localX < 0) localX += CHUNK_SIZE;
    if (localY < 0) localY += CHUNK_HEIGHT;
    if (localZ < 0) localZ += CHUNK_SIZE;
    
    // Ensure the values are within valid range for the chunk
    localX = glm::clamp(localX, 0, CHUNK_SIZE - 1);
    localY = glm::clamp(localY, 0, CHUNK_HEIGHT - 1);
    localZ = glm::clamp(localZ, 0, CHUNK_SIZE - 1);
    
    // Handle the special case of exact chunk boundaries
    // If position is exactly at a chunk border, assign it to the appropriate chunk
    const float EPSILON = 0.0001f;
    
    // Check if position is exactly at chunk boundaries
    bool xAtBoundary = std::abs(std::fmod(worldPos.x, CHUNK_SIZE)) < EPSILON || 
                      std::abs(std::fmod(worldPos.x, CHUNK_SIZE) - CHUNK_SIZE) < EPSILON;
    bool yAtBoundary = std::abs(std::fmod(worldPos.y, CHUNK_HEIGHT)) < EPSILON || 
                      std::abs(std::fmod(worldPos.y, CHUNK_HEIGHT) - CHUNK_HEIGHT) < EPSILON;
    bool zAtBoundary = std::abs(std::fmod(worldPos.z, CHUNK_SIZE)) < EPSILON || 
                      std::abs(std::fmod(worldPos.z, CHUNK_SIZE) - CHUNK_SIZE) < EPSILON;
                      
    // If we're exactly at a boundary, make a consistent choice about which chunk it belongs to
    if (xAtBoundary || yAtBoundary || zAtBoundary) {
        // Assign more consistently based on the fractional part
        // For coordinates like 16.0, 32.0, etc., use the next chunk
        if (xAtBoundary) {
            float xFrac = worldPos.x - std::floor(worldPos.x);
            if (xFrac < EPSILON && std::floor(worldPos.x) > 0 && 
                std::fmod(std::floor(worldPos.x), CHUNK_SIZE) == 0) {
                // At upper boundary like 16.0, 32.0 - we're at x=0 of the next chunk
                localX = 0;
            }
        }
        
        if (yAtBoundary) {
            float yFrac = worldPos.y - std::floor(worldPos.y);
            if (yFrac < EPSILON && std::floor(worldPos.y) > 0 && 
                std::fmod(std::floor(worldPos.y), CHUNK_HEIGHT) == 0) {
                // At upper boundary like 16.0, 32.0 - we're at y=0 of the next chunk 
                localY = 0;
            }
        }
        
        if (zAtBoundary) {
            float zFrac = worldPos.z - std::floor(worldPos.z);
            if (zFrac < EPSILON && std::floor(worldPos.z) > 0 && 
                std::fmod(std::floor(worldPos.z), CHUNK_SIZE) == 0) {
                // At upper boundary like 16.0, 32.0 - we're at z=0 of the next chunk
                localZ = 0;
            }
        }
    }
    
    return glm::ivec3(localX, localY, localZ);
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
        std::cout << "Regenerating mesh for chunk " << chunkPos.x << "," << chunkPos.y << "," << chunkPos.z 
                  << " with greedy meshing " << (disableGreedyMeshing ? "DISABLED" : "ENABLED") << std::endl;
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
            std::cout << "Regenerating mesh for neighbor chunk " << neighborPos.x << "," << neighborPos.y << "," << neighborPos.z 
                      << " with greedy meshing " << (disableGreedyMeshing ? "DISABLED" : "ENABLED") << std::endl;
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

World::RaycastResult World::raycast(const glm::vec3& start, const glm::vec3& direction, float maxDistance) const {
    RaycastResult result;
    result.hit = false;
    result.distance = maxDistance;
    
    // Normalize direction
    glm::vec3 dir = glm::normalize(direction);
    
    // Ray march through the world
    glm::vec3 currentPos = start;
    float stepSize = 0.05f; // Use a smaller step size for more precision
    
    for (float distance = 0.0f; distance < maxDistance; distance += stepSize) {
        // Get current block position (floor for proper block coords)
        glm::ivec3 blockPos = glm::ivec3(
            std::floor(currentPos.x),
            std::floor(currentPos.y),
            std::floor(currentPos.z)
        );
        
        // Get block type at current position
        int blockType = getBlock(blockPos);
        
        // If we hit a solid block (not air)
        if (blockType > 0) {
            // Calculate chunk position and local position for debugging
            glm::ivec3 chunkPos = worldToChunkPos(glm::vec3(blockPos));
            glm::ivec3 localPos = worldToLocalPos(glm::vec3(blockPos));
            
            // Check if we're at a chunk boundary
            bool isChunkBoundary = 
                localPos.x == 0 || localPos.x == (CHUNK_SIZE - 1) ||
                localPos.y == 0 || localPos.y == (CHUNK_HEIGHT - 1) ||
                localPos.z == 0 || localPos.z == (CHUNK_SIZE - 1);
                
            std::cout << "Raycast - Hit block of type " << blockType 
                      << " at position: (" << blockPos.x << ", " << blockPos.y << ", " << blockPos.z 
                      << "), chunk: (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z
                      << "), local: (" << localPos.x << ", " << localPos.y << ", " << localPos.z 
                      << "), is boundary: " << (isChunkBoundary ? "YES" : "NO") << std::endl;
            
            // Calculate fractional position within the block
            glm::vec3 fractPos = glm::fract(currentPos);
            
            // For edge cases, we'll check all potentially visible faces
            std::vector<glm::ivec3> potentialFaces;
            std::vector<float> faceWeights;
            
            // Check all six faces with their distances to block boundaries
            // X- face (left)
            if (fractPos.x < 0.1f) {
                potentialFaces.push_back(glm::ivec3(-1, 0, 0));
                faceWeights.push_back(fractPos.x);
            }
            
            // X+ face (right)
            if (fractPos.x > 0.9f) {
                potentialFaces.push_back(glm::ivec3(1, 0, 0));
                faceWeights.push_back(1.0f - fractPos.x);
            }
            
            // Y- face (bottom)
            if (fractPos.y < 0.1f) {
                potentialFaces.push_back(glm::ivec3(0, -1, 0));
                faceWeights.push_back(fractPos.y);
            }
            
            // Y+ face (top)
            if (fractPos.y > 0.9f) {
                potentialFaces.push_back(glm::ivec3(0, 1, 0));
                faceWeights.push_back(1.0f - fractPos.y);
            }
            
            // Z- face (back)
            if (fractPos.z < 0.1f) {
                potentialFaces.push_back(glm::ivec3(0, 0, -1));
                faceWeights.push_back(fractPos.z);
            }
            
            // Z+ face (front)
            if (fractPos.z > 0.9f) {
                potentialFaces.push_back(glm::ivec3(0, 0, 1));
                faceWeights.push_back(1.0f - fractPos.z);
            }
            
            // If we're not at an edge, use standard approach
            if (potentialFaces.empty()) {
                // Determine which face was hit by finding the component closest to an edge
                glm::ivec3 normal(0, 0, 0);
                
                // Calculate distances to block boundaries in each direction
                float distToMinX = fractPos.x;
                float distToMaxX = 1.0f - fractPos.x;
                float distToMinY = fractPos.y;
                float distToMaxY = 1.0f - fractPos.y;
                float distToMinZ = fractPos.z;
                float distToMaxZ = 1.0f - fractPos.z;
                
                // Find the closest boundary
                float minDist = distToMinX;
                normal = glm::ivec3(-1, 0, 0); // -X face
                
                if (distToMaxX < minDist) {
                    minDist = distToMaxX;
                    normal = glm::ivec3(1, 0, 0); // +X face
                }
                
                if (distToMinY < minDist) {
                    minDist = distToMinY;
                    normal = glm::ivec3(0, -1, 0); // -Y face
                }
                
                if (distToMaxY < minDist) {
                    minDist = distToMaxY;
                    normal = glm::ivec3(0, 1, 0); // +Y face
                }
                
                if (distToMinZ < minDist) {
                    minDist = distToMinZ;
                    normal = glm::ivec3(0, 0, -1); // -Z face
                }
                
                if (distToMaxZ < minDist) {
                    minDist = distToMaxZ;
                    normal = glm::ivec3(0, 0, 1); // +Z face
                }
                
                potentialFaces.push_back(normal);
            }
            
            // Check all potential faces to find a visible one
            bool found = false;
            glm::ivec3 bestNormal(0, 0, 0);
            float bestAlignment = -1.0f;
            
            for (size_t i = 0; i < potentialFaces.size(); i++) {
                glm::ivec3 normal = potentialFaces[i];
                
                // Check if the face is actually exposed (adjacent block is air)
                glm::ivec3 adjacentBlockPos = blockPos + normal;
                int adjacentBlockType = getBlock(adjacentBlockPos);
                
                // Only consider faces that are exposed to air
                if (adjacentBlockType == 0) {
                    // Calculate how directly this face is being viewed
                    // Dot product between view direction and face normal
                    float alignment = std::abs(glm::dot(glm::vec3(normal), dir));
                    
                    // If this is the most directly faced surface, or if we're at an edge with
                    // a low distance weight, choose this face
                    if (alignment > bestAlignment) {
                        bestAlignment = alignment;
                        bestNormal = normal;
                        found = true;
                    }
                }
            }
            
            // If we found a visible face
            if (found) {
                // Make sure the normal points outward (away from ray direction)
                if (glm::dot(glm::vec3(bestNormal), dir) > 0) {
                    bestNormal = -bestNormal; // Flip the normal if it's pointing in the wrong direction
                }
                
                std::cout << "Raycast - Selected face with normal (" 
                          << bestNormal.x << ", " << bestNormal.y << ", " << bestNormal.z 
                          << ") facing " << (bestNormal.x < 0 ? "-X" : 
                                           bestNormal.x > 0 ? "+X" : 
                                           bestNormal.y < 0 ? "-Y" : 
                                           bestNormal.y > 0 ? "+Y" : 
                                           bestNormal.z < 0 ? "-Z" : "+Z") << std::endl;
                
                // Fill the result
                result.hit = true;
                result.blockPos = blockPos;
                result.faceNormal = bestNormal;
                result.distance = distance;
                break;
            }
            
            // If no visible face found, continue ray marching
            currentPos += dir * stepSize;
            continue;
        }
        
        // Move along the ray
        currentPos += dir * stepSize;
    }
    
    return result;
}

bool World::checkPlayerPhysicsUpdate(const glm::vec3& playerPosition, float playerWidth, float playerHeight) {
    // If there are no recently modified blocks, no need to check
    if (m_recentlyModifiedBlocks.empty()) {
        return false;
    }
    
    // Get current time
    double currentTime = glfwGetTime();
    
    // Use player width directly rather than a fixed value
    const float collisionWidth = playerWidth * 0.9f; // Scale to match CollisionSystem
    
    // Check center point first
    glm::vec3 centerPoint = playerPosition;
    centerPoint.y -= 0.05f; // Slightly below player feet
    
    // Check if any recently modified blocks were directly below the player
    for (const auto& modifiedBlock : m_recentlyModifiedBlocks) {
        // Only consider blocks changed to air (when removed) and only for recent changes
        if (modifiedBlock.newType == 0 && currentTime - modifiedBlock.timeModified < 0.3) {
            glm::vec3 blockPos = glm::vec3(modifiedBlock.position);
            
            // Check if this block is directly below the player's center
            if (std::abs(blockPos.x - centerPoint.x) < 0.5f &&
                std::abs(blockPos.z - centerPoint.z) < 0.5f &&
                blockPos.y <= centerPoint.y && 
                blockPos.y > centerPoint.y - 0.2f) {
                
                return true; // Block directly below player was removed
            }
            
            // Check in a small circle around the player's feet (same as ground collision)
            const int numSamplePoints = 4; // Reduced from previous value
            const float radius = collisionWidth * 0.75f; // Reduced to match ground check
            
            for (int i = 0; i < numSamplePoints; i++) {
                float angle = (float)i * (2.0f * M_PI / numSamplePoints);
                float sampleX = centerPoint.x + radius * cos(angle);
                float sampleZ = centerPoint.z + radius * sin(angle);
                
                // Check if this block covers any of our sample points
                if (std::abs(blockPos.x - sampleX) < 0.5f &&
                    std::abs(blockPos.z - sampleZ) < 0.5f &&
                    blockPos.y <= centerPoint.y && 
                    blockPos.y > centerPoint.y - 0.2f) {
                    
                    return true; // Block under a sample point was removed
                }
            }
        }
    }
    
    // Clear blocks older than 1 second to manage memory
    double cutoffTime = currentTime - 1.0;
    while (!m_recentlyModifiedBlocks.empty() && m_recentlyModifiedBlocks.front().timeModified < cutoffTime) {
        m_recentlyModifiedBlocks.pop_front();
    }
    
    return false;
}

int World::getPendingChunksCount() const {
    // This is an approximation since we don't have a queue of pending chunks
    // Instead we'll estimate based on current operations and what chunks should be loaded
    
    // Get a copy of the last known pending count
    return m_pendingChunkOperations;
} 