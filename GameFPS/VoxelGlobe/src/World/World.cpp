// ./VoxelGlobe/src/World.cpp
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
    glm::vec3 pos(bx, radius + y, bz);
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
        if (g_showDebug) std::cout << "Chunk (" << intChunkX << ", " << intChunkZ << ") not found" << std::endl;
        return radius;
    }
    const Chunk& chunk = it->second;
    for (int y = Chunk::SIZE - 1; y >= 0; --y) {
        BlockType type = chunk.getBlock(8, y, 8).type;
        if (type != BlockType::AIR) {
            if (g_showDebug) std::cout << "Surface at y = " << y << " in chunk (" << intChunkX << ", " << intChunkZ << ")" << std::endl;
            return radius + y * 1.0f;
        }
    }
    if (g_showDebug) std::cout << "No surface in chunk (" << intChunkX << ", " << intChunkZ << ")" << std::endl;
    return radius;
}

void World::setBlock(int worldX, int worldY, int worldZ, BlockType type) {
    int chunkX = worldX / Chunk::SIZE;
    int chunkZ = worldZ / Chunk::SIZE;
    int localX = worldX % Chunk::SIZE;
    int localZ = worldZ % Chunk::SIZE;
    int localY = worldY - static_cast<int>(radius + 8.0f); // Convert world Y to local Y
    if (localX < 0) { localX += Chunk::SIZE; chunkX--; }
    if (localZ < 0) { localZ += Chunk::SIZE; chunkZ--; }

    if (localY < 0 || localY >= Chunk::SIZE) {
        if (g_showDebug) std::cout << "SetBlock out of bounds: Y=" << localY << std::endl;
        return;
    }

    auto key = std::make_pair(chunkX, chunkZ);
    auto it = chunks.find(key);
    if (it == chunks.end()) {
        auto result = chunks.emplace(key, Chunk(chunkX, chunkZ));
        it = result.first;
    }
    it->second.setBlock(localX, localY, localZ, type);

    if (localX == 0) {
        auto neighborKey = std::make_pair(chunkX - 1, chunkZ);
        auto neighbor = chunks.find(neighborKey);
        if (neighbor != chunks.end()) neighbor->second.regenerateMesh();
        else chunks.emplace(neighborKey, Chunk(chunkX - 1, chunkZ));
    }
    if (localX == Chunk::SIZE - 1) {
        auto neighborKey = std::make_pair(chunkX + 1, chunkZ);
        auto neighbor = chunks.find(neighborKey);
        if (neighbor != chunks.end()) neighbor->second.regenerateMesh();
        else chunks.emplace(neighborKey, Chunk(chunkX + 1, chunkZ));
    }
    if (localZ == 0) {
        auto neighborKey = std::make_pair(chunkX, chunkZ - 1);
        auto neighbor = chunks.find(neighborKey);
        if (neighbor != chunks.end()) neighbor->second.regenerateMesh();
        else chunks.emplace(neighborKey, Chunk(chunkX, chunkZ - 1));
    }
    if (localZ == Chunk::SIZE - 1) {
        auto neighborKey = std::make_pair(chunkX, chunkZ + 1);
        auto neighbor = chunks.find(neighborKey);
        if (neighbor != chunks.end()) neighbor->second.regenerateMesh();
        else chunks.emplace(neighborKey, Chunk(chunkX, chunkZ + 1));
    }
}

Block World::getBlock(int worldX, int worldY, int worldZ) const {
    int chunkX = worldX / Chunk::SIZE;
    int chunkZ = worldZ / Chunk::SIZE;
    int localX = worldX % Chunk::SIZE;
    int localZ = worldZ % Chunk::SIZE;
    if (localX < 0) { localX += Chunk::SIZE; chunkX--; }
    if (localZ < 0) { localZ += Chunk::SIZE; chunkZ--; }

    auto it = chunks.find(std::make_pair(chunkX, chunkZ));
    if (it != chunks.end()) {
        return it->second.getBlock(localX, worldY, localZ);
    }
    return Block(BlockType::AIR);
}