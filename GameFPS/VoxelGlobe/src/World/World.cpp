// ./src/World/World.cpp
#include "World/World.hpp"
#include <cmath>
#include <iostream>
#include "Debug/DebugManager.hpp"
#include <vector>
#include <algorithm>
#include <set>

// Define surface radius offset directly here to avoid requiring DebugHelper.hpp
const float SURFACE_RADIUS_OFFSET = 8.0f;

// Get the surface radius consistently
float World::getSurfaceRadius() const {
    return radius + SURFACE_RADIUS_OFFSET;
}

void World::update(const glm::vec3& playerPos) {
    static int frameCounter = 0;
    frameCounter++;

    // Calculate integer chunk coordinates based on player position
    int px = static_cast<int>(floor(playerPos.x / static_cast<float>(Chunk::SIZE)));
    int py = static_cast<int>(floor(playerPos.y / static_cast<float>(Chunk::SIZE)));
    int pz = static_cast<int>(floor(playerPos.z / static_cast<float>(Chunk::SIZE)));

    // Store the origin for relative coordinates
    localOrigin = glm::ivec3(px, py, pz);

    // Add chunk loading limit to prevent excessive regeneration
    const int MAX_NEW_CHUNKS_PER_FRAME = 2; // Increased from 1 to 2 chunks per frame
    int newChunksCreated = 0;

    // Debug log player position
    if (DebugManager::getInstance().logPlayerInfo() && frameCounter % 60 == 0) {
        std::cout << "Player position: " << playerPos.x << ", " << playerPos.y << ", " << playerPos.z << std::endl;
        std::cout << "Player chunk coords: " << px << ", " << py << ", " << pz << std::endl;
        std::cout << "Setting local origin to: (" << localOrigin.x << ", " << localOrigin.y << ", " << localOrigin.z << ")" << std::endl;
        
        // Debug the distance from center for sanity check
        double px_d = static_cast<double>(playerPos.x);
        double py_d = static_cast<double>(playerPos.y);
        double pz_d = static_cast<double>(playerPos.z);
        double distFromCenter = sqrt(px_d*px_d + py_d*py_d + pz_d*pz_d);
        std::cout << "Player distance from center: " << distFromCenter 
                 << ", Surface at: " << getSurfaceRadius() 
                 << ", Height above surface: " << (distFromCenter - getSurfaceRadius()) << std::endl;
    }

    // Create a list of chunks to create/update, ordered by priority
    std::vector<std::tuple<int, int, int, int, float>> chunkCandidates;

    // Generate the 3x3x3 grid (27 chunks) around player
    // Use a strict 3x3x3 grid without any distance-based criteria
    for (int x = px - 1; x <= px + 1; x++) {
        for (int y = py - 1; y <= py + 1; y++) {
            for (int z = pz - 1; z <= pz + 1; z++) {
                // Calculate distance to player's chunk for sorting
                int dx = x - px;
                int dy = y - py;
                int dz = z - pz;
                float dist = sqrt(dx*dx + dy*dy + dz*dz);
                chunkCandidates.emplace_back(x, y, z, 1, dist);
            }
        }
    }

    // Sort by distance from player for priority
    std::sort(chunkCandidates.begin(), chunkCandidates.end(),
              [](const auto& a, const auto& b) { return std::get<4>(a) < std::get<4>(b); });

    // Create new chunk map
    std::unordered_map<std::tuple<int, int, int, int>, std::unique_ptr<Chunk>, quad_hash> newChunks;
    
    // Process the 27 closest chunks
    for (const auto& candidate : chunkCandidates) {
        auto [x, y, z, mf, dist] = candidate;
        auto key = std::make_tuple(x, y, z, mf);
        auto it = chunks.find(key);

        if (it != chunks.end()) {
            // Move existing chunk to newChunks
            newChunks.emplace(key, std::move(it->second));
        } else {
            // Skip creating too many chunks in one frame
            if (newChunksCreated >= MAX_NEW_CHUNKS_PER_FRAME) {
                if (DebugManager::getInstance().logChunkUpdates()) {
                    std::cout << "Deferring chunk creation at (" << x << ", " << y << ", " << z 
                             << ") to avoid excessive regeneration" << std::endl;
                }
                continue;
            }
            
            // Create new chunk with careful validation
            try {
                // Create new chunk
                newChunks[key] = std::make_unique<Chunk>(x, y, z, mf);
                newChunks[key]->setWorld(this);
                
                // Generate terrain in a try block
                try {
                    newChunks[key]->generateTerrain();
                }
                catch (const std::exception& e) {
                    std::cerr << "Exception during terrain generation: " << e.what() << std::endl;
                }
                catch (...) {
                    std::cerr << "Unknown exception during terrain generation" << std::endl;
                }
                
                newChunksCreated++;
                
                if (DebugManager::getInstance().logChunkUpdates()) {
                    std::cout << "Created new chunk at (" << x << ", " << y << ", " << z << ")" << std::endl;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Exception creating chunk: " << e.what() << std::endl;
            }
            catch (...) {
                std::cerr << "Unknown exception creating chunk" << std::endl;
            }
        }

        // Ensure chunk is properly initialized
        auto& chunk = newChunks[key];
        if (chunk && !chunk->isBuffersInitialized()) {
            try {
                chunk->initializeBuffers();
            } catch (...) {
                std::cerr << "Exception during buffer initialization for chunk (" 
                          << x << ", " << y << ", " << z << ")" << std::endl;
            }
        }
        
        // Force regeneration of mesh if needed
        if (chunk && chunk->isMeshDirty()) {
            try {
                if (DebugManager::getInstance().logChunkUpdates()) {
                    std::cout << "Regenerating mesh for chunk at (" << x << ", " << y << ", " << z << ")" << std::endl;
                }
                chunk->regenerateMesh();
            } catch (...) {
                std::cerr << "Exception during mesh regeneration for chunk (" 
                          << x << ", " << y << ", " << z << ")" << std::endl;
            }
        }
    }

    // Replace old chunks with new ones
    chunks = std::move(newChunks);

    // Debug output
    if (DebugManager::getInstance().logChunkUpdates() && frameCounter % 60 == 0) {
        size_t totalMemory = 0;
        int chunksWithMesh = 0;
        
        for (const auto& [key, chunk] : chunks) {
            totalMemory += chunk->getMesh().size() * sizeof(float) +
                           (chunk->getMergeFactor() == 1 ? Chunk::SIZE * Chunk::SIZE * Chunk::SIZE * sizeof(Block) : 0);
            
            if (chunk->getMesh().size() > 0) {
                chunksWithMesh++;
            }
            
            if (DebugManager::getInstance().logChunkUpdates() && chunksWithMesh < 5) {
                std::cout << "Loaded chunk (" << std::get<0>(key) << ", " << std::get<1>(key) << ", " << std::get<2>(key)
                      << "), MergeFactor: " << chunk->getMergeFactor() << ", Vertices: " << chunk->getMesh().size() / 5 << std::endl;
            }
        }
        std::cout << "Chunks updated, count: " << chunks.size() 
                  << ", Chunks with mesh: " << chunksWithMesh
                  << ", Est. Memory: " << totalMemory / (1024.0f * 1024.0f) << " MB" << std::endl;
        std::cout << "Player chunk coords: (" << px << ", " << py << ", " << pz << ")" << std::endl;
    }
}

glm::vec3 World::cubeToSphere(int face, int x, int z, float y) const {
    // Convert from cube coordinates to spherical coordinates
    // Calculate normalized coordinates in the range [-1, 1]
    float nx = x / (radius * 0.5f);
    float ny = y / (radius * 0.5f);
    float nz = z / (radius * 0.5f);
    
    // Calculate the distance from the origin in cube space
    float cubeLength = std::max(std::max(std::abs(nx), std::abs(ny)), std::abs(nz));
    
    // Project onto sphere surface by normalizing and scaling by radius
    if (cubeLength > 0) {
        float sphereX = nx / cubeLength * radius;
        float sphereY = ny / cubeLength * radius;
        float sphereZ = nz / cubeLength * radius;
        return glm::vec3(sphereX, sphereY, sphereZ);
    }
    
    // Fallback to original coordinates if at origin
    return glm::vec3(x, y, z);
}

float World::findSurfaceHeight(float chunkX, float chunkZ) const {
    // Return fixed surface height using our consistent method
    return getSurfaceRadius();
}

void World::setBlock(int worldX, int worldY, int worldZ, BlockType type) {
    // Simple mapping to chunk coordinates
    int chunkX = static_cast<int>(floor(worldX / static_cast<float>(Chunk::SIZE)));
    int chunkY = static_cast<int>(floor(worldY / static_cast<float>(Chunk::SIZE)));
    int chunkZ = static_cast<int>(floor(worldZ / static_cast<float>(Chunk::SIZE)));
    
    // Calculate local block position within chunk
    int localX = worldX - chunkX * Chunk::SIZE;
    int localY = worldY - chunkY * Chunk::SIZE;
    int localZ = worldZ - chunkZ * Chunk::SIZE;
    
    // Handle negative local coordinates properly
    if (localX < 0) { localX += Chunk::SIZE; chunkX--; }
    if (localY < 0) { localY += Chunk::SIZE; chunkY--; }
    if (localZ < 0) { localZ += Chunk::SIZE; chunkZ--; }

    if (DebugManager::getInstance().logBlockPlacement()) {
        std::cout << "Setting block at world (" << worldX << ", " << worldY << ", " << worldZ 
                << ") -> chunk (" << chunkX << ", " << chunkY << ", " << chunkZ 
                << ") local (" << localX << ", " << localY << ", " << localZ << ")" << std::endl;
                
        // Add debug info about current local origin
        std::cout << "Current local origin: (" << localOrigin.x << ", " << localOrigin.y << ", " << localOrigin.z << ")" << std::endl;
        
        // Log distance from center for validation
        double wx = static_cast<double>(worldX);
        double wy = static_cast<double>(worldY);
        double wz = static_cast<double>(worldZ);
        double dist = sqrt(wx*wx + wy*wy + wz*wz);
        std::cout << "Block world position distance from center: " << dist << std::endl;
        
        // Calculate and log the planet-relative height
        float surfaceHeight = getSurfaceRadius();
        std::cout << "Block height relative to surface: " << (dist - surfaceHeight) << std::endl;
        
        // Check if the position is valid for editing based on distance from center
        bool isValidRange = (dist >= radius - 5.0f) && (dist <= getSurfaceRadius() + 15.0f);
        std::cout << "Is valid edit range: " << (isValidRange ? "Yes" : "No") 
                  << " (valid range: " << (radius - 5.0f) << " to " << (getSurfaceRadius() + 15.0f) << ")" << std::endl;
    }

    // Track all chunks that need to be updated
    std::set<std::tuple<int, int, int>> chunksToUpdate;
    
    // Add the primary chunk
    chunksToUpdate.insert(std::make_tuple(chunkX, chunkY, chunkZ));
    
    // Add neighboring chunks if we're at a chunk boundary
    if (localX == 0) chunksToUpdate.insert(std::make_tuple(chunkX - 1, chunkY, chunkZ));
    if (localX == Chunk::SIZE - 1) chunksToUpdate.insert(std::make_tuple(chunkX + 1, chunkY, chunkZ));
    if (localY == 0) chunksToUpdate.insert(std::make_tuple(chunkX, chunkY - 1, chunkZ));
    if (localY == Chunk::SIZE - 1) chunksToUpdate.insert(std::make_tuple(chunkX, chunkY + 1, chunkZ));
    if (localZ == 0) chunksToUpdate.insert(std::make_tuple(chunkX, chunkY, chunkZ - 1));
    if (localZ == Chunk::SIZE - 1) chunksToUpdate.insert(std::make_tuple(chunkX, chunkY, chunkZ + 1));

    // First set the block in the primary chunk
    auto key = std::make_tuple(chunkX, chunkY, chunkZ, 1);
    auto it = chunks.find(key);
    if (it == chunks.end()) {
        // Create new chunk if needed
        try {
            auto result = chunks.emplace(key, std::make_unique<Chunk>(chunkX, chunkY, chunkZ, 1));
            if (result.second) {
                result.first->second->setWorld(this);
                result.first->second->generateTerrain();
                it = result.first;
            } else {
                std::cerr << "Failed to create chunk for block placement" << std::endl;
                return;
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception creating chunk for block placement: " << e.what() << std::endl;
            return;
        } catch (...) {
            std::cerr << "Unknown exception creating chunk for block placement" << std::endl;
            return;
        }
    }
    
    // Verify the block position is within the valid editing range
    // This ensures players can only edit from below crust to 15km above surface
    double wx = static_cast<double>(worldX);
    double wy = static_cast<double>(worldY);
    double wz = static_cast<double>(worldZ);
    double dist = sqrt(wx*wx + wy*wy + wz*wz);
    float surfaceHeight = getSurfaceRadius();
    
    if (dist < radius - 5.0f || dist > surfaceHeight + 15.0f) {
        std::cerr << "Block position outside valid editing range: " << dist 
                 << " (valid range: " << (radius - 5.0f) << " to " << (surfaceHeight + 15.0f) << ")" << std::endl;
        return;
    }
    
    // Set the block in the primary chunk
    try {
        std::cout << "Setting block in chunk at local (" << localX << ", " << localY << ", " << localZ 
                  << ") to type " << static_cast<int>(type) << std::endl;
        it->second->setBlock(localX, localY, localZ, type);
    } catch (const std::exception& e) {
        std::cerr << "Exception setting block: " << e.what() << std::endl;
        return;
    } catch (...) {
        std::cerr << "Unknown exception setting block" << std::endl;
        return;
    }
    
    // Now update all affected chunks
    for (const auto& chunkCoords : chunksToUpdate) {
        auto key = std::make_tuple(std::get<0>(chunkCoords), std::get<1>(chunkCoords), std::get<2>(chunkCoords), 1);
        auto it = chunks.find(key);
        
        if (it != chunks.end()) {
            try {
                std::cout << "Updating chunk at " << std::get<0>(chunkCoords) << ", " 
                          << std::get<1>(chunkCoords) << ", " << std::get<2>(chunkCoords) << std::endl;
                
                // Force immediate mesh regeneration
                it->second->markMeshDirty();
                it->second->regenerateMesh();
                
                // Ensure buffers are updated for rendering AND collision
                if (it->second->isBuffersInitialized()) {
                    it->second->updateBuffers();
                } else {
                    it->second->initializeBuffers();
                }
                
                std::cout << "Updated collision geometry for chunk (" 
                         << std::get<0>(chunkCoords) << ", " 
                         << std::get<1>(chunkCoords) << ", " 
                         << std::get<2>(chunkCoords) << ")" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Exception updating chunk: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown exception updating chunk" << std::endl;
            }
        }
    }
    
    if (DebugManager::getInstance().logBlockPlacement()) {
        std::cout << "Block update complete - updated " << chunksToUpdate.size() << " chunks" << std::endl;
        
        // Indicate what happened to the block
        if (type == BlockType::AIR) {
            std::cout << "Removed block at (" << worldX << ", " << worldY << ", " << worldZ << ")" << std::endl;
        } else {
            std::cout << "Placed " << static_cast<int>(type) << " block at (" << worldX << ", " << worldY << ", " << worldZ << ")" << std::endl;
        }
    }
}

Block World::getBlock(int worldX, int worldY, int worldZ) const {
    // Simple mapping to chunk coordinates
    int chunkX = static_cast<int>(floor(worldX / static_cast<float>(Chunk::SIZE)));
    int chunkY = static_cast<int>(floor(worldY / static_cast<float>(Chunk::SIZE)));
    int chunkZ = static_cast<int>(floor(worldZ / static_cast<float>(Chunk::SIZE)));
    
    // Calculate local block position within chunk
    int localX = worldX - chunkX * Chunk::SIZE;
    int localY = worldY - chunkY * Chunk::SIZE;
    int localZ = worldZ - chunkZ * Chunk::SIZE;
    
    // Handle negative local coordinates properly
    if (localX < 0) { localX += Chunk::SIZE; chunkX--; }
    if (localY < 0) { localY += Chunk::SIZE; chunkY--; }
    if (localZ < 0) { localZ += Chunk::SIZE; chunkZ--; }

    // Sanity check for extreme coordinates
    if (abs(chunkX) > 10000 || abs(chunkY) > 10000 || abs(chunkZ) > 10000) {
        // If coordinates are extremely large, use double precision distance calculation
        double wx = static_cast<double>(worldX);
        double wy = static_cast<double>(worldY);
        double wz = static_cast<double>(worldZ);
        double distFromCenter = sqrt(wx*wx + wy*wy + wz*wz);
        float surfaceHeight = getSurfaceRadius();
        
        // Return block type based on distance from center
        if (distFromCenter < surfaceHeight - 5.0f) {
            return Block(BlockType::DIRT);
        } else if (distFromCenter < surfaceHeight) {
            return Block(BlockType::GRASS);
        }
        return Block(BlockType::AIR);
    }

    // Find the chunk
    auto it = chunks.find(std::make_tuple(chunkX, chunkY, chunkZ, 1));
    if (it != chunks.end()) {
        return it->second->getBlock(localX, localY, localZ);
    }
    
    // If chunk doesn't exist, determine block type based on distance from center
    // using double precision for accurate calculation
    double wx = static_cast<double>(worldX);
    double wy = static_cast<double>(worldY);
    double wz = static_cast<double>(worldZ);
    double distFromCenter = sqrt(wx*wx + wy*wy + wz*wz);
    float surfaceHeight = getSurfaceRadius();
    
    if (distFromCenter < surfaceHeight - 5.0f) {
        return Block(BlockType::DIRT);
    } else if (distFromCenter < surfaceHeight) {
        return Block(BlockType::GRASS);
    }
    return Block(BlockType::AIR);
}

std::unordered_map<std::tuple<int, int, int, int>, std::unique_ptr<Chunk>, quad_hash>& World::getChunks() {
    return chunks;
}

const std::unordered_map<std::tuple<int, int, int, int>, std::unique_ptr<Chunk>, quad_hash>& World::getChunks() const {
    return chunks;
}