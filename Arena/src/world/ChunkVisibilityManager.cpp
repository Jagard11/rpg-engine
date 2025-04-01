#include "world/ChunkVisibilityManager.hpp"
#include "world/World.hpp"
#include "player/Player.hpp"
#include <iostream>
#include <unordered_set>
#include <utility> // for std::pair
#include <GLFW/glfw3.h> // For glfwGetTime
#include <algorithm>
#include <glm/gtx/vector_angle.hpp>

// Define hash function for std::pair<int, int>
namespace std {
    template<>
    struct hash<std::pair<int, int>> {
        size_t operator()(const std::pair<int, int>& p) const {
            // Combine hashes of the x and z coordinates
            size_t h1 = hash<int>()(p.first);
            size_t h2 = hash<int>()(p.second);
            return h1 ^ (h2 << 1);
        }
    };
}

ChunkVisibilityManager::ChunkVisibilityManager(World* world)
    : m_world(world)
{
}

ChunkVisibilityManager::~ChunkVisibilityManager()
{
}

void ChunkVisibilityManager::updateChunkVisibility(const Player& player)
{
    // Early return if world isn't ready
    if (!m_world || !m_world->isInitialized()) {
        return;
    }

    // Get the player's position and forward vector
    glm::vec3 playerPosition = player.getPosition();
    glm::vec3 playerForward = player.getForward();
    
    // Calculate the grid coordinates of the player
    glm::ivec3 playerChunkPos = m_world->worldToChunkPos(playerPosition);

    // Clear previous visibility data
    m_world->clearVisibleChunks(); 

    // Set up the grid boundaries for chunk visibility
    // Use appropriate render distance from world
    int renderDistance = m_world->getViewDistance();
    int minX = playerChunkPos.x - (renderDistance / 2);
    int maxX = playerChunkPos.x + (renderDistance / 2);
    int minY = std::max(0, playerChunkPos.y - (renderDistance / 2));
    int maxY = playerChunkPos.y + (renderDistance / 2);
    int minZ = playerChunkPos.z - (renderDistance / 2);
    int maxZ = playerChunkPos.z + (renderDistance / 2);

    // For performance logging
    int chunksProcessed = 0;
    int chunksVisible = 0;
    int chunksRenderable = 0;
    
    std::cout << "Starting chunk visibility update. Player at grid (" 
              << playerChunkPos.x << ", " << playerChunkPos.y 
              << ", " << playerChunkPos.z << ")" << std::endl;
    std::cout << "Grid boundaries: X[" << minX << ", " << maxX << "] Y[" << minY << ", " << maxY 
              << "] Z[" << minZ << ", " << maxZ << "]" << std::endl;
    
    // EXTREME PERFORMANCE OPTIMIZATION: Process a maximum of 50 chunks per frame
    // This prevents the game from freezing by spreading chunk processing across frames
    const int MAX_CHUNKS_PER_FRAME = 50;
    
    // Track chunks we've already processed in previous frames
    static std::unordered_set<glm::ivec3, std::hash<glm::ivec3>> processedChunks;
    
    // Store chunks to process in priority order (near to far)
    std::vector<glm::ivec3> chunksToProcess;

    // Add chunks to the processing queue
    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            for (int z = minZ; z <= maxZ; z++) {
                glm::ivec3 chunkPos(x, y, z);
                if (processedChunks.find(chunkPos) == processedChunks.end()) {
                    chunksToProcess.push_back(chunkPos);
                }
            }
        }
    }
    
    // Sort chunks by distance to player (closest first)
    std::sort(chunksToProcess.begin(), chunksToProcess.end(), 
        [&playerPosition](const glm::ivec3& a, const glm::ivec3& b) {
            glm::vec3 posA(a.x * Chunk::CHUNK_SIZE, a.y * Chunk::CHUNK_SIZE, a.z * Chunk::CHUNK_SIZE);
            glm::vec3 posB(b.x * Chunk::CHUNK_SIZE, b.y * Chunk::CHUNK_SIZE, b.z * Chunk::CHUNK_SIZE);
            return glm::distance(posA, playerPosition) < glm::distance(posB, playerPosition);
        });
    
    std::cout << "Found " << chunksToProcess.size() << " unprocessed chunks" << std::endl;
    
    // Process only a limited number of chunks this frame
    int chunksToProcessThisFrame = std::min(static_cast<int>(chunksToProcess.size()), MAX_CHUNKS_PER_FRAME);
    
    for (int i = 0; i < chunksToProcessThisFrame; i++) {
        glm::ivec3 chunkPos = chunksToProcess[i];
        chunksProcessed++;
        
        // Add to processed set
        processedChunks.insert(chunkPos);
        
        // Add to visible chunks
        m_world->addToVisibleChunks(chunkPos);
        chunksVisible++;
        
        // Only check for exposed faces if the chunk exists and is loaded
        bool hasExposed = checkForOpenVoxel(chunkPos);
        if (hasExposed) {
            markChunkForRendering(chunkPos);
            chunksRenderable++;
        }
    }
    
    // If we've processed all chunks, clear the tracking set to start fresh next time
    if (chunksToProcessThisFrame < chunksToProcess.size()) {
        std::cout << "Processed " << chunksToProcessThisFrame << " out of " << chunksToProcess.size() 
                  << " chunks this frame" << std::endl;
    } else {
        std::cout << "Processed all " << chunksToProcess.size() << " chunks this frame, resetting tracker" << std::endl;
        processedChunks.clear();
    }

    std::cout << "Chunk visibility update: " << chunksProcessed << " processed, " 
              << chunksVisible << " visible, " << chunksRenderable << " renderable" << std::endl;
}

