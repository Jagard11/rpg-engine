// ./src/World/World.cpp
#include "World/World.hpp"
#include <cmath>
#include <iostream>
#include "Debug/DebugManager.hpp"
#include <vector>
#include <algorithm>

void World::update(const glm::vec3& playerPos) {
    static int frameCounter = 0;
    frameCounter++;

    // For debugging, calculate integer chunk coordinates based directly on player position
    // This simpler mapping helps isolate coordinate system issues
    int px = static_cast<int>(floor(playerPos.x / Chunk::SIZE));
    int py = static_cast<int>(floor((playerPos.y) / Chunk::SIZE)); // No radius offset for now
    int pz = static_cast<int>(floor(playerPos.z / Chunk::SIZE));

    // Store the origin for relative coordinates
    localOrigin = glm::ivec3(px, py, pz);

    // Debug log player position
    if (DebugManager::getInstance().logPlayerInfo() && frameCounter % 60 == 0) {
        std::cout << "Player position: " << playerPos.x << ", " << playerPos.y << ", " << playerPos.z << std::endl;
        std::cout << "Player chunk coords: " << px << ", " << py << ", " << pz << std::endl;
    }

    std::vector<std::tuple<int, int, int, int, float>> chunkCandidates;

    // Generate the 3x3x3 grid (27 chunks) around player
    const int renderDistance = 1; // 1 chunk in each direction = 3x3x3 grid
    for (int x = px - renderDistance; x <= px + renderDistance; x++) {
        for (int y = py - renderDistance; y <= py + renderDistance; y++) {
            for (int z = pz - renderDistance; z <= pz + renderDistance; z++) {
                float dist = sqrt(pow(x - px, 2) + pow(y - py, 2) + pow(z - pz, 2));
                chunkCandidates.emplace_back(x, y, z, 1, dist); // mergeFactor = 1 for full detail
            }
        }
    }

    // Sort by distance from player
    std::sort(chunkCandidates.begin(), chunkCandidates.end(),
              [](const auto& a, const auto& b) { return std::get<4>(a) < std::get<4>(b); });

    // Create new chunk map
    std::unordered_map<std::tuple<int, int, int, int>, std::unique_ptr<Chunk>, quad_hash> newChunks;
    bool firstLoad = chunks.empty();
    
    // Process the 27 closest chunks
    for (const auto& candidate : chunkCandidates) {
        auto [x, y, z, mf, dist] = candidate;
        auto key = std::make_tuple(x, y, z, mf);
        auto it = chunks.find(key);

        if (it != chunks.end()) {
            // Move existing chunk to newChunks
            newChunks.emplace(key, std::move(it->second));
        } else {
            // Create new chunk
            newChunks[key] = std::make_unique<Chunk>(x, y, z, mf);
            newChunks[key]->setWorld(this);
            newChunks[key]->generateTerrain();
            
            if (DebugManager::getInstance().logChunkUpdates()) {
                std::cout << "Created new chunk at (" << x << ", " << y << ", " << z << ")" << std::endl;
            }
        }

        // Force terrain generation to ensure chunks have content
        auto& chunk = newChunks[key];
        chunk->generateTerrain();
        chunk->markMeshDirty();
        chunk->regenerateMesh();
    }

    // Replace old chunks with new ones
    chunks = std::move(newChunks);

    // Debug output
    if (DebugManager::getInstance().logChunkUpdates() && frameCounter % 60 == 0) {
        size_t totalMemory = 0;
        for (const auto& [key, chunk] : chunks) {
            totalMemory += chunk->getMesh().size() * sizeof(float) +
                           (chunk->getMergeFactor() == 1 ? Chunk::SIZE * Chunk::SIZE * Chunk::SIZE * sizeof(Block) : 0);
            std::cout << "Loaded chunk (" << std::get<0>(key) << ", " << std::get<1>(key) << ", " << std::get<2>(key)
                      << "), MergeFactor: " << chunk->getMergeFactor() << ", Vertices: " << chunk->getMesh().size() / 5 << std::endl;
        }
        std::cout << "Chunks updated, count: " << chunks.size()
                  << ", Est. Memory: " << totalMemory / (1024.0f * 1024.0f) << " MB" << std::endl;
        std::cout << "Player chunk coords: (" << px << ", " << py << ", " << pz << ")" << std::endl;
    }
}

glm::vec3 World::cubeToSphere(int face, int x, int z, float y) const {
    // For debugging, just return cartesian coordinates directly
    // This simplifies debugging and helps isolate coordinate system issues
    return glm::vec3(x, y, z);
}

