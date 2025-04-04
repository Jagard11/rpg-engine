#include "world/World.hpp"
#include <fstream>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <GLFW/glfw3.h>
#include "player/Player.hpp"
#include "world/ChunkVisibilityManager.hpp"
#include <set>

World::World(uint64_t seed)
    : m_seed(seed)
    , m_viewDistance(8)
    , m_disableGreedyMeshing(false)  // Enable greedy meshing by default
    , m_pendingChunkOperations(0)
    , m_maxSimultaneousChunksLoaded(0)
    , m_initialized(false)
    , m_maxVisibleChunks(1000)  // Set a reasonable limit for visible chunks
{
    m_worldGenerator = std::make_unique<WorldGenerator>(seed);
    m_visibilityManager = std::make_unique<ChunkVisibilityManager>(this);
    std::cout << "World created with seed: " << seed << std::endl;
}

World::~World() {
}

void World::initialize() {
    std::cout << "Initializing world state..." << std::endl;
    m_initialized = true;
    std::cout << "World state initialized." << std::endl;
}

// Updated method to generate the initial chunks for a new game
void World::generateInitialArea(const glm::vec3& spawnPosition) {
    if (!m_initialized) {
        std::cerr << "ERROR: Cannot generate initial area before world is initialized." << std::endl;
        return;
    }

    std::cout << "Generating initial chunk area around spawn point: (" 
              << spawnPosition.x << ", " << spawnPosition.y << ", " << spawnPosition.z << ")" << std::endl;

    glm::ivec3 spawnChunkPos = worldToChunkPos(spawnPosition);
    int chunkCount = 0;

    // Use the EXACT 16x16 grid pattern around spawn
    int minX = spawnChunkPos.x - 8; 
    int maxX = spawnChunkPos.x + 7;
    int minZ = spawnChunkPos.z - 7; 
    int maxZ = spawnChunkPos.z + 8;
    
    // Start by generating just the top layer of chunks plus one below
    int topY = spawnChunkPos.y;
    int startY = std::max(0, topY - 1); 
    int endY = topY;
    
    // Always include ground level chunks
    bool includeGroundLevel = startY > 0;
    int groundY = 0;

    std::cout << "Generating initial chunk data (16x16 grid, Y=" << startY << " to " << endY;
    if (includeGroundLevel) {
        std::cout << " plus ground level";
    }
    std::cout << ")" << std::endl;
    
    // Calculate how many chunks we'll generate
    int xCount = maxX - minX + 1;
    int zCount = maxZ - minZ + 1;
    int yCount = endY - startY + 1 + (includeGroundLevel ? 1 : 0);
    int totalExpected = xCount * zCount * yCount;
    
    std::cout << "Expected chunk count: " << totalExpected 
              << " (" << xCount << "x" << zCount << "x" << yCount << ")" << std::endl;

    // First pass - generate all chunks and calculate exposure
    // Only mark as dirty if exposed (needing a mesh)
    std::vector<glm::ivec3> exposedChunks;

    // Generate all chunks and calculate exposure during generation
    for (int x = minX; x <= maxX; x++) {
        for (int z = minZ; z <= maxZ; z++) {
            // Generate the top level chunks
            for (int y = startY; y <= endY; y++) { 
                glm::ivec3 chunkPos(x, y, z);
                
                // Skip if chunk somehow already exists (shouldn't happen here)
                if (m_chunks.count(chunkPos)) continue;

                // Create Chunk and populate data
                auto chunk = std::make_unique<Chunk>(chunkPos.x, chunkPos.y, chunkPos.z);
                chunk->setWorld(this);

                // Generate terrain data 
                for (int lx = 0; lx < CHUNK_SIZE; lx++) {
                    for (int lz = 0; lz < CHUNK_SIZE; lz++) {
                        int worldX = chunkPos.x * CHUNK_SIZE + lx;
                        int worldZ = chunkPos.z * CHUNK_SIZE + lz;
                        int totalHeight = m_worldGenerator->getHeight(worldX, worldZ);
                        int chunkMinY = chunkPos.y * CHUNK_HEIGHT;
                        
                        for (int ly = 0; ly < CHUNK_HEIGHT; ly++) {
                            int worldY = chunkMinY + ly;
                            if (worldY < totalHeight) {
                                int blockType = m_worldGenerator->getBlockType(worldX, worldY, worldZ);
                                if (blockType > 0) {
                                    chunk->setBlock(lx, ly, lz, blockType);
                                }
                            }
                        }
                    }
                }

                // Calculate exposure mask once after terrain generation
                chunk->calculateExposureMask();
                
                // Track if this chunk will need a mesh
                bool needsMesh = chunk->isExposed();
                
                // Set initial states - only mark dirty if exposed
                chunk->setDirty(needsMesh);
                chunk->setModified(false);
                
                // Add to world
                m_chunks[chunkPos] = std::move(chunk);
                
                // Update column metadata based on exposure
                updateColumnMetadata(chunkPos);
                
                // Track exposed chunks for second pass
                if (needsMesh) {
                    exposedChunks.push_back(chunkPos);
                }
                
                chunkCount++;
            }
            
            // Optionally generate ground level chunks
            if (includeGroundLevel) {
                glm::ivec3 groundChunkPos(x, groundY, z);
                
                // Skip if we've already generated this chunk
                if (m_chunks.count(groundChunkPos)) continue;
                
                auto chunk = std::make_unique<Chunk>(groundChunkPos.x, groundChunkPos.y, groundChunkPos.z);
                chunk->setWorld(this);
                
                // Generate terrain data for ground level
                for (int lx = 0; lx < CHUNK_SIZE; lx++) {
                    for (int lz = 0; lz < CHUNK_SIZE; lz++) {
                        int worldX = groundChunkPos.x * CHUNK_SIZE + lx;
                        int worldZ = groundChunkPos.z * CHUNK_SIZE + lz;
                        int totalHeight = m_worldGenerator->getHeight(worldX, worldZ);
                        
                        for (int ly = 0; ly < CHUNK_HEIGHT; ly++) {
                            int worldY = groundY * CHUNK_HEIGHT + ly;
                            if (worldY < totalHeight) {
                                int blockType = m_worldGenerator->getBlockType(worldX, worldY, worldZ);
                                if (blockType > 0) {
                                    chunk->setBlock(lx, ly, lz, blockType);
                                }
                            }
                        }
                    }
                }
                
                // Calculate exposure mask once after terrain generation
                chunk->calculateExposureMask();
                
                // Only mark dirty if it needs a mesh (exposed)
                bool needsMesh = chunk->isExposed();
                chunk->setDirty(needsMesh);
                chunk->setModified(false);
                m_chunks[groundChunkPos] = std::move(chunk);
                
                // Update column metadata based on exposure
                updateColumnMetadata(groundChunkPos);
                
                // Track for second pass
                if (needsMesh) {
                    exposedChunks.push_back(groundChunkPos);
                }
                
                chunkCount++;
            }

            // Occasional logging to show progress
            if ((x % 4 == 0) && (z % 4 == 0)) {
                 std::cout << "Populated initial chunk data at XZ position (" << x << ", " << z 
                          << "), " << chunkCount << "/" << totalExpected << " chunks completed (" 
                          << (int)(100.0f * chunkCount / totalExpected) << "%)" << std::endl;
            }
        }
    }

    // Update chunk visibility (this is relatively cheap)
    if (m_visibilityManager) {
        Player tempPlayer; 
        glm::vec3 visibilityCheckPos = spawnPosition;
        if (visibilityCheckPos.y < 10.0f) visibilityCheckPos.y = 10.0f; 
        tempPlayer.setPosition(visibilityCheckPos);
        m_lastPlayerPosition = spawnPosition; 
        
        // Add only exposed chunks to the visible set
        for (const auto& chunkPos : exposedChunks) {
            m_visibleChunks.insert(chunkPos);
        }
        
        std::cout << "Initial chunk visibility updated for spawn point." << std::endl;
    }

    std::cout << "Initial area data generation complete. Created " << chunkCount 
              << " chunks, " << exposedChunks.size() << " will require mesh generation." << std::endl;
}

