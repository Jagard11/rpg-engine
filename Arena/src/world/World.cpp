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
    , m_maxSimultaneousChunksLoaded(0)
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
        // Reduce log spam by only showing for important chunks
        if (chunkPos.y == 0) {
            std::cout << "Loaded existing chunk from file: " << filename << std::endl;
        }
        m_chunks[chunkPos] = std::move(chunk);
    } else {
        // If file doesn't exist or can't be loaded, generate a new chunk
        generateChunk(chunkPos);
        return; // Return since generateChunk adds the chunk to m_chunks
    }
    
    // After loading/generating, mark mesh for update but don't generate immediately
    // This spreads out the expensive mesh generation over multiple frames
    Chunk* loadedChunk = m_chunks[chunkPos].get();
    if (loadedChunk) {
        loadedChunk->setDirty(true);
    }
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
            
            // Check if this is a boundary voxel that affects neighboring chunks
            bool isChunkBoundary = 
                localPos.x == 0 || localPos.x == (CHUNK_SIZE - 1) ||
                localPos.y == 0 || localPos.y == (CHUNK_HEIGHT - 1) ||
                localPos.z == 0 || localPos.z == (CHUNK_SIZE - 1);
                
            // Just mark chunks as dirty rather than immediately regenerating meshes
            chunk->setDirty(true);
            
            // If at a boundary, mark neighboring chunks as dirty too
            if (isChunkBoundary) {
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
                        neighbor->setDirty(true);
                    }
                }
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

void World::evaluateChunksNeeded(const glm::vec3& playerPos) {
    // Convert player position to chunk coordinates
    glm::ivec3 playerChunkPos = worldToChunkPos(playerPos);
    
    // Only update visibility periodically to avoid constant recalculation
    static int visibilityUpdateCounter = 0;
    if (visibilityUpdateCounter++ % 10 == 0) {  // Only update every 10 frames
        updateVisibleChunks(playerPos, glm::vec3(0, 0, 0));
    }
    
    // Clear the queues from the previous evaluation
    m_chunksToLoadQueue.clear();
    m_chunksToUnloadQueue.clear();
    
    // Temporary lists for sorting before adding to queue
    std::vector<glm::ivec3> chunksToLoadList;
    std::vector<glm::ivec3> chunksToUnloadList;
    
    // Determine chunks to load (within view distance)
    for (int x = playerChunkPos.x - m_viewDistance; x <= playerChunkPos.x + m_viewDistance; x++) {
        for (int z = playerChunkPos.z - m_viewDistance; z <= playerChunkPos.z + m_viewDistance; z++) {
            // Calculate squared distance to player chunk (in 2D for XZ plane)
            int dx = x - playerChunkPos.x;
            int dz = z - playerChunkPos.z;
            int squaredDist = dx * dx + dz * dz;
            
            if (squaredDist <= m_viewDistance * m_viewDistance) {
                // Calculate vertical range based on player height
                int minY = std::max(0, (playerChunkPos.y - 8));
                int maxY = playerChunkPos.y + 8; // Increased to match visibility
                
                // Add chunks in the vertical range
                for (int y = minY; y <= maxY; y++) {
                    glm::ivec3 chunkPos(x, y, z);
                    
                    // Only consider loading chunks that should be visible or are critical
                    if (m_chunks.find(chunkPos) == m_chunks.end() && shouldLoadChunk(chunkPos, playerChunkPos)) {
                        chunksToLoadList.push_back(chunkPos);
                    }
                }
            }
        }
    }
    
    // Sort chunks to load by distance to player (closest first)
    std::sort(chunksToLoadList.begin(), chunksToLoadList.end(), 
        [playerChunkPos](const glm::ivec3& a, const glm::ivec3& b) {
            // Calculate squared distances
            float distA = glm::length(glm::vec3(a - playerChunkPos));
            float distB = glm::length(glm::vec3(b - playerChunkPos));
            return distA < distB;
        }
    );
    
    // Add sorted list to the actual load queue
    for(const auto& pos : chunksToLoadList) {
        m_chunksToLoadQueue.push_back(pos);
    }
    
    // Determine chunks to unload (outside view distance)
    for (const auto& pair : m_chunks) {
        const glm::ivec3& chunkPos = pair.first;
        
        // Calculate squared distance from player chunk
        int dx = chunkPos.x - playerChunkPos.x;
        int dz = chunkPos.z - playerChunkPos.z;
        int squaredDist = dx * dx + dz * dz;
        
        // Keep chunks that are within an extended unload distance to prevent frequent loading/unloading
        int unloadDistance = m_viewDistance + 2; // Reduced from 4 to 2 to unload more aggressively
        bool tooFarHorizontal = squaredDist > unloadDistance * unloadDistance;
        bool tooFarVertical = chunkPos.y < (playerChunkPos.y - 10) || chunkPos.y > (playerChunkPos.y + 10);
        bool notVisible = m_visibleChunks.find(chunkPos) == m_visibleChunks.end();
        
        // Unload chunks that are too far away OR are no longer visible and not in the immediate
        // vertical vicinity of the player
        if (tooFarHorizontal || (tooFarVertical && notVisible)) {
            chunksToUnloadList.push_back(chunkPos);
        }
    }
    
    // Add list to the actual unload queue
    for(const auto& pos : chunksToUnloadList) {
        m_chunksToUnloadQueue.push_back(pos);
    }
    
    // Update the pending chunk operations count based on the load queue size
    m_pendingChunkOperations = m_chunksToLoadQueue.size();
    
    // Log the result of the evaluation (only occasionally)
    static int evalLogCounter = 0;
    if (evalLogCounter++ % 60 == 0) { // Log roughly once per second if called every frame
        std::cout << "Evaluated chunks: To Load=" << m_chunksToLoadQueue.size() 
                  << ", To Unload=" << m_chunksToUnloadQueue.size() 
                  << ", Visible=" << m_visibleChunks.size() << std::endl;
    }
}

