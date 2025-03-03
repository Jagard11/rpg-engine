// ./src/World/World.cpp
#include "World/World.hpp"
#include <cmath>
#include <iostream>
#include "Core/Debug.hpp"

void World::update(const glm::vec3& playerPos) {
    int px = static_cast<int>(playerPos.x / Chunk::SIZE);
    int pz = static_cast<int>(playerPos.z / Chunk::SIZE);
    int renderDist = 8;

    px = glm::clamp(px, -1000, 1000);
    pz = glm::clamp(pz, -1000, 1000);

    std::unordered_map<std::pair<int, int>, Chunk, pair_hash> newChunks;
    for (int face = 0; face < 1; face++) {
        for (int x = px - renderDist; x <= px + renderDist; x++) {
            for (int z = pz - renderDist; z <= pz + renderDist; z++) {
                auto key = std::make_pair(x + face * 1000, z);
                auto it = chunks.find(key);
                if (it != chunks.end()) {
                    newChunks.emplace(key, std::move(it->second));
                } else {
                    newChunks.emplace(key, Chunk(x, z));
                }
            }
        }
    }
    chunks = std::move(newChunks);

    if (g_showDebug) {
        std::cout << "Chunks updated, count: " << chunks.size() << std::endl;
    }
}

glm::vec3 World::cubeToSphere(int face, int x, int z, float y) const {
    float scale = 1.0f;
    float bx = x * Chunk::SIZE * scale;
    float bz = z * Chunk::SIZE * scale;
    glm::vec3 pos(bx, 1500.0f + y, bz); // y is local offset (0-255)
    if (g_showDebug) {
        std::cout << "Chunk Pos: " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
    }
    return pos;
}

float World::findSurfaceHeight(float chunkX, float chunkZ) const {
    int intChunkX = static_cast<int>(chunkX);
    int intChunkZ = static_cast<int>(chunkZ);
    auto it = chunks.find(std::make_pair(intChunkX, intChunkZ));
    if (it == chunks.end()) {
        if (g_showDebug) std::cout << "Chunk (" << intChunkX << ", " << intChunkZ << ") not found, defaulting to 1508" << std::endl;
        return 1508.0f;
    }
    const Chunk& chunk = it->second;
    for (int y = Chunk::SIZE - 1; y >= 0; --y) {
        BlockType type = chunk.getBlock(8, y, 8).type;
        if (type != BlockType::AIR) {
            float surfaceHeight = 1500.0f + y;
            if (g_showDebug) std::cout << "Surface at y = " << y << " in chunk (" << intChunkX << ", " << intChunkZ 
                                      << "), world height = " << surfaceHeight << std::endl;
            return surfaceHeight;
        }
    }
    if (g_showDebug) std::cout << "No surface in chunk (" << intChunkX << ", " << intChunkZ << "), defaulting to 1508" << std::endl;
    return 1508.0f;
}

void World::setBlock(int worldX, int worldY, int worldZ, BlockType type) {
    if (worldY < 1500 || worldY > 1755) {
        if (g_showDebug) std::cout << "SetBlock out of bounds: Y=" << worldY << std::endl;
        return;
    }

    int chunkX = worldX / Chunk::SIZE;
    int chunkZ = worldZ / Chunk::SIZE;
    int localX = worldX % Chunk::SIZE;
    int localZ = worldZ % Chunk::SIZE;
    int localY = worldY - 1500; // Convert world Y to local Y (FLOOR_HEIGHT = 1500)
    if (localX < 0) { localX += Chunk::SIZE; chunkX--; }
    if (localZ < 0) { localZ += Chunk::SIZE; chunkZ--; }

    auto key = std::make_pair(chunkX, chunkZ);
    auto it = chunks.find(key);
    if (it == chunks.end()) {
        auto result = chunks.emplace(key, Chunk(chunkX, chunkZ));
        it = result.first;
    }
    it->second.setBlock(localX, localY, localZ, type);

    // Regenerate mesh for this chunk
    it->second.regenerateMesh();

    // Regenerate adjacent chunks if on boundary
    if (localX == 0) {
        auto neighborKey = std::make_pair(chunkX - 1, chunkZ);
        auto neighbor = chunks.find(neighborKey);
        if (neighbor != chunks.end()) neighbor->second.regenerateMesh();
        else chunks.emplace(neighborKey, Chunk(chunkX - 1, chunkZ)).first->second.regenerateMesh();
    }
    if (localX == Chunk::SIZE - 1) {
        auto neighborKey = std::make_pair(chunkX + 1, chunkZ);
        auto neighbor = chunks.find(neighborKey);
        if (neighbor != chunks.end()) neighbor->second.regenerateMesh();
        else chunks.emplace(neighborKey, Chunk(chunkX + 1, chunkZ)).first->second.regenerateMesh();
    }
    if (localZ == 0) {
        auto neighborKey = std::make_pair(chunkX, chunkZ - 1);
        auto neighbor = chunks.find(neighborKey);
        if (neighbor != chunks.end()) neighbor->second.regenerateMesh();
        else chunks.emplace(neighborKey, Chunk(chunkX, chunkZ - 1)).first->second.regenerateMesh();
    }
    if (localZ == Chunk::SIZE - 1) {
        auto neighborKey = std::make_pair(chunkX, chunkZ + 1);
        auto neighbor = chunks.find(neighborKey);
        if (neighbor != chunks.end()) neighbor->second.regenerateMesh();
        else chunks.emplace(neighborKey, Chunk(chunkX, chunkZ + 1)).first->second.regenerateMesh();
    }
}

Block World::getBlock(int worldX, int worldY, int worldZ) const {
    int chunkX = worldX / Chunk::SIZE;
    int chunkZ = worldZ / Chunk::SIZE;
    int localX = worldX % Chunk::SIZE;
    int localZ = worldZ % Chunk::SIZE;
    int localY = worldY - 1500; // Convert world Y to local Y (FLOOR_HEIGHT = 1500)
    if (localX < 0) { localX += Chunk::SIZE; chunkX--; }
    if (localZ < 0) { localZ += Chunk::SIZE; chunkZ--; }

    auto it = chunks.find(std::make_pair(chunkX, chunkZ));
    if (it != chunks.end() && localY >= 0 && localY < Chunk::SIZE) {
        return it->second.getBlock(localX, localY, localZ);
    }
    return Block(BlockType::AIR);
}