// Modified generateChunk: Only marks chunks dirty if they're in player view area
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
    bool chunkIsEmpty = true;
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            int worldX = chunkPos.x * CHUNK_SIZE + x;
            int worldZ = chunkPos.z * CHUNK_SIZE + z;
            
            // Get height at this position
            int totalHeight = m_worldGenerator->getHeight(worldX, worldZ);
            
            // Calculate range for this chunk
            int chunkMinY = chunkPos.y * CHUNK_HEIGHT;
            
            // Fill blocks within this chunk's y-range
            for (int localY = 0; localY < CHUNK_HEIGHT; localY++) {
                int worldY = chunkMinY + localY;
                
                if (worldY < totalHeight) {
                    int blockType = m_worldGenerator->getBlockType(worldX, worldY, worldZ);
                    if (blockType > 0) {
                        chunk->setBlock(x, localY, z, blockType);
                        chunkIsEmpty = false;
                    }
                }
            }
        }
    }
    
    // Mark this as a procedurally generated chunk, not modified by the player
    chunk->setModified(false);
    
    // Calculate exposure mask for the new chunk
    chunk->calculateExposureMask();
    
    // Store the chunk
    m_chunks[chunkPos] = std::move(chunk);
    
    // Update column metadata for exposure-based loading if the chunk is exposed
    Chunk* generatedChunk = m_chunks[chunkPos].get();
    if (generatedChunk && generatedChunk->isExposed() && !chunkIsEmpty) {
        updateColumnMetadata(chunkPos);
    }
    
    // Only mark as dirty if it's near the player and exposed (needs mesh)
    glm::ivec3 playerChunkPos = worldToChunkPos(m_lastPlayerPosition);
    bool inPlayerGrid = (chunkPos.x >= playerChunkPos.x - 8 && chunkPos.x <= playerChunkPos.x + 7 && 
                       chunkPos.z >= playerChunkPos.z - 7 && chunkPos.z <= playerChunkPos.z + 8);
    bool nearPlayerHeight = std::abs(chunkPos.y - playerChunkPos.y) <= 2;
    
    // Only mark as dirty if it's in the player's view area AND exposed
    if (generatedChunk && inPlayerGrid && nearPlayerHeight && generatedChunk->isExposed()) {
        generatedChunk->setDirty(true);
    }
}