void World::processChunkQueues() {
    // Check if we're currently loading too many chunks
    // If pending operations are too high, only process critical chunks?
    // (Let's keep the adaptive logic based on FPS for now)
    
    // Track current FPS for adaptive loading
    static float lastFrameTime = glfwGetTime();
    float currentTime = glfwGetTime();
    float deltaTime = currentTime - lastFrameTime;
    lastFrameTime = currentTime;
    
    float currentFps = 1.0f / (deltaTime > 0.001f ? deltaTime : 0.001f);
    
    // Dynamically adjust chunk processing based on FPS
    int maxChunksToLoadPerFrame = 6;
    int maxChunksToUnloadPerFrame = 3;
    
    if (currentFps < 20.0f) {
        maxChunksToLoadPerFrame = 1;
        maxChunksToUnloadPerFrame = 1;
    } else if (currentFps < 40.0f) {
        maxChunksToLoadPerFrame = 2;
        maxChunksToUnloadPerFrame = 1;
    }

    // Define critical chunks - these need to be loaded ASAP if they appear in the queue
    // Recalculate based on current player position (assuming player hasn't moved too far)
    // Ideally, Game class would pass player pos here, but using last known for now.
    glm::ivec3 playerChunkPos = worldToChunkPos(glm::vec3(0)); // Placeholder - needs actual player pos
    // This part is tricky without direct player access. A better design would pass playerPos.
    // For now, we'll rely on the load queue being sorted correctly.
    std::vector<glm::ivec3> criticalChunks; // Placeholder, not strictly enforced here now.
    
    // Process loading queue
    int chunksLoaded = 0;
    while (!m_chunksToLoadQueue.empty() && chunksLoaded < maxChunksToLoadPerFrame) {
        glm::ivec3 chunkPos = m_chunksToLoadQueue.front();
        m_chunksToLoadQueue.pop_front();
        
        // Ensure we don't accidentally re-load a chunk
        if (m_chunks.find(chunkPos) == m_chunks.end()) {
            loadChunk(chunkPos);
            chunksLoaded++;
        }
    }
    
    // Process unloading queue (only if loading is not maxed out)
    int chunksUnloaded = 0;
    if (chunksLoaded < maxChunksToLoadPerFrame) {
        while (!m_chunksToUnloadQueue.empty() && chunksUnloaded < maxChunksToUnloadPerFrame) {
            glm::ivec3 chunkPos = m_chunksToUnloadQueue.front();
            m_chunksToUnloadQueue.pop_front();
            
             // Ensure we don't unload critical chunks? (Sort of handled by shouldLoadChunk in evaluation)
            // Ensure chunk still exists before unloading
            if (m_chunks.find(chunkPos) != m_chunks.end()) {
                 // Save modified chunks before unloading
                 Chunk* chunk = getChunkAt(chunkPos);
                 if (chunk && chunk->isModified()) {
                      std::string filename = "chunks/" + std::to_string(chunkPos.x) + "_" + 
                                            std::to_string(chunkPos.y) + "_" + 
                                            std::to_string(chunkPos.z) + ".chunk";
                      std::filesystem::create_directories("chunks");
                      chunk->serialize(filename);
                 }
                
                 unloadChunk(chunkPos);
                 chunksUnloaded++;
            }
        }
    }
    
    // Update the final pending chunk operations count
    m_pendingChunkOperations = m_chunksToLoadQueue.size();
    
    // Log processing results (only occasionally)
    static int procLogCounter = 0;
    if ((chunksLoaded > 0 || chunksUnloaded > 0) && procLogCounter++ % 30 == 0) {
         std::cout << "Processed chunks: loaded " << chunksLoaded 
                  << ", unloaded " << chunksUnloaded
                  << ", total active: " << m_chunks.size() 
                  << ", pending load: " << m_pendingChunkOperations 
                  << ", pending unload: " << m_chunksToUnloadQueue.size()
                  << ", FPS: " << currentFps << std::endl;
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
    // Get the chunk
    Chunk* chunk = getChunkAt(chunkPos);
    if (!chunk) {
        return;
    }
    
    // Instead of updating all adjacent chunks at once, just update this chunk
    // Adjacent chunks will be updated when they're detected as dirty in the frame loop
    std::cout << "Generating mesh for chunk " << chunkPos.x << "," << chunkPos.y << "," << chunkPos.z 
              << " with greedy meshing " << (disableGreedyMeshing ? "DISABLED" : "ENABLED") << std::endl;
    chunk->generateMesh(disableGreedyMeshing);
    chunk->setDirty(false);
    
    // Mark adjacent chunks as dirty, but don't generate their meshes yet
    // This allows the mesh generation to be spread across multiple frames
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
            neighbor->setDirty(true);
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

bool World::isChunkVisible(const glm::ivec3& chunkPos, const glm::vec3& playerPos, const glm::vec3& playerForward) const {
    // Check if chunk is in the visible set
    return m_visibleChunks.find(chunkPos) != m_visibleChunks.end();
}

bool World::isVisibleFromAbove(const glm::ivec3& chunkPos, const glm::ivec3& playerChunkPos) const {
    // A chunk is visible from above if it's the highest chunk at this X,Z position
    // or if it's the chunk directly below a visible one
    
    // Always consider chunks at or above player level visible
    if (chunkPos.y >= playerChunkPos.y) {
        return true;
    }
    
    // Calculate horizontal distance (in chunks) from player
    int dx = std::abs(chunkPos.x - playerChunkPos.x);
    int dz = std::abs(chunkPos.z - playerChunkPos.z);
    
    // Use squared distance for comparison with view distance
    int squaredDist = dx * dx + dz * dz;
    
    // Only check chunks within horizontal view distance
    if (squaredDist > m_viewDistance * m_viewDistance) {
        return false;
    }
    
    // Check if this is the highest non-empty chunk at this X,Z position
    // or if it's the chunk directly below the highest non-empty chunk
    
    // Find the highest non-empty chunk at this X,Z position
    int highestY = chunkPos.y;
    bool foundHigherChunk = false;
    
    // Look from maximum height down
    const int MAX_HEIGHT = 128; // Some reasonable maximum height
    for (int y = MAX_HEIGHT; y > chunkPos.y; y--) {
        glm::ivec3 higherPos(chunkPos.x, y, chunkPos.z);
        auto it = m_chunks.find(higherPos);
        if (it != m_chunks.end()) {
            // Found a higher chunk, check if it's not empty
            Chunk* chunk = it->second.get();
            if (chunk && !chunk->isEmpty()) {
                foundHigherChunk = true;
                highestY = y;
                break;
            }
        }
    }
    
    // If no higher non-empty chunk was found, this is the highest one
    if (!foundHigherChunk) {
        return true;
    }
    
    // Also make visible the chunk directly below the surface
    if (chunkPos.y == highestY - 1) {
        return true;
    }
    
    // For very steep terrain, use a ratio check (1:4 ratio to catch very steep areas)
    int verticalDistance = playerChunkPos.y - chunkPos.y;
    int horizontalDistance = std::max(dx, dz); // Use max distance for steepness check
    
    return horizontalDistance * 4 <= verticalDistance;
}

void World::markChunkVisible(const glm::ivec3& chunkPos) {
    // Add chunk to visible set if not already present
    if (m_visibleChunks.insert(chunkPos).second) {
        // If successfully inserted (meaning it wasn't already there)
        // Propagate visibility downward
        propagateVisibilityDownward(chunkPos);
    }
}

void World::propagateVisibilityDownward(const glm::ivec3& chunkPos) {
    // Mark the chunk directly below as visible
    glm::ivec3 chunkBelow(chunkPos.x, chunkPos.y - 1, chunkPos.z);
    
    // Always show at least one chunk below the surface
    if (chunkBelow.y >= 0) {
        m_visibleChunks.insert(chunkBelow);
    }
}

void World::updateVisibleChunks(const glm::vec3& playerPos, const glm::vec3& playerForward) {
    // Clear previous visibility data
    m_visibleChunks.clear();
    
    // Get player's chunk position
    glm::ivec3 playerChunkPos = worldToChunkPos(playerPos);
    
    // Debug counters
    int surfaceChunks = 0;
    int deepChunks = 0;
    int totalChunks = 0;
    
    // Limit surface detection to a reduced distance for performance
    int surfaceDetectionDistance = std::min(m_viewDistance, 6);
    
    // First pass: Mark all terrain surface chunks as visible
    for (int x = playerChunkPos.x - surfaceDetectionDistance; x <= playerChunkPos.x + surfaceDetectionDistance; x++) {
        for (int z = playerChunkPos.z - surfaceDetectionDistance; z <= playerChunkPos.z + surfaceDetectionDistance; z++) {
            // Check horizontal distance (in 2D for XZ plane)
            int dx = x - playerChunkPos.x;
            int dz = z - playerChunkPos.z;
            int squaredDist = dx * dx + dz * dz;
            
            if (squaredDist <= surfaceDetectionDistance * surfaceDetectionDistance) {
                // First, find the highest terrain chunk at this x,z coordinate
                int highestY = -1;
                
                // Look through loaded chunks to find the highest non-empty one
                for (int y = playerChunkPos.y + 16; y >= 0; y--) {
                    glm::ivec3 checkPos(x, y, z);
                    auto it = m_chunks.find(checkPos);
                    if (it != m_chunks.end() && !it->second->isEmpty()) {
                        highestY = y;
                        break;
                    }
                }
                
                // If we found a highest chunk, mark it and the one below it as visible
                if (highestY >= 0) {
                    m_visibleChunks.insert(glm::ivec3(x, highestY, z));
                    surfaceChunks++;
                    if (highestY > 0) {
                        m_visibleChunks.insert(glm::ivec3(x, highestY - 1, z));
                        surfaceChunks++;
                    }
                }
                
                // Also mark chunks near the player as visible for a vertical range
                int minY = std::max(0, playerChunkPos.y - 6); // Reduced from 8
                int maxY = playerChunkPos.y + 6; // Reduced from 8
                
                // For chunks closer to the player, use a more generous vertical range
                if (std::max(std::abs(dx), std::abs(dz)) <= 2) { // Reduced from 3
                    for (int y = minY; y <= maxY; y++) {
                        m_visibleChunks.insert(glm::ivec3(x, y, z));
                        deepChunks++;
                    }
                }
                // For more distant chunks, check using the visibility from above criteria
                else {
                    for (int y = minY; y <= maxY; y++) {
                        glm::ivec3 checkChunkPos(x, y, z);
                        if (isVisibleFromAbove(checkChunkPos, playerChunkPos)) {
                            markChunkVisible(checkChunkPos);
                            deepChunks++;
                        }
                    }
                }
            }
        }
    }
    
    // Second pass: Use a more limited propagation
    // Make a copy to avoid modifying while iterating
    auto visibleChunksCopy = m_visibleChunks;
    
    // Propagate visibility downward from all visible chunks
    for (const auto& chunkPos : visibleChunksCopy) {
        // Only propagate if we're within a close horizontal distance to the player
        int dx = std::abs(chunkPos.x - playerChunkPos.x);
        int dz = std::abs(chunkPos.z - playerChunkPos.z);
        if (dx <= 3 && dz <= 3) { // Only propagate for chunks close to player
            propagateVisibilityDownward(chunkPos);
        }
    }
    
    totalChunks = m_visibleChunks.size();
    
    // Only log occasionally to reduce spam
    static int debugCounter = 0;
    if (debugCounter++ % 100 == 0) {
        std::cout << "Chunk visibility: " << totalChunks << " total visible chunks ("
                  << surfaceChunks << " surface, " 
                  << deepChunks << " by depth criteria, "
                  << (totalChunks - surfaceChunks - deepChunks) << " from propagation)" << std::endl;
    }
}

bool World::shouldLoadChunk(const glm::ivec3& chunkPos, const glm::ivec3& playerChunkPos) const {
    // Critical chunks around player - always load
    if (std::abs(chunkPos.x - playerChunkPos.x) <= 1 &&
        std::abs(chunkPos.y - playerChunkPos.y) <= 1 &&
        std::abs(chunkPos.z - playerChunkPos.z) <= 1) {
        return true;
    }
    
    // Calculate horizontal distance
    int dx = std::abs(chunkPos.x - playerChunkPos.x);
    int dz = std::abs(chunkPos.z - playerChunkPos.z);
    int squaredDist = dx * dx + dz * dz;
    
    // Within render distance horizontally
    if (squaredDist <= m_viewDistance * m_viewDistance) {
        // Always load all chunks at the surface level for all X,Z positions within render distance
        
        // Check if this is potentially a surface chunk (at or above player height)
        if (chunkPos.y >= playerChunkPos.y) {
            return true;
        }
        
        // Always load at least 4 chunks below the player for any X,Z within render distance
        if (chunkPos.y >= playerChunkPos.y - 4 && chunkPos.y < playerChunkPos.y) {
            return true;
        }
        
        // For very steep terrain, use our visibility criteria
        if (chunkPos.y < playerChunkPos.y) {
            int verticalDist = playerChunkPos.y - chunkPos.y;
            int horizontalDist = std::max(dx, dz);
            
            // Check using the steep terrain criteria (1:4 ratio)
            if (horizontalDist * 4 <= verticalDist && verticalDist <= 20) {
                return true;
            }
        }
    }
    
    // Also load any chunk marked as visible
    return isChunkVisible(chunkPos, glm::vec3(0), glm::vec3(0));
}

int World::getDirtyChunkCount() const {
    int count = 0;
    for (const auto& pair : m_chunks) {
        if (pair.second->isDirty()) {
            count++;
        }
    }
    return count;
}

// Add a method to update a limited number of dirty chunk meshes per frame
void World::updateDirtyChunkMeshes(int maxUpdatesPerFrame) {
    // Find dirty chunks that need mesh updates
    std::vector<glm::ivec3> dirtyChunks;
    
    for (const auto& pair : m_chunks) {
        if (pair.second->isDirty()) {
            dirtyChunks.push_back(pair.first);
        }
    }
    
    // Sort by distance to 0,0,0 (arbitrary but consistent ordering)
    std::sort(dirtyChunks.begin(), dirtyChunks.end(), 
        [](const glm::ivec3& a, const glm::ivec3& b) {
            return glm::length(glm::vec3(a)) < glm::length(glm::vec3(b));
        }
    );
    
    // Update a limited number of meshes
    int updatesThisFrame = 0;
    for (const auto& chunkPos : dirtyChunks) {
        if (updatesThisFrame >= maxUpdatesPerFrame) {
            break;
        }
        
        Chunk* chunk = getChunkAt(chunkPos);
        if (chunk && chunk->isDirty()) {
            chunk->generateMesh(m_disableGreedyMeshing);
            chunk->setDirty(false);
            updatesThisFrame++;
        }
    }
    
    if (updatesThisFrame > 0) {
        std::cout << "Updated " << updatesThisFrame << " dirty chunk meshes, " 
                  << (dirtyChunks.size() - updatesThisFrame) << " remaining." << std::endl;
    }
} 