bool ChunkVisibilityManager::checkForOpenVoxel(const glm::ivec3& chunkPos)
{
    // First check if the chunk exists and is loaded
    const auto& chunks = m_world->getChunks();
    auto it = chunks.find(chunkPos);
    if (it == chunks.end()) {
        // Chunk doesn't exist, so we can't check for open voxels
        return true; // Assume it should be rendered until proven otherwise
    }
    
    // Get a reference to the chunk
    Chunk* chunk = it->second.get();
    if (!chunk) {
        return true; // Safety check - assume it should be rendered
    }
    
    // Check if chunk is empty - no need to search further
    if (chunk->isEmpty()) {
        chunk->setHasVisibleFaces(false);
        return false;
    }
    
    std::cout << "Checking exposed faces for chunk (" << chunkPos.x << ", " 
              << chunkPos.y << ", " << chunkPos.z << ")" << std::endl;

    // Check bottom face (y=0)
    for (int x = 0; x < Chunk::CHUNK_SIZE; x++) {
        for (int z = 0; z < Chunk::CHUNK_SIZE; z++) {
            if (chunk->getBlock(x, 0, z) != 0) {
                // Check if exposed to air (below the chunk)
                int belowVoxel = getVoxel(chunkPos, x, -1, z);
                if (belowVoxel == 0) {
                    std::cout << "  Found exposed voxel at bottom face" << std::endl;
                    chunk->setHasVisibleFaces(true);
                    return true;
                }
            }
        }
    }

    // Check top face (y=CHUNK_HEIGHT-1)
    for (int x = 0; x < Chunk::CHUNK_SIZE; x++) {
        for (int z = 0; z < Chunk::CHUNK_SIZE; z++) {
            if (chunk->getBlock(x, Chunk::CHUNK_HEIGHT - 1, z) != 0) {
                // Check if exposed to air (above the chunk)
                int aboveVoxel = getVoxel(chunkPos, x, Chunk::CHUNK_HEIGHT, z);
                if (aboveVoxel == 0) {
                    std::cout << "  Found exposed voxel at top face" << std::endl;
                    chunk->setHasVisibleFaces(true);
                    return true;
                }
            }
        }
    }

    // Check front face (z=0)
    for (int x = 0; x < Chunk::CHUNK_SIZE; x++) {
        for (int y = 0; y < Chunk::CHUNK_HEIGHT; y++) {
            if (chunk->getBlock(x, y, 0) != 0) {
                // Check if exposed to air (in front of the chunk)
                int frontVoxel = getVoxel(chunkPos, x, y, -1);
                if (frontVoxel == 0) {
                    std::cout << "  Found exposed voxel at front face" << std::endl;
                    chunk->setHasVisibleFaces(true);
                    return true;
                }
            }
        }
    }

    // Check back face (z=CHUNK_SIZE-1)
    for (int x = 0; x < Chunk::CHUNK_SIZE; x++) {
        for (int y = 0; y < Chunk::CHUNK_HEIGHT; y++) {
            if (chunk->getBlock(x, y, Chunk::CHUNK_SIZE - 1) != 0) {
                // Check if exposed to air (behind the chunk)
                int backVoxel = getVoxel(chunkPos, x, y, Chunk::CHUNK_SIZE);
                if (backVoxel == 0) {
                    std::cout << "  Found exposed voxel at back face" << std::endl;
                    chunk->setHasVisibleFaces(true);
                    return true;
                }
            }
        }
    }

    // Check left face (x=0)
    for (int y = 0; y < Chunk::CHUNK_HEIGHT; y++) {
        for (int z = 0; z < Chunk::CHUNK_SIZE; z++) {
            if (chunk->getBlock(0, y, z) != 0) {
                // Check if exposed to air (to the left of the chunk)
                int leftVoxel = getVoxel(chunkPos, -1, y, z);
                if (leftVoxel == 0) {
                    std::cout << "  Found exposed voxel at left face" << std::endl;
                    chunk->setHasVisibleFaces(true);
                        return true;
                    }
            }
        }
    }

    // Check right face (x=CHUNK_SIZE-1)
    for (int y = 0; y < Chunk::CHUNK_HEIGHT; y++) {
        for (int z = 0; z < Chunk::CHUNK_SIZE; z++) {
            if (chunk->getBlock(Chunk::CHUNK_SIZE - 1, y, z) != 0) {
                // Check if exposed to air (to the right of the chunk)
                int rightVoxel = getVoxel(chunkPos, Chunk::CHUNK_SIZE, y, z);
                if (rightVoxel == 0) {
                    std::cout << "  Found exposed voxel at right face" << std::endl;
                    chunk->setHasVisibleFaces(true);
                    return true;
                }
            }
        }
    }

    // If we get here, no exposed faces were found
    std::cout << "  No exposed faces found for chunk" << std::endl;
    chunk->setHasVisibleFaces(false);
    return false;
}