// Modified loadChunk: Only marks chunks dirty if they're in view or being directly accessed
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
        
        // Ensure the exposure mask is calculated accurately
        chunk->calculateExposureMask();
        
        // Add chunk to the world
        m_chunks[chunkPos] = std::move(chunk);
        
        // Update column metadata only if this chunk is exposed
        Chunk* loadedChunk = m_chunks[chunkPos].get();
        if (loadedChunk && loadedChunk->isExposed()) {
            updateColumnMetadata(chunkPos);
        }
        
        // Only mark as dirty if the chunk is in the player's view area
        // This prevents excessive mesh generation on initial load
        glm::ivec3 playerChunkPos = worldToChunkPos(m_lastPlayerPosition);
        bool inPlayerGrid = (chunkPos.x >= playerChunkPos.x - 8 && chunkPos.x <= playerChunkPos.x + 7 && 
                           chunkPos.z >= playerChunkPos.z - 7 && chunkPos.z <= playerChunkPos.z + 8);
        
        // Also consider if it's within the player's height range
        bool nearPlayerHeight = std::abs(chunkPos.y - playerChunkPos.y) <= 2;
        
        // Only mark initially as dirty if it's both in grid AND near player height AND exposed
        if (inPlayerGrid && nearPlayerHeight && loadedChunk->isExposed()) {
            loadedChunk->setDirty(true);
        }
    } else {
        // If file doesn't exist or can't be loaded, generate a new chunk
        generateChunk(chunkPos);
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
            
            // Get original exposure state before modification
            bool wasExposed = chunk->isExposed();
            ExposureMask oldMask = chunk->getExposureMask();
            
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
                
            // Mark this chunk as dirty for mesh update
            chunk->setDirty(true);
            
            // Update exposure and column metadata - this is a more efficient implementation
            // that only recalculates what's necessary
            updateExposureOnBlockChange(worldPos);
            
            // If at a boundary, check if exposure changed and only mark affected adjacent chunks
            if (isChunkBoundary) {
                // Calculate new exposure
                bool isNowExposed = chunk->isExposed();
                ExposureMask newMask = chunk->getExposureMask();
                
                // Determine which faces (if any) changed exposure
                bool exposureChanged = (wasExposed != isNowExposed) || 
                    (oldMask.countExposedFaces() != newMask.countExposedFaces());
                
                // Only mark neighbors as dirty if:
                // 1. The block was at a chunk boundary
                // 2. The block change affected visibility (e.g., solid→air or air→solid)
                // 3. The exposure mask changed for the affected face
                if (exposureChanged) {
                    // Determine which adjacent chunks to update based on which boundary
                    std::vector<glm::ivec3> chunksToUpdate;
                    
                    if (localPos.x == 0) 
                        chunksToUpdate.push_back(glm::ivec3(chunkPos.x - 1, chunkPos.y, chunkPos.z));
                    else if (localPos.x == (CHUNK_SIZE - 1)) 
                        chunksToUpdate.push_back(glm::ivec3(chunkPos.x + 1, chunkPos.y, chunkPos.z));
                    
                    if (localPos.y == 0) 
                        chunksToUpdate.push_back(glm::ivec3(chunkPos.x, chunkPos.y - 1, chunkPos.z));
                    else if (localPos.y == (CHUNK_HEIGHT - 1)) 
                        chunksToUpdate.push_back(glm::ivec3(chunkPos.x, chunkPos.y + 1, chunkPos.z));
                    
                    if (localPos.z == 0) 
                        chunksToUpdate.push_back(glm::ivec3(chunkPos.x, chunkPos.y, chunkPos.z - 1));
                    else if (localPos.z == (CHUNK_SIZE - 1)) 
                        chunksToUpdate.push_back(glm::ivec3(chunkPos.x, chunkPos.y, chunkPos.z + 1));
                    
                    // Only mark affected neighbors as dirty
                    for (const auto& neighborPos : chunksToUpdate) {
                    Chunk* neighbor = getChunkAt(neighborPos);
                    if (neighbor) {
                        neighbor->setDirty(true);
                        }
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
    
    // Clear and log the visible chunks
    size_t oldVisibleSize = m_visibleChunks.size();
    m_visibleChunks.clear();
    std::cout << "Reset visible chunks list. Removed " << oldVisibleSize << " entries." << std::endl;
    
    m_chunksToLoadQueue.clear();
    m_chunksToUnloadQueue.clear();
    m_recentlyModifiedBlocks.clear();
    
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
                // Generate if chunk file doesn't exist (should ideally not happen often)
                std::cerr << "Warning: Chunk file missing for position (" 
                          << pos.x << "," << pos.y << "," << pos.z 
                          << "), generating instead." << std::endl;
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
    
    m_initialized = true; // Set initialized flag after loading
    
    // Update visibility for the loaded world (find a central point or use last player pos)
    // For now, let's assume the player position will be set separately after loading
    
    return true;
}

void World::evaluateChunksNeeded(const Player& player) {
    // Simply call the position-based version with the player's position
    glm::vec3 playerPos = player.getPosition();
    evaluateChunksNeeded(playerPos);
}

void World::processChunkQueues() {
    // Only process if the world is initialized
    if (!m_initialized) {
        return;
    }
    
    // Get current time for timeout tracking
    double startProcessingTime = glfwGetTime();
    const double MAX_PROCESSING_TIME = 0.016; // 16ms max (~1 frame at 60 FPS)
    
    // Get player chunk position for distance-based checks
    glm::ivec3 playerChunkPos = worldToChunkPos(m_lastPlayerPosition);
    
    // Track if player has moved since last frame
    static glm::ivec3 lastPlayerPosForQueue = playerChunkPos;
    bool playerMoved = (lastPlayerPosForQueue != playerChunkPos);
    lastPlayerPosForQueue = playerChunkPos;
    
    // Define the 16x16 grid boundaries around the player
    int minX = playerChunkPos.x - 8;
    int maxX = playerChunkPos.x + 7;
    int minZ = playerChunkPos.z - 7;
    int maxZ = playerChunkPos.z + 8;
    
    // Process the queue of chunks to load
    const int maxChunksToLoadPerFrame = 2;  // Limit to maintain frame rate
    const int maxChunksToUnloadPerFrame = 5; // Can unload more as it's less intensive
    
    int chunksLoaded = 0;
    
    // First, process loading queue up to the maximum limit
    while (!m_chunksToLoadQueue.empty() && chunksLoaded < maxChunksToLoadPerFrame) {
        // Check timeout - if we're taking too long, pause until next frame
        if (glfwGetTime() - startProcessingTime > MAX_PROCESSING_TIME) {
            break;
        }
        
        // Get the next chunk to load
        glm::ivec3 chunkPos = m_chunksToLoadQueue.front();
        m_chunksToLoadQueue.pop_front();
        
        // Skip if the chunk already exists
        if (m_chunks.find(chunkPos) != m_chunks.end()) {
            continue;
        }
        
        try {
            // Check again if this chunk should be loaded (player may have moved)
            bool isInGrid = (chunkPos.x >= minX && chunkPos.x <= maxX &&
                            chunkPos.z >= minZ && chunkPos.z <= maxZ);
            
            bool shouldLoad = isInGrid;
            
            // For chunks outside the grid, only load if they're exposed or adjacent
            if (!shouldLoad) {
                shouldLoad = shouldLoadBasedOnExposure(chunkPos);
            }
            
            if (shouldLoad) {
                // Load or generate the chunk
                loadChunk(chunkPos);
                chunksLoaded++;
                
                // Only mark adjacent chunks as dirty if the player has moved
                // This prevents cascading mesh regeneration when standing still
                if (playerMoved) {
                    // Mark adjacent chunks as dirty if they're already loaded
                    // This ensures proper meshing at chunk boundaries
                    const glm::ivec3 adjacentOffsets[6] = {
                        glm::ivec3(1, 0, 0),  // +X
                        glm::ivec3(-1, 0, 0), // -X
                        glm::ivec3(0, 1, 0),  // +Y
                        glm::ivec3(0, -1, 0), // -Y
                        glm::ivec3(0, 0, 1),  // +Z
                        glm::ivec3(0, 0, -1)  // -Z
                    };
                    
                    // Only mark adjacent chunks as dirty if they exist, are exposed, and are visible
                    for (int i = 0; i < 6; i++) {
                        glm::ivec3 adjacentPos = chunkPos + adjacentOffsets[i];
                        auto it = m_chunks.find(adjacentPos);
                        if (it != m_chunks.end() && it->second->isExposed() && 
                            m_visibleChunks.count(adjacentPos) > 0) {
                            it->second->setDirty(true);
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "ERROR loading chunk at [" << chunkPos.x << "," 
                     << chunkPos.y << "," << chunkPos.z << "]: " << e.what() << std::endl;
        }
    }
    
    // If still processing loading, skip unloading for this frame
    if (!m_chunksToLoadQueue.empty() || chunksLoaded >= maxChunksToLoadPerFrame) {
        // Update the pending chunk operations count
        m_pendingChunkOperations = m_chunksToLoadQueue.size();
        return;
    }
    
    // Process unloading queue (only if loading is complete)
    int chunksUnloaded = 0;
    while (!m_chunksToUnloadQueue.empty() && chunksUnloaded < maxChunksToUnloadPerFrame) {
        // Check timeout
        if (glfwGetTime() - startProcessingTime > MAX_PROCESSING_TIME) {
            break;
        }
        
        glm::ivec3 chunkPos = m_chunksToUnloadQueue.front();
        m_chunksToUnloadQueue.pop_front();
        
        // Skip chunks in the 16x16 grid around player
        bool withinGrid = (chunkPos.x >= minX && chunkPos.x <= maxX &&
                           chunkPos.z >= minZ && chunkPos.z <= maxZ);
        if (withinGrid) {
            continue;
        }
        
        // Recheck if this chunk should be kept loaded based on exposure
        bool keepLoaded = false;
        
        // Don't unload chunks that are:
        // 1. Visible within the view frustum
        if (m_visibleChunks.count(chunkPos) > 0) {
            keepLoaded = true;
        }
        // 2. Exposed to air or water
        else if (isChunkExposed(chunkPos)) {
            keepLoaded = true;
        }
        // 3. Adjacent to an exposed chunk
        else if (isAdjacentToExposedChunk(chunkPos)) {
            keepLoaded = true;
        }
        // 4. At ground level (important for world continuity)
        else if (chunkPos.y < 4) {
            keepLoaded = true;
        }
        
        // If we need to keep this chunk loaded, skip unloading
        if (keepLoaded) {
            continue;
        }
        
        // Only unload chunks that actually exist
        auto it = m_chunks.find(chunkPos);
        if (it != m_chunks.end()) {
            try {
                // Only save modified chunks
                if (it->second->isModified()) {
                    std::string filename = "chunks/" + std::to_string(chunkPos.x) + "_" + 
                                         std::to_string(chunkPos.y) + "_" + 
                                         std::to_string(chunkPos.z) + ".chunk";
                    std::filesystem::create_directories("chunks");
                    it->second->serialize(filename);
                }
                
                // Unload the chunk
                m_chunks.erase(it);
                chunksUnloaded++;
            } catch (const std::exception& e) {
                std::cerr << "ERROR unloading chunk at [" << chunkPos.x << "," 
                         << chunkPos.y << "," << chunkPos.z << "]: " << e.what() << std::endl;
            }
        }
    }
    
    // Update the pending chunk operations count
    m_pendingChunkOperations = m_chunksToLoadQueue.size();
    
    // Occasional logging
    static int processLogCounter = 0;
    if (processLogCounter++ % 100 == 0 && (chunksLoaded > 0 || chunksUnloaded > 0)) {
        std::cout << "Processed chunks: " << chunksLoaded << " loaded, " 
                 << chunksUnloaded << " unloaded. Remaining queues: " 
                 << m_chunksToLoadQueue.size() << " to load, " 
                 << m_chunksToUnloadQueue.size() << " to unload" << std::endl;
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
    
    // Check if within the 16x16 grid boundaries 
    bool withinXBounds = (chunkPos.x >= playerChunkPos.x - 8 && chunkPos.x <= playerChunkPos.x + 7);
    bool withinZBounds = (chunkPos.z >= playerChunkPos.z - 7 && chunkPos.z <= playerChunkPos.z + 8);
    
    // Only check chunks within horizontal grid
    if (!withinXBounds || !withinZBounds) {
        return false;
    }
    
    // Calculate horizontal distance components (needed for both checks)
    int dx = std::abs(chunkPos.x - playerChunkPos.x);
    int dz = std::abs(chunkPos.z - playerChunkPos.z);
    
    // CRITICAL: Always consider chunks directly below player visible
    // This ensures chunks below are rendered when flying upward
    // Look at a vertical column below the player with some width
    int horizontalDistance = std::max(dx, dz);
    
    // If chunk is within a small horizontal distance and below player, make it visible
    // This ensures a column of chunks below the player is always visible
    if (horizontalDistance <= 2 && chunkPos.y < playerChunkPos.y) {
        return true;
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
    
    // Reuse the horizontalDistance calculated earlier
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
    // Mark only one chunk directly below as visible
    glm::ivec3 chunkBelow(chunkPos.x, chunkPos.y - 1, chunkPos.z);
    
    // Only if it's above ground level
    if (chunkBelow.y >= 0) {
        m_visibleChunks.insert(chunkBelow);
    }
}

void World::updateVisibleChunks(const glm::vec3& playerPos, const glm::vec3& playerForward) {
    try {
        // Get current time to prevent too-frequent visibility updates
        static float lastVisibilityUpdateTime = 0.0f;
        float currentTime = glfwGetTime();
        
        // Call periodic reset if it's been a while
        resetChunkStates();
        
        // Only update visibility at most once per second
        if (currentTime - lastVisibilityUpdateTime < 1.0f) {
            return;
        }
        lastVisibilityUpdateTime = currentTime;
        
        std::cout << "*** UPDATING CHUNK VISIBILITY ***" << std::endl;
        
        // Get player's chunk position
        glm::ivec3 playerChunkPos = worldToChunkPos(playerPos);
        
        // Define the STRICT 16x16 grid boundaries
        int minX = playerChunkPos.x - 8;
        int maxX = playerChunkPos.x + 7;
        int minZ = playerChunkPos.z - 7;
        int maxZ = playerChunkPos.z + 8;
        
        // Track which chunks should be visible (new visibility set)
        std::unordered_set<glm::ivec3, Vec3Hash> newVisibleChunks;
        
        // First stage: Add all chunks in the 16x16 grid that have exposed faces
        for (int x = minX; x <= maxX; ++x) {
            for (int z = minZ; z <= maxZ; ++z) {
                // Find all chunks in this column
                for (int y = 0; y < 255; ++y) {
                    glm::ivec3 chunkPos(x, y, z);
                    
                    // Skip if chunk doesn't exist
                    auto it = m_chunks.find(chunkPos);
                    if (it == m_chunks.end()) {
                        continue;
                    }
                    
                    // If chunk has any exposed face, add it to visible set
                    if (it->second->isExposed()) {
                        newVisibleChunks.insert(chunkPos);
                        
                        // Also add chunks adjacent to exposed faces
                        // Check each face of this chunk
                        for (int face = 0; face < 6; ++face) {
                            if (it->second->isFaceExposed(face)) {
                                // Get the position of the adjacent chunk in this direction
                                glm::ivec3 adjacentPos = chunkPos;
                                
                                // Apply offset based on face index
                                switch (face) {
                                    case 0: adjacentPos.z += 1; break; // +Z
                                    case 1: adjacentPos.z -= 1; break; // -Z
                                    case 2: adjacentPos.x -= 1; break; // -X
                                    case 3: adjacentPos.x += 1; break; // +X
                                    case 4: adjacentPos.y += 1; break; // +Y
                                    case 5: adjacentPos.y -= 1; break; // -Y
                                }
                                
                                // Add the adjacent chunk to visible set if it exists
                                if (m_chunks.find(adjacentPos) != m_chunks.end()) {
                                    newVisibleChunks.insert(adjacentPos);
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // Second stage: Handle chunks outside the 16x16 grid
        // Only consider chunks that are already loaded and either:
        // 1. Have exposed faces themselves, or
        // 2. Are adjacent to chunks with exposed faces pointing toward them
        for (const auto& pair : m_chunks) {
            const glm::ivec3& chunkPos = pair.first;
            
            // Skip chunks in the 16x16 grid (already handled)
            if (chunkPos.x >= minX && chunkPos.x <= maxX && 
                chunkPos.z >= minZ && chunkPos.z <= maxZ) {
                continue;
            }
            
            // If this chunk has exposed faces, add it to visible set
            if (pair.second->isExposed()) {
                newVisibleChunks.insert(chunkPos);
                
                // Also add chunks adjacent to exposed faces
                for (int face = 0; face < 6; ++face) {
                    if (pair.second->isFaceExposed(face)) {
                        glm::ivec3 adjacentPos = chunkPos;
                        
                        // Apply offset based on face index
                        switch (face) {
                            case 0: adjacentPos.z += 1; break; // +Z
                            case 1: adjacentPos.z -= 1; break; // -Z
                            case 2: adjacentPos.x -= 1; break; // -X
                            case 3: adjacentPos.x += 1; break; // +X
                            case 4: adjacentPos.y += 1; break; // +Y
                            case 5: adjacentPos.y -= 1; break; // -Y
                        }
                        
                        // Add the adjacent chunk to visible set if it exists
                        if (m_chunks.find(adjacentPos) != m_chunks.end()) {
                            newVisibleChunks.insert(adjacentPos);
                        }
                    }
                }
            }
            // Check if this chunk is adjacent to an exposed chunk with a face toward it
            else if (isAdjacentToExposedChunk(chunkPos)) {
                newVisibleChunks.insert(chunkPos);
            }
        }
        
        // If we exceed the maximum visible chunks, prioritize chunks closer to the player
        if (newVisibleChunks.size() > m_maxVisibleChunks) {
            std::cout << "WARNING: " << newVisibleChunks.size() << " visible chunks exceeds limit of " 
                     << m_maxVisibleChunks << ". Prioritizing closest chunks." << std::endl;
                     
            // Convert to vector for sorting
            std::vector<glm::ivec3> sortedChunks(newVisibleChunks.begin(), newVisibleChunks.end());
            
            // Sort by Manhattan distance from player
            std::sort(sortedChunks.begin(), sortedChunks.end(),
                [playerChunkPos](const glm::ivec3& a, const glm::ivec3& b) {
                    int distA = std::abs(a.x - playerChunkPos.x) + 
                                std::abs(a.y - playerChunkPos.y) * 2 + // Weight Y more
                                std::abs(a.z - playerChunkPos.z);
                                
                    int distB = std::abs(b.x - playerChunkPos.x) + 
                                std::abs(b.y - playerChunkPos.y) * 2 + 
                                std::abs(b.z - playerChunkPos.z);
                                
                    return distA < distB; // Closest first
                }
            );
            
            // Create a new set with only the closest chunks
            newVisibleChunks.clear();
            for (size_t i = 0; i < m_maxVisibleChunks && i < sortedChunks.size(); i++) {
                newVisibleChunks.insert(sortedChunks[i]);
            }
        }
        
        // Calculate debugging stats
        int chunksAdded = 0;
        int chunksRemoved = 0;
        
        // Calculate chunks being added
        for (const auto& chunkPos : newVisibleChunks) {
            if (m_visibleChunks.find(chunkPos) == m_visibleChunks.end()) {
                chunksAdded++;
            }
        }
        
        // Calculate chunks being removed
        for (const auto& chunkPos : m_visibleChunks) {
            if (newVisibleChunks.find(chunkPos) == newVisibleChunks.end()) {
                chunksRemoved++;
            }
        }
        
        // Update the visible chunks set
        m_visibleChunks = std::move(newVisibleChunks);
        
        // Log visibility update status
        std::cout << "Visibility update: " << chunksAdded << " chunks added, " 
                  << chunksRemoved << " chunks removed, " 
                  << m_visibleChunks.size() << " chunks now visible" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ERROR in updateVisibleChunks: " << e.what() << std::endl;
    }
}

// Debug method to print column exposure information
void World::dumpColumnDebugInfo() const {
    std::cout << "=== Column Metadata Debug Information ===" << std::endl;
    std::cout << "Total columns tracked: " << m_columnMetadata.size() << std::endl;
    
    for (const auto& pair : m_columnMetadata) {
        std::cout << "Column (" << pair.first.first << ", " << pair.first.second << "): "
                  << "Top exposed at Y=" << pair.second.topExposedHeight
                  << ", Bottom exposed at Y=" << pair.second.bottomExposedHeight << std::endl;
    }
    
    std::cout << "=======================================" << std::endl;
}

// Helper method to determine if a chunk should be loaded based on exposure rules
bool World::shouldLoadBasedOnExposure(const glm::ivec3& chunkPos) const {
    // First, if the chunk is already loaded, check its exposure directly
    auto chunkIt = m_chunks.find(chunkPos);
    if (chunkIt != m_chunks.end()) {
        return chunkIt->second->isExposed();
    }
    
    // Get the column for this chunk
    ColumnXZ columnKey(chunkPos.x, chunkPos.z);
    
    // If we have column metadata for this column, use it
    auto columnIt = m_columnMetadata.find(columnKey);
    if (columnIt != m_columnMetadata.end()) {
        // This column has exposed chunks
        // Check if this chunk's Y-coordinate is within the exposed range or adjacent to it
        if (chunkPos.y >= columnIt->second.bottomExposedHeight - 1 && 
            chunkPos.y <= columnIt->second.topExposedHeight + 1) {
            return true;
        }
    }
    
    // If no column metadata, check adjacent columns for exposure
    const std::pair<int, int> adjacentColumnOffsets[4] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}
    };
    
    for (int i = 0; i < 4; i++) {
        ColumnXZ adjacentColumnKey(
            columnKey.first + adjacentColumnOffsets[i].first, 
            columnKey.second + adjacentColumnOffsets[i].second
        );
        
        auto adjacentColumnIt = m_columnMetadata.find(adjacentColumnKey);
        if (adjacentColumnIt != m_columnMetadata.end()) {
            // Check if this chunk's Y is within the adjacent column's exposed range
            if (chunkPos.y >= adjacentColumnIt->second.bottomExposedHeight - 1 && 
                chunkPos.y <= adjacentColumnIt->second.topExposedHeight + 1) {
                return true;
            }
        }
    }
    
    // Finally, check if any adjacent chunks are loaded and exposed
    const glm::ivec3 directions[6] = {
        glm::ivec3(1, 0, 0),   // +X
        glm::ivec3(-1, 0, 0),  // -X
        glm::ivec3(0, 1, 0),   // +Y
        glm::ivec3(0, -1, 0),  // -Y
        glm::ivec3(0, 0, 1),   // +Z
        glm::ivec3(0, 0, -1)   // -Z
    };
    
    for (const auto& dir : directions) {
        glm::ivec3 adjacentPos = chunkPos + dir;
        auto adjIt = m_chunks.find(adjacentPos);
        
        if (adjIt != m_chunks.end() && adjIt->second->isExposed()) {
            // Check if the adjacent chunk has the face toward this chunk exposed
            if (adjIt->second->isFaceExposedToChunk(chunkPos)) {
                return true;
            }
        }
    }
    
    // No exposure criteria met, this chunk should not be loaded based on exposure
    return false;
}

// Helper method to update exposure when a block changes
void World::updateExposureOnBlockChange(const glm::ivec3& blockPos) {
    // Convert block position to chunk position
    glm::ivec3 chunkPos = worldToChunkPos(blockPos);
    
    // Update the chunk's exposure mask
    Chunk* chunk = getChunkAt(chunkPos);
    if (chunk) {
        // Get the current exposure state before recalculation
        bool wasExposed = chunk->isExposed();
        ExposureMask oldMask = chunk->getExposureMask();
        
        // Recalculate the chunk's exposure mask
        chunk->calculateExposureMask();
        
        // Only update column metadata if exposure changed
        if (wasExposed != chunk->isExposed() || 
            oldMask.countExposedFaces() != chunk->getExposureMask().countExposedFaces()) {
            updateColumnMetadata(chunkPos);
        }
        
        // Check adjacent chunks too, if the block is at a chunk boundary
        glm::ivec3 localPos = worldToLocalPos(blockPos);
        
        // Only check neighbors if the block is at a boundary
        bool isAtBoundary = (
            localPos.x == 0 || localPos.x == CHUNK_SIZE - 1 || 
            localPos.y == 0 || localPos.y == CHUNK_HEIGHT - 1 || 
            localPos.z == 0 || localPos.z == CHUNK_SIZE - 1
        );
        
        if (isAtBoundary) {
            // Only check the specific faces that could be affected
            if (localPos.x == 0) {
                glm::ivec3 adjacentChunkPos = chunkPos + glm::ivec3(-1, 0, 0);
                Chunk* adjacentChunk = getChunkAt(adjacentChunkPos);
                if (adjacentChunk) {
                    bool wasAdjExposed = adjacentChunk->isExposed();
                    ExposureMask oldAdjMask = adjacentChunk->getExposureMask();
                    
                    // Only recalculate the +X face of the adjacent chunk
                    adjacentChunk->calculateFaceExposure(3); // 3 is +X face
                    
                    if (wasAdjExposed != adjacentChunk->isExposed() ||
                        oldAdjMask.countExposedFaces() != adjacentChunk->getExposureMask().countExposedFaces()) {
                        updateColumnMetadata(adjacentChunkPos);
                    }
                }
            } 
            else if (localPos.x == CHUNK_SIZE - 1) {
                glm::ivec3 adjacentChunkPos = chunkPos + glm::ivec3(1, 0, 0);
                Chunk* adjacentChunk = getChunkAt(adjacentChunkPos);
                if (adjacentChunk) {
                    bool wasAdjExposed = adjacentChunk->isExposed();
                    ExposureMask oldAdjMask = adjacentChunk->getExposureMask();
                    
                    // Only recalculate the -X face of the adjacent chunk
                    adjacentChunk->calculateFaceExposure(2); // 2 is -X face
                    
                    if (wasAdjExposed != adjacentChunk->isExposed() ||
                        oldAdjMask.countExposedFaces() != adjacentChunk->getExposureMask().countExposedFaces()) {
                        updateColumnMetadata(adjacentChunkPos);
                    }
                }
            }
            
            if (localPos.y == 0) {
                glm::ivec3 adjacentChunkPos = chunkPos + glm::ivec3(0, -1, 0);
                Chunk* adjacentChunk = getChunkAt(adjacentChunkPos);
                if (adjacentChunk) {
                    bool wasAdjExposed = adjacentChunk->isExposed();
                    ExposureMask oldAdjMask = adjacentChunk->getExposureMask();
                    
                    // Only recalculate the +Y face of the adjacent chunk
                    adjacentChunk->calculateFaceExposure(4); // 4 is +Y face
                    
                    if (wasAdjExposed != adjacentChunk->isExposed() ||
                        oldAdjMask.countExposedFaces() != adjacentChunk->getExposureMask().countExposedFaces()) {
                        updateColumnMetadata(adjacentChunkPos);
                    }
                }
            } 
            else if (localPos.y == CHUNK_HEIGHT - 1) {
                glm::ivec3 adjacentChunkPos = chunkPos + glm::ivec3(0, 1, 0);
                Chunk* adjacentChunk = getChunkAt(adjacentChunkPos);
                if (adjacentChunk) {
                    bool wasAdjExposed = adjacentChunk->isExposed();
                    ExposureMask oldAdjMask = adjacentChunk->getExposureMask();
                    
                    // Only recalculate the -Y face of the adjacent chunk
                    adjacentChunk->calculateFaceExposure(5); // 5 is -Y face
                    
                    if (wasAdjExposed != adjacentChunk->isExposed() ||
                        oldAdjMask.countExposedFaces() != adjacentChunk->getExposureMask().countExposedFaces()) {
                        updateColumnMetadata(adjacentChunkPos);
                    }
                }
            }
            
            if (localPos.z == 0) {
                glm::ivec3 adjacentChunkPos = chunkPos + glm::ivec3(0, 0, -1);
                Chunk* adjacentChunk = getChunkAt(adjacentChunkPos);
                if (adjacentChunk) {
                    bool wasAdjExposed = adjacentChunk->isExposed();
                    ExposureMask oldAdjMask = adjacentChunk->getExposureMask();
                    
                    // Only recalculate the +Z face of the adjacent chunk
                    adjacentChunk->calculateFaceExposure(0); // 0 is +Z face
                    
                    if (wasAdjExposed != adjacentChunk->isExposed() ||
                        oldAdjMask.countExposedFaces() != adjacentChunk->getExposureMask().countExposedFaces()) {
                        updateColumnMetadata(adjacentChunkPos);
                    }
                }
            } 
            else if (localPos.z == CHUNK_SIZE - 1) {
                glm::ivec3 adjacentChunkPos = chunkPos + glm::ivec3(0, 0, 1);
                Chunk* adjacentChunk = getChunkAt(adjacentChunkPos);
                if (adjacentChunk) {
                    bool wasAdjExposed = adjacentChunk->isExposed();
                    ExposureMask oldAdjMask = adjacentChunk->getExposureMask();
                    
                    // Only recalculate the -Z face of the adjacent chunk
                    adjacentChunk->calculateFaceExposure(1); // 1 is -Z face
                    
                    if (wasAdjExposed != adjacentChunk->isExposed() ||
                        oldAdjMask.countExposedFaces() != adjacentChunk->getExposureMask().countExposedFaces()) {
                        updateColumnMetadata(adjacentChunkPos);
                    }
                }
            }
        }
    }
}

// Update the column metadata for a specific chunk
void World::updateColumnMetadata(const glm::ivec3& chunkPos) {
    // Get the chunk
    auto it = m_chunks.find(chunkPos);
    if (it == m_chunks.end()) {
        return; // Chunk doesn't exist
    }
    
    // Get the exposure mask for this chunk
    const ExposureMask& mask = it->second->getExposureMask();
    
    // If this chunk is not exposed, nothing to update
    if (!mask.isExposed()) {
        return;
    }
    
    // Key for column metadata (X,Z coordinates)
    ColumnXZ columnKey(chunkPos.x, chunkPos.z);
    
    // Check if we already have metadata for this column
    auto columnIt = m_columnMetadata.find(columnKey);
    if (columnIt == m_columnMetadata.end()) {
        // First exposed chunk in this column, initialize the metadata
        m_columnMetadata[columnKey] = ColumnMetadata(chunkPos.y, chunkPos.y);
    } else {
        // Before updating, check if the metadata actually needs changing
        // Only update if this chunk extends the exposure range
        bool changed = false;
        
        if (chunkPos.y > columnIt->second.topExposedHeight) {
            columnIt->second.topExposedHeight = chunkPos.y;
            changed = true;
        }
        if (chunkPos.y < columnIt->second.bottomExposedHeight) {
            columnIt->second.bottomExposedHeight = chunkPos.y;
            changed = true;
        }
        
        // If no change, log this to help debug 
        if (!changed && columnIt->second.topExposedHeight - columnIt->second.bottomExposedHeight > 10) {
            // Only log for columns with significant height range to avoid spam
            std::cout << "Skipped redundant metadata update for column (" 
                      << columnKey.first << "," << columnKey.second 
                      << "), height range already " << columnIt->second.bottomExposedHeight 
                      << "-" << columnIt->second.topExposedHeight << std::endl;
        }
    }
}

// Add a method to update a limited number of dirty chunk meshes per frame
void World::updateDirtyChunkMeshes(int maxUpdatesPerFrame) {
    // Call the reset function periodically
    resetChunkStates();
    
    // Find dirty chunks that need mesh updates
    std::vector<glm::ivec3> dirtyChunks;
    
    for (const auto& pair : m_chunks) {
        if (pair.second->isDirty()) {
            dirtyChunks.push_back(pair.first);
        }
    }
    
    if (dirtyChunks.empty()) {
        return; // No dirty chunks to process
    }
    
    // Get player's chunk position
    glm::ivec3 playerChunkPos = worldToChunkPos(m_lastPlayerPosition);
    
    // Track if the player has moved since last frame
    static glm::ivec3 lastPlayerChunkPos = playerChunkPos;
    bool playerMoved = (lastPlayerChunkPos != playerChunkPos);
    lastPlayerChunkPos = playerChunkPos;
    
    // Define the 16x16 grid boundaries
    int minX = playerChunkPos.x - 8;
    int maxX = playerChunkPos.x + 7;
    int minZ = playerChunkPos.z - 7;
    int maxZ = playerChunkPos.z + 8;
    
    // Log when we start and how many we have
    static bool firstTimeRunning = true;
    if (firstTimeRunning && !dirtyChunks.empty()) {
        std::cout << "*** CRITICAL: First mesh update with " << dirtyChunks.size() 
                  << " dirty chunks needing meshes" << std::endl;
        firstTimeRunning = false;
    }
    
    // If we have too many dirty chunks, something may be wrong - impose a hard limit
    const int MAX_DIRTY_CHUNKS = 1000; // Set a hard limit for safety
    if (dirtyChunks.size() > MAX_DIRTY_CHUNKS) {
        std::cout << "WARNING: Too many dirty chunks (" << dirtyChunks.size() 
                  << ") - forcing reset to avoid excessive processing." << std::endl;
                  
        // Reset all dirty flags, keeping only chunks in player's immediate area
        for (auto& pair : m_chunks) {
            const glm::ivec3& chunkPos = pair.first;
            Chunk* chunk = pair.second.get();
            
            // Keep as dirty only if in immediate area around player and exposed
            bool inImmediateArea = (
                std::abs(chunkPos.x - playerChunkPos.x) <= 2 && 
                std::abs(chunkPos.y - playerChunkPos.y) <= 2 &&
                std::abs(chunkPos.z - playerChunkPos.z) <= 2);
                
            if (!inImmediateArea || !chunk->isExposed()) {
                chunk->setDirty(false);
            }
        }
        
        // Rebuild dirty chunk list with only the immediate chunks
        dirtyChunks.clear();
        for (const auto& pair : m_chunks) {
            if (pair.second->isDirty()) {
                dirtyChunks.push_back(pair.first);
            }
        }
        
        std::cout << "Dirty chunks reduced to " << dirtyChunks.size() << " after reset." << std::endl;
    }
    
    // If player hasn't moved and we're not in the initial loading phase,
    // lower the number of updates per frame to reduce unnecessary mesh regeneration
    if (!playerMoved && !firstTimeRunning && dirtyChunks.size() < 10) {
        maxUpdatesPerFrame = std::min(maxUpdatesPerFrame, 1);
    }
    
    // Sort dirty chunks by priority
    // 1. Chunks within immediate player area (3x3x3 around player) first
    // 2. Chunks in the 16x16 grid with exposed faces next
    // 3. Rest of the chunks sorted by distance
    std::sort(dirtyChunks.begin(), dirtyChunks.end(), 
        [playerChunkPos, this](const glm::ivec3& a, const glm::ivec3& b) {
            // Define immediate area around player (higher priority)
            bool aImmediate = std::abs(a.x - playerChunkPos.x) <= 1 && 
                              std::abs(a.y - playerChunkPos.y) <= 1 && 
                              std::abs(a.z - playerChunkPos.z) <= 1;
            bool bImmediate = std::abs(b.x - playerChunkPos.x) <= 1 && 
                              std::abs(b.y - playerChunkPos.y) <= 1 && 
                              std::abs(b.z - playerChunkPos.z) <= 1;
            
            if (aImmediate && !bImmediate) return true;
            if (!aImmediate && bImmediate) return false;
            
            // Define 16x16 grid
            bool aIn16x16 = (a.x >= playerChunkPos.x - 8 && a.x <= playerChunkPos.x + 7 && 
                             a.z >= playerChunkPos.z - 7 && a.z <= playerChunkPos.z + 8);
            bool bIn16x16 = (b.x >= playerChunkPos.x - 8 && b.x <= playerChunkPos.x + 7 && 
                             b.z >= playerChunkPos.z - 7 && b.z <= playerChunkPos.z + 8);
            
            // Prioritize exposed chunks in 16x16 grid
            bool aExposed = this->isChunkExposed(a);
            bool bExposed = this->isChunkExposed(b);
            
            if (aIn16x16 && aExposed && !(bIn16x16 && bExposed)) return true;
            if (!(aIn16x16 && aExposed) && (bIn16x16 && bExposed)) return false;
            
            // Prioritize any chunks in 16x16 grid
            if (aIn16x16 && !bIn16x16) return true;
            if (!aIn16x16 && bIn16x16) return false;
            
            // For chunks with same priority level, sort by distance
            int distA = std::abs(a.x - playerChunkPos.x) + 
                        std::abs(a.y - playerChunkPos.y) + 
                        std::abs(a.z - playerChunkPos.z);
            int distB = std::abs(b.x - playerChunkPos.x) + 
                        std::abs(b.y - playerChunkPos.y) + 
                        std::abs(b.z - playerChunkPos.z);
            
            return distA < distB;
        }
    );
    
    // CRITICAL: Process MORE chunks early in the game's lifecycle
    // This ensures the initial state gets all meshes generated quickly
    static int firstFramesMeshCount = 0;
    if (firstFramesMeshCount < 20) {
        maxUpdatesPerFrame = 100; // Process many more chunks on first frames
        firstFramesMeshCount++;
    }
    else {
        // Increase updates for immediate area chunks
        int immediateAreaChunks = 0;
        for (const auto& chunkPos : dirtyChunks) {
            bool isImmediate = std::abs(chunkPos.x - playerChunkPos.x) <= 1 && 
                               std::abs(chunkPos.y - playerChunkPos.y) <= 1 && 
                               std::abs(chunkPos.z - playerChunkPos.z) <= 1;
            if (isImmediate) immediateAreaChunks++;
        }
        
        // If we have many chunks in immediate area, increase update count
        if (immediateAreaChunks > maxUpdatesPerFrame / 2) {
            maxUpdatesPerFrame = std::min(maxUpdatesPerFrame + 10, immediateAreaChunks + 5);
        }
    }
    
    // Add timeout protection
    float startProcessingTime = glfwGetTime();
    const float MAX_MESH_PROCESSING_TIME = 0.05f; // 50ms max for mesh updates
    
    // Update a limited number of meshes
    int updatesThisFrame = 0;
    for (const auto& chunkPos : dirtyChunks) {
        if (updatesThisFrame >= maxUpdatesPerFrame) {
            break;
        }
        
        // Check timeout
        if (glfwGetTime() - startProcessingTime > MAX_MESH_PROCESSING_TIME) {
            break;
        }
        
        Chunk* chunk = getChunkAt(chunkPos);
        if (chunk && chunk->isDirty()) {
            // Skip chunks that are outside the player's current grid and not exposed
            bool inGrid = (chunkPos.x >= minX && chunkPos.x <= maxX && 
                          chunkPos.z >= minZ && chunkPos.z <= maxZ);
                          
            if (!inGrid && !chunk->isExposed() && !isAdjacentToExposedChunk(chunkPos)) {
                // Just clear the dirty flag and skip mesh generation
                chunk->setDirty(false);
                continue;
            }
            
            // Skip chunks that already have a mesh when player hasn't moved
            // This prevents constant regeneration of meshes when standing still
            if (!playerMoved && chunk->hasMesh() && !firstTimeRunning) {
                chunk->setDirty(false);
                continue;
            }
            
            // Check if this chunk should be visible based on position and exposure
            bool shouldBeVisible = false;
            
            // Is it in the 16x16 grid and exposed?
            bool isExposed = chunk->isExposed();
            bool isAdjacentToExposed = isAdjacentToExposedChunk(chunkPos);
            
            // Determine if chunk should be visible
            if ((inGrid && isExposed) || isAdjacentToExposed) {
                shouldBeVisible = true;
            }
            
            // Only add to visible set if it should be visible
            if (shouldBeVisible) {
                m_visibleChunks.insert(chunkPos);
            }
            
            // Generate the mesh
            chunk->generateMesh(m_disableGreedyMeshing);
            chunk->setDirty(false);
            updatesThisFrame++;
        }
    }
    
    // Always log the first few updates (important for debugging)
    static int initialUpdates = 0;
    bool shouldLog = false;
    
    if (initialUpdates < 5) {
        shouldLog = true;
        initialUpdates++;
    } else {
        // Only log if we updated chunks and not too frequently
        static int updateLogCounter = 0;
        if (updatesThisFrame > 0 && updateLogCounter++ % 120 == 0) {
            shouldLog = true;
        }
    }
    
    if (shouldLog) {
        std::cout << "Updated " << updatesThisFrame << " dirty chunk meshes, " 
                  << (dirtyChunks.size() - updatesThisFrame) << " remaining." << std::endl;
        std::cout << "Total visible chunks: " << m_visibleChunks.size() 
                  << ", Total loaded chunks: " << m_chunks.size() << std::endl;
    }
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

// Determine if a chunk is exposed (has at least one face exposed)
bool World::isChunkExposed(const glm::ivec3& chunkPos) const {
    // Get the chunk
    auto it = m_chunks.find(chunkPos);
    if (it == m_chunks.end()) {
        // If chunk doesn't exist, check if it's at a world boundary
        if (chunkPos.y <= 0 || chunkPos.y >= 255) {
            // Chunks at vertical world boundaries are always considered exposed
            return true;
        }
        
        // For non-existent chunks, we need to check if they would be exposed by:
        // 1. Looking at adjacent chunks if they exist
        // 2. Checking column metadata
        
        // Check column metadata first (more efficient)
        ColumnXZ columnKey(chunkPos.x, chunkPos.z);
        auto colIt = m_columnMetadata.find(columnKey);
        if (colIt != m_columnMetadata.end()) {
            // If this chunk's Y is within the exposed range, it's potentially exposed
            if (chunkPos.y >= colIt->second.bottomExposedHeight - 1 && 
                chunkPos.y <= colIt->second.topExposedHeight + 1) {
                return true;
            }
        }
        
        // Check all adjacent chunks if they exist
        const glm::ivec3 directions[6] = {
            glm::ivec3(1, 0, 0),   // +X
            glm::ivec3(-1, 0, 0),  // -X
            glm::ivec3(0, 1, 0),   // +Y
            glm::ivec3(0, -1, 0),  // -Y
            glm::ivec3(0, 0, 1),   // +Z
            glm::ivec3(0, 0, -1)   // -Z
        };
        
        for (const auto& dir : directions) {
            glm::ivec3 adjPos = chunkPos + dir;
            auto adjIt = m_chunks.find(adjPos);
            
            if (adjIt != m_chunks.end() && adjIt->second->isExposed()) {
                // If the adjacent chunk exists and is exposed on the face toward this chunk,
                // this chunk would likely be exposed too
                if (adjIt->second->isFaceExposedToChunk(chunkPos)) {
                    return true;
                }
            }
        }
        
        // We don't have enough information to determine exposure
        return false;
    }
    
    // For an existing chunk, check if it has any exposed faces
    return it->second->isExposed();
}

// Check if a chunk is adjacent to any exposed chunk
bool World::isAdjacentToExposedChunk(const glm::ivec3& chunkPos) const {
    // Array of offsets for the 6 adjacent chunks (±1 in each direction)
    const glm::ivec3 adjacentOffsets[6] = {
        glm::ivec3(1, 0, 0),  // +X
        glm::ivec3(-1, 0, 0), // -X
        glm::ivec3(0, 1, 0),  // +Y
        glm::ivec3(0, -1, 0), // -Y
        glm::ivec3(0, 0, 1),  // +Z
        glm::ivec3(0, 0, -1)  // -Z
    };
    
    // Check each adjacent chunk
    for (int i = 0; i < 6; i++) {
        glm::ivec3 adjacentPos = chunkPos + adjacentOffsets[i];
        
        // Find the adjacent chunk
        auto it = m_chunks.find(adjacentPos);
        if (it == m_chunks.end()) {
            continue; // Skip if the adjacent chunk doesn't exist
        }
        
        // If the adjacent chunk is exposed and specifically has the face toward this chunk exposed, return true
        if (it->second->isExposed()) {
            // Check if the face of the adjacent chunk toward this chunk is exposed
            if (it->second->isFaceExposedToChunk(chunkPos)) {
                return true;
            }
        }
    }
    
    // No adjacent exposed chunks found
    return false;
}

void World::evaluateChunksNeeded(const glm::vec3& playerPos) {
    try {
        // Only evaluate if the world is initialized
        if (!m_initialized) {
            return;
        }

        // Track when we last evaluated to prevent thrashing
        static float lastEvaluationTime = 0.0f;
        float currentTime = glfwGetTime();
        
        // Only evaluate once per second to prevent oscillation/thrashing
        if (currentTime - lastEvaluationTime < 1.0f) {
            return;
        }
        lastEvaluationTime = currentTime;
        
        // Safety check for invalid positions
        if (std::isnan(playerPos.x) || std::isnan(playerPos.y) || std::isnan(playerPos.z) ||
            std::isinf(playerPos.x) || std::isinf(playerPos.y) || std::isinf(playerPos.z)) {
            std::cerr << "ERROR: Invalid player position detected: " 
                    << playerPos.x << ", " << playerPos.y << ", " << playerPos.z << std::endl;
            return;
        }
        
        // Update the stored position if valid
        m_lastPlayerPosition = playerPos;
        
        // Convert player position to chunk coordinates
        glm::ivec3 playerChunkPos = worldToChunkPos(playerPos);
        
        // Define the 16x16 grid boundaries around the player
        int minX = playerChunkPos.x - 8;
        int maxX = playerChunkPos.x + 7;
        int minZ = playerChunkPos.z - 7;
        int maxZ = playerChunkPos.z + 8;
        
        // Temporary collections for chunk operations
        std::vector<glm::ivec3> newChunksToLoad;
        std::vector<glm::ivec3> newChunksToUnload;
        
        // Cache currently loaded chunks
        std::unordered_set<glm::ivec3, Vec3Hash> existingChunkPositions;
        for (const auto& pair : m_chunks) {
            existingChunkPositions.insert(pair.first);
        }
        
        // Cache what's already in the load queue to avoid duplicates
        std::unordered_set<glm::ivec3, Vec3Hash> chunksInLoadQueue;
        for (const auto& chunkPos : m_chunksToLoadQueue) {
            chunksInLoadQueue.insert(chunkPos);
        }
        
        // STEP 1: First identify all columns that need evaluation
        std::set<ColumnXZ> columnsToCheck;
        
        // Add all columns in the 16x16 grid
        for (int x = minX; x <= maxX; x++) {
            for (int z = minZ; z <= maxZ; z++) {
                columnsToCheck.insert(ColumnXZ(x, z));
            }
        }
        
        // Also add columns with exposed chunks that are adjacent to the grid
        int gridBoundaryPadding = 1; // Only check 1 chunk out from the grid
        for (const auto& colMeta : m_columnMetadata) {
            // Skip if already in the set
            if (columnsToCheck.count(colMeta.first) > 0) {
                continue;
            }
            
            // Check if this column is adjacent to the player's grid
            bool adjacentToGrid = 
                (colMeta.first.first >= minX - gridBoundaryPadding && colMeta.first.first <= maxX + gridBoundaryPadding &&
                 colMeta.first.second >= minZ - gridBoundaryPadding && colMeta.first.second <= maxZ + gridBoundaryPadding);
                 
            if (adjacentToGrid) {
                columnsToCheck.insert(colMeta.first);
            }
        }
        
        // STEP 2: For each column, determine which chunks should be loaded
        int exposedChunksInGrid = 0;
        
        for (const auto& columnKey : columnsToCheck) {
            // Determine if this column is within the player's grid
            bool inGrid = (columnKey.first >= minX && columnKey.first <= maxX && 
                           columnKey.second >= minZ && columnKey.second <= maxZ);
                           
            // Get column metadata if it exists
            auto colIt = m_columnMetadata.find(columnKey);
            
            // Determine Y range to check based on exposure info
            int minY, maxY;
            
            if (colIt != m_columnMetadata.end()) {
                // We have exposure data, use it to determine range
                minY = colIt->second.bottomExposedHeight - 1; // Include one below
                maxY = colIt->second.topExposedHeight + 1;    // Include one above
                
                // Ensure we also include player's height in grid
                if (inGrid) {
                    minY = std::min(minY, playerChunkPos.y - 2);
                    maxY = std::max(maxY, playerChunkPos.y + 2);
                }
                
                // Constrain to reasonable bounds
                minY = std::max(0, minY);
                maxY = std::min(255, maxY);
            } else {
                // No exposure data for this column
                if (inGrid) {
                    // For columns in grid with no exposure data, load around player
                    minY = std::max(0, playerChunkPos.y - 2);
                    maxY = std::min(255, playerChunkPos.y + 2);
                } else {
                    // Skip columns outside grid with no exposure data
                    continue;
                }
            }
            
            // Check each chunk in the column's Y range
            for (int y = minY; y <= maxY; y++) {
                glm::ivec3 chunkPos(columnKey.first, y, columnKey.second);
                
                // Skip if chunk is already loaded or in queue
                if (existingChunkPositions.count(chunkPos) > 0 || 
                    chunksInLoadQueue.count(chunkPos) > 0) {
                    continue;
                }
                
                // Determine if we should load this chunk
                bool shouldLoad = false;
                
                if (inGrid) {
                    // For chunks in 16x16 grid, load if:
                    // 1. Near player's height (+/- 2 chunks)
                    // 2. Within exposed range (or adjacent to it)
                    if (std::abs(y - playerChunkPos.y) <= 2) {
                        shouldLoad = true;
                    } else if (colIt != m_columnMetadata.end()) {
                        // Check exposure-based criteria
                        shouldLoad = (y >= colIt->second.bottomExposedHeight - 1 && 
                                      y <= colIt->second.topExposedHeight + 1);
                    }
                } else {
                    // For chunks outside the grid, only load if they're exposed or adjacent to exposed
                    shouldLoad = shouldLoadBasedOnExposure(chunkPos);
                }
                
                if (shouldLoad) {
                    newChunksToLoad.push_back(chunkPos);
                }
            }
        }
        
        // STEP 3: Find chunks to unload (more than 8 chunks away horizontally)
        int chunksChecked = 0;
        const int MAX_CHUNKS_TO_CHECK = 1000; // Limit to prevent performance issues
        
        for (const auto& pair : m_chunks) {
            if (chunksChecked++ > MAX_CHUNKS_TO_CHECK) break;
            
            const glm::ivec3& chunkPos = pair.first;
            
            // Keep chunks in the 16x16 grid around player
            bool inGrid = (chunkPos.x >= minX && chunkPos.x <= maxX &&
                          chunkPos.z >= minZ && chunkPos.z <= maxZ);
            if (inGrid) continue;
            
            // Don't unload chunks that are close to player's height
            bool nearPlayerHeight = std::abs(chunkPos.y - playerChunkPos.y) <= 2;
            if (nearPlayerHeight && (std::abs(chunkPos.x - playerChunkPos.x) <= 10 &&
                                    std::abs(chunkPos.z - playerChunkPos.z) <= 10)) {
                continue;
            }
            
            // Don't unload ground-level chunks (important for world continuity)
            bool isGroundLevel = chunkPos.y < 4;
            if (isGroundLevel) continue;
            
            // Check exposure criteria - don't unload exposed chunks or those adjacent to exposed
            bool isExposed = pair.second->isExposed();
            bool isAdjacentToExposed = false;
            
            if (!isExposed) {
                isAdjacentToExposed = isAdjacentToExposedChunk(chunkPos);
            }
            
            // If not exposed and not adjacent to exposed, mark for unloading
            if (!isExposed && !isAdjacentToExposed) {
                newChunksToUnload.push_back(chunkPos);
            }
        }
        
        // Sort load candidates by distance (closest first)
        std::sort(newChunksToLoad.begin(), newChunksToLoad.end(),
            [playerChunkPos](const glm::ivec3& a, const glm::ivec3& b) {
                // Manhattan distance is faster than Euclidean and works well for this
                float distA = std::abs(a.x - playerChunkPos.x) + 
                             std::abs(a.y - playerChunkPos.y) + 
                             std::abs(a.z - playerChunkPos.z);
                float distB = std::abs(b.x - playerChunkPos.x) + 
                             std::abs(b.y - playerChunkPos.y) + 
                             std::abs(b.z - playerChunkPos.z);
                return distA < distB; // Closest FIRST
            }
        );
        
        // Sort unload candidates by distance (furthest first)
        std::sort(newChunksToUnload.begin(), newChunksToUnload.end(),
            [playerChunkPos](const glm::ivec3& a, const glm::ivec3& b) {
                float distA = std::abs(a.x - playerChunkPos.x) + 
                             std::abs(a.y - playerChunkPos.y) + 
                             std::abs(a.z - playerChunkPos.z);
                float distB = std::abs(b.x - playerChunkPos.x) + 
                             std::abs(b.y - playerChunkPos.y) + 
                             std::abs(b.z - playerChunkPos.z);
                return distA > distB; // Furthest FIRST
            }
        );
        
        // Add to loading queue (keep a reasonable limit)
        const int MAX_NEW_CHUNKS_TO_QUEUE = 50;
        for (size_t i = 0; i < newChunksToLoad.size() && i < MAX_NEW_CHUNKS_TO_QUEUE; i++) {
            m_chunksToLoadQueue.push_back(newChunksToLoad[i]);
        }
        
        // Add to unloading queue (keep a reasonable limit)
        const int MAX_NEW_CHUNKS_TO_UNLOAD = 20;
        for (size_t i = 0; i < newChunksToUnload.size() && i < MAX_NEW_CHUNKS_TO_UNLOAD; i++) {
            m_chunksToUnloadQueue.push_back(newChunksToUnload[i]);
        }
        
        // Update the pending chunk operations count
        m_pendingChunkOperations = m_chunksToLoadQueue.size();
        
        // Log what happened (occasionally)
        static int logCounter = 0;
        if (logCounter++ % 10 == 0) {
            std::cout << "Chunk evaluation: Added " << newChunksToLoad.size() 
                      << " chunks to load queue, " << newChunksToUnload.size() 
                      << " to unload queue. Queue sizes: Load=" << m_chunksToLoadQueue.size() 
                      << ", Unload=" << m_chunksToUnloadQueue.size() 
                      << ", Active chunks=" << m_chunks.size() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR in evaluateChunksNeeded: " << e.what() << std::endl;
    }
}

// Add a new method to periodically reset all dirty flags and visibility state
void World::resetChunkStates() {
    static float lastResetTime = 0.0f;
    float currentTime = glfwGetTime();
    
    // Only perform this reset once every 30 seconds (increased from 10s to reduce frequency)
    if (currentTime - lastResetTime < 30.0f) {
        return;
    }
    
    // Track if player has moved significantly since last reset
    static glm::ivec3 lastResetPlayerChunkPos = worldToChunkPos(m_lastPlayerPosition);
    glm::ivec3 currentPlayerChunkPos = worldToChunkPos(m_lastPlayerPosition);
    bool playerMovedSignificantly = (lastResetPlayerChunkPos != currentPlayerChunkPos);
    lastResetPlayerChunkPos = currentPlayerChunkPos;
    
    // If player hasn't moved significantly, don't do a full reset
    if (!playerMovedSignificantly) {
        // Skip the reset to avoid unnecessary mesh regeneration
        lastResetTime = currentTime; // Update the time anyway
        return;
    }
    
    lastResetTime = currentTime;
    std::cout << "*** PERFORMING FULL CHUNK STATE RESET ***" << std::endl;

    // Define the strict 16x16 grid boundaries
    int minX = currentPlayerChunkPos.x - 8;
    int maxX = currentPlayerChunkPos.x + 7;
    int minZ = currentPlayerChunkPos.z - 7;
    int maxZ = currentPlayerChunkPos.z + 8;
    
    // Reset visibility set
    size_t oldVisibleCount = m_visibleChunks.size();
    m_visibleChunks.clear();
    
    // Reset dirty flags and rebuild visibility set properly
    int dirtyChunks = 0;
    int visibleChunks = 0;
    
    for (auto& pair : m_chunks) {
        const glm::ivec3& chunkPos = pair.first;
        Chunk* chunk = pair.second.get();
        
        // Clear dirty flag for chunks that already have meshes
        bool wasDirty = chunk->isDirty();
        if (wasDirty && chunk->hasMesh()) {
            chunk->setDirty(false);
            dirtyChunks++;
        }
        
        // Apply proper visibility rules
        bool inGrid = (chunkPos.x >= minX && chunkPos.x <= maxX && 
                     chunkPos.z >= minZ && chunkPos.z <= maxZ);
                     
        // Add to visible set ONLY IF:
        // 1. In grid and exposed
        // 2. Adjacent to an exposed chunk and that exposed face points toward this chunk
        if ((inGrid && chunk->isExposed()) || 
            isAdjacentToExposedChunk(chunkPos)) {
            m_visibleChunks.insert(chunkPos);
            
            // Only re-mark as dirty if it's exposed AND doesn't have a mesh yet
            if (chunk->isExposed() && !chunk->hasMesh()) {
                chunk->setDirty(true);
            }
            
            visibleChunks++;
        }
    }
    
    std::cout << "Reset " << dirtyChunks << " dirty chunks and rebuilt visibility - now "
              << visibleChunks << " visible chunks (was " << oldVisibleCount << ")" << std::endl;
}

// Add a method to reset visible chunks
void World::resetVisibleChunks() {
    size_t oldSize = m_visibleChunks.size();
    m_visibleChunks.clear();
    std::cout << "Reset visible chunks list. Removed " << oldSize << " entries." << std::endl;
} 