float World::findSurfaceHeight(float chunkX, float chunkZ) const {
    // Return fixed surface height for simplicity (around y=1599.55)
    return radius + 8.0f;
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
    
    if (localX < 0) { localX += Chunk::SIZE; chunkX--; }
    if (localY < 0) { localY += Chunk::SIZE; chunkY--; }
    if (localZ < 0) { localZ += Chunk::SIZE; chunkZ--; }

    std::cout << "Setting block at world (" << worldX << ", " << worldY << ", " << worldZ 
              << ") -> chunk (" << chunkX << ", " << chunkY << ", " << chunkZ 
              << ") local (" << localX << ", " << localY << ", " << localZ << ")" << std::endl;

    // Find or create the chunk
    auto key = std::make_tuple(chunkX, chunkY, chunkZ, 1);
    auto it = chunks.find(key);
    if (it == chunks.end()) {
        // Chunk doesn't exist yet, create it
        std::cout << "Creating new chunk for block placement" << std::endl;
        auto result = chunks.emplace(key, std::make_unique<Chunk>(chunkX, chunkY, chunkZ, 1));
        result.first->second->setWorld(this);
        result.first->second->generateTerrain();
        it = result.first;
    }
    
    // Set the block
    it->second->setBlock(localX, localY, localZ, type);
    it->second->markMeshDirty();  // Make sure the mesh gets updated
    it->second->regenerateMesh(); // Force immediate mesh update

    // Mark neighboring chunks as dirty if the block is on a boundary
    for (int dx = -1; dx <= 1; dx += 2) {
        auto neighborKey = std::make_tuple(chunkX + dx, chunkY, chunkZ, 1);
        if (localX == (dx == -1 ? 0 : Chunk::SIZE - 1)) {
            auto neighbor = chunks.find(neighborKey);
            if (neighbor != chunks.end()) {
                neighbor->second->markMeshDirty();
                neighbor->second->regenerateMesh();
            }
            else {
                auto result = chunks.emplace(neighborKey, std::make_unique<Chunk>(chunkX + dx, chunkY, chunkZ, 1));
                result.first->second->setWorld(this);
                result.first->second->generateTerrain();
                result.first->second->markMeshDirty();
                result.first->second->regenerateMesh();
            }
        }
    }
    for (int dy = -1; dy <= 1; dy += 2) {
        auto neighborKey = std::make_tuple(chunkX, chunkY + dy, chunkZ, 1);
        if (localY == (dy == -1 ? 0 : Chunk::SIZE - 1)) {
            auto neighbor = chunks.find(neighborKey);
            if (neighbor != chunks.end()) {
                neighbor->second->markMeshDirty();
                neighbor->second->regenerateMesh();
            }
            else {
                auto result = chunks.emplace(neighborKey, std::make_unique<Chunk>(chunkX, chunkY + dy, chunkZ, 1));
                result.first->second->setWorld(this);
                result.first->second->generateTerrain();
                result.first->second->markMeshDirty();
                result.first->second->regenerateMesh();
            }
        }
    }
    for (int dz = -1; dz <= 1; dz += 2) {
        auto neighborKey = std::make_tuple(chunkX, chunkY, chunkZ + dz, 1);
        if (localZ == (dz == -1 ? 0 : Chunk::SIZE - 1)) {
            auto neighbor = chunks.find(neighborKey);
            if (neighbor != chunks.end()) {
                neighbor->second->markMeshDirty();
                neighbor->second->regenerateMesh();
            }
            else {
                auto result = chunks.emplace(neighborKey, std::make_unique<Chunk>(chunkX, chunkY, chunkZ + dz, 1));
                result.first->second->setWorld(this);
                result.first->second->generateTerrain();
                result.first->second->markMeshDirty();
                result.first->second->regenerateMesh();
            }
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
    
    if (localX < 0) { localX += Chunk::SIZE; chunkX--; }
    if (localY < 0) { localY += Chunk::SIZE; chunkY--; }
    if (localZ < 0) { localZ += Chunk::SIZE; chunkZ--; }

    // Find the chunk
    auto it = chunks.find(std::make_tuple(chunkX, chunkY, chunkZ, 1));
    if (it != chunks.end()) {
        return it->second->getBlock(localX, localY, localZ);
    }
    
    // If chunk doesn't exist, determine block type based on fixed height
    float surfaceHeight = radius + 8.0f; // ~1599.55
    
    if (worldY < surfaceHeight - 1.0f) {
        return Block(BlockType::DIRT);
    } else if (worldY < surfaceHeight) {
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