bool ChunkVisibilityManager::isAdjacentToOpenVoxel(const glm::ivec3& chunkPos, int x, int y, int z)
{
    if (x < 0 || x >= Chunk::CHUNK_SIZE || 
        y < 0 || y >= Chunk::CHUNK_HEIGHT || 
        z < 0 || z >= Chunk::CHUNK_SIZE) {
        // Out of bounds
        return false;
    }
    
    // Get the current voxel type
    int currentVoxel = getVoxel(chunkPos, x, y, z);
    
    // If the voxel itself is air, it's not a boundary with air
    if (currentVoxel == 0) {
        return false;
    }
    
    // Check all 6 adjacent voxels
    const int dx[] = {-1, 1, 0, 0, 0, 0};
    const int dy[] = {0, 0, -1, 1, 0, 0};
    const int dz[] = {0, 0, 0, 0, -1, 1};
    
    for (int i = 0; i < 6; ++i) {
        try {
            int adjacentVoxel = getVoxel(chunkPos, x + dx[i], y + dy[i], z + dz[i]);
            if (adjacentVoxel == 0) { // 0 represents air
                return true;
            }
        } catch (const std::exception& e) {
            // Just skip this direction if it causes an error
            continue;
        }
    }
    
    return false;
}

int ChunkVisibilityManager::getVoxel(const glm::ivec3& chunkPos, int x, int y, int z)
{
    // Handle voxel coordinates that go outside current chunk bounds
    int chunkX = chunkPos.x;
    int chunkY = chunkPos.y;
    int chunkZ = chunkPos.z;
    
    // Adjust coordinates if they're outside the current chunk
    if (x < 0) {
        x += Chunk::CHUNK_SIZE;
        chunkX--;
    } else if (x >= Chunk::CHUNK_SIZE) {
        x -= Chunk::CHUNK_SIZE;
        chunkX++;
    }
    
    if (y < 0) {
        y += Chunk::CHUNK_HEIGHT;
        chunkY--;
    } else if (y >= Chunk::CHUNK_HEIGHT) {
        y -= Chunk::CHUNK_HEIGHT;
        chunkY++;
    }
    
    if (z < 0) {
        z += Chunk::CHUNK_SIZE;
        chunkZ--;
    } else if (z >= Chunk::CHUNK_SIZE) {
        z -= Chunk::CHUNK_SIZE;
        chunkZ++;
    }
    
    // Bounds checking
    if (y < 0) {
        return 0; // Assume air below the world
    }
    
    // Calculate the world position for this voxel
    glm::ivec3 worldPos(
        chunkX * Chunk::CHUNK_SIZE + x,
        chunkY * Chunk::CHUNK_HEIGHT + y,
        chunkZ * Chunk::CHUNK_SIZE + z
    );
    
    // Use try-catch in case getBlock throws an exception
    try {
        // Get the block type at this world position
        return m_world->getBlock(worldPos);
    } catch (const std::exception& e) {
        // Default to air when errors occur
        return 0;
    }
}

void ChunkVisibilityManager::markChunkForRendering(const glm::ivec3& chunkPos)
{
    // First, add to visible chunks set
    m_world->addToVisibleChunks(chunkPos);
    
    // Update the chunk's visibility flag if it exists
    const auto& chunks = m_world->getChunks();
    auto it = chunks.find(chunkPos);
    if (it != chunks.end() && it->second) {
        // We need to const_cast since getChunks() returns a const reference
        Chunk* chunk = const_cast<Chunk*>(it->second.get());
        chunk->setHasVisibleFaces(true);
    }
    
    // For initialization, we don't want to immediately load all chunks
    // as it will cause the game to lock up.
    // Instead, we'll let the normal chunk loading system handle it
    // in the World::processChunkQueues method.
    
    // We don't need to do anything else - the evaluateChunksNeeded method
    // will handle loading chunks that are in the visible set.
}

void ChunkVisibilityManager::loadChunkIntoMemory(const glm::ivec3& chunkPos)
{
    // Request the chunk to be loaded into memory immediately
    m_world->loadChunk(chunkPos);
}

bool ChunkVisibilityManager::shouldSkipChunk(const glm::ivec3& chunkPos) {
    // Fast ground level check - always load chunks near ground level
    // This ensures that the terrain is always visible
    if (chunkPos.y >= 0 && chunkPos.y <= 4) {
        return false; // Don't skip chunks at or near ground level (0-4)
    }

    // Check if chunk exists in memory
    const auto& chunks = m_world->getChunks();
    auto it = chunks.find(chunkPos);
    
    // If chunk doesn't exist yet, we need a quick heuristic
    if (it == chunks.end()) {
        // For chunks at player level or slightly above/below, try to load
        glm::ivec3 playerChunkPos = m_world->worldToChunkPos(m_world->getLastPlayerPosition());
        int verticalDistance = std::abs(chunkPos.y - playerChunkPos.y);
        
        if (verticalDistance <= 2) {
            return false; // Don't skip chunks near player's vertical position
        }

        // For non-ground chunks, only check if they're below a known non-empty chunk
        // This prevents loading too many empty air chunks above ground
        if (chunkPos.y > 4) {
            // For high chunks, check if any chunk below in the same column exists and isn't empty
            // Only check a few chunks below to avoid excessive checking
            for (int y = chunkPos.y - 1; y >= std::max(0, chunkPos.y - 3); --y) {
                glm::ivec3 lowerChunkPos(chunkPos.x, y, chunkPos.z);
                auto lowerIt = chunks.find(lowerChunkPos);
                if (lowerIt != chunks.end() && !lowerIt->second->isEmpty()) {
                    // There's a non-empty chunk below this one, so this might contain content
                    return false; // Don't skip - try to load this chunk
                }
            }
            
            // Skip if no nearby non-empty chunks were found
            return true;
        }
    }
    
    // For loaded chunks, check if they have content
    if (it != chunks.end()) {
        Chunk* chunk = it->second.get();
        if (!chunk) {
            return true; // Skip invalid chunks
        }
        
        // Always render chunks that are known to have content
        if (!chunk->isEmpty()) {
            return false; // Don't skip chunks with content
        }
    }
    
    // Default behavior - skip chunks that are likely empty
    return true;
} 