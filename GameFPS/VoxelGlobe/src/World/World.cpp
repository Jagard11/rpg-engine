// ./src/World/World.cpp
#include "World/World.hpp"
#include <cmath>
#include <iostream>
#include "Debug/DebugManager.hpp"

World::World() : radius(6371000.0f), localOrigin(0, 0, 0) {}

void World::update(const glm::vec3& playerPos) {
    localOrigin = glm::ivec3(floor(playerPos.x / Chunk::SIZE) * Chunk::SIZE, 
                             floor(playerPos.y / Chunk::SIZE) * Chunk::SIZE, 
                             floor(playerPos.z / Chunk::SIZE) * Chunk::SIZE);
    int px = static_cast<int>(playerPos.x / Chunk::SIZE);
    int pz = static_cast<int>(playerPos.z / Chunk::SIZE);
    int renderDist = 64; // Increased to ~1000 units for visibility

    px = glm::clamp(px, -1000, 1000);
    pz = glm::clamp(pz, -1000, 1000);

    std::unordered_map<std::pair<int, int>, Chunk, pair_hash> newChunks;
    for (int face = 0; face < 6; face++) {
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

    if (DebugManager::getInstance().logChunkUpdates()) {
        std::cout << "Chunks updated, count: " << chunks.size() << " around player (" << playerPos.x << ", " << playerPos.y << ", " << playerPos.z << ")" << std::endl;
    }
}

glm::vec3 World::cubeToSphere(int face, int x, int z, float y) const {
    float scale = Chunk::SIZE;
    float bx = x * scale;
    float bz = z * scale;
    float by = y; // Simplified: no radius offset, keep local to player
    glm::vec3 pos;

    // Simplified mapping: place chunks in a flat grid for now, adjust later
    switch (face) {
        case 0: pos = glm::vec3(bx, by, bz); break; // Top face as flat plane
        case 1: pos = glm::vec3(bx, by - 16.0f, bz); break; // Bottom
        case 2: pos = glm::vec3(bx + 16.0f, by, bz); break; // Right
        case 3: pos = glm::vec3(bx - 16.0f, by, bz); break; // Left
        case 4: pos = glm::vec3(bx, by, bz + 16.0f); break; // Front
        case 5: pos = glm::vec3(bx, by, bz - 16.0f); break; // Back
    }

    if (DebugManager::getInstance().logChunkUpdates()) {
        std::cout << "Chunk mapped: face=" << face << ", x=" << x << ", z=" << z << ", pos=(" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
    }
    return pos;
}

float World::findSurfaceHeight(float chunkX, float chunkZ) const {
    int intChunkX = static_cast<int>(chunkX);
    int intChunkZ = static_cast<int>(chunkZ);
    auto it = chunks.find(std::make_pair(intChunkX, intChunkZ));
    if (it == chunks.end()) {
        return 8.0f; // Default surface height
    }
    const Chunk& chunk = it->second;
    for (int y = Chunk::SIZE - 1; y >= 0; --y) {
        BlockType type = chunk.getBlock(8, y, 8).type;
        if (type != BlockType::AIR) {
            return y;
        }
    }
    return 8.0f;
}

void World::setBlock(int worldX, int worldY, int worldZ, BlockType type) {
    int chunkX = worldX / Chunk::SIZE;
    int chunkZ = worldZ / Chunk::SIZE;
    int localX = worldX % Chunk::SIZE;
    int localZ = worldZ % Chunk::SIZE;
    int localY = worldY % Chunk::SIZE;
    if (localX < 0) { localX += Chunk::SIZE; chunkX--; }
    if (localZ < 0) { localZ += Chunk::SIZE; chunkZ--; }
    if (localY < 0) { localY += Chunk::SIZE; }

    auto key = std::make_pair(chunkX, chunkZ);
    auto it = chunks.find(key);
    if (it == chunks.end()) {
        auto result = chunks.emplace(key, Chunk(chunkX, chunkZ));
        it = result.first;
    }
    it->second.setBlock(localX, localY, localZ, type);

    it->second.regenerateMesh(0);

    if (localX == 0) {
        auto neighborKey = std::make_pair(chunkX - 1, chunkZ);
        auto neighbor = chunks.find(neighborKey);
        if (neighbor != chunks.end()) neighbor->second.regenerateMesh(0);
        else chunks.emplace(neighborKey, Chunk(chunkX - 1, chunkZ)).first->second.regenerateMesh(0);
    }
    if (localX == Chunk::SIZE - 1) {
        auto neighborKey = std::make_pair(chunkX + 1, chunkZ);
        auto neighbor = chunks.find(neighborKey);
        if (neighbor != chunks.end()) neighbor->second.regenerateMesh(0);
        else chunks.emplace(neighborKey, Chunk(chunkX + 1, chunkZ)).first->second.regenerateMesh(0);
    }
    if (localZ == 0) {
        auto neighborKey = std::make_pair(chunkX, chunkZ - 1);
        auto neighbor = chunks.find(neighborKey);
        if (neighbor != chunks.end()) neighbor->second.regenerateMesh(0);
        else chunks.emplace(neighborKey, Chunk(chunkX, chunkZ - 1)).first->second.regenerateMesh(0);
    }
    if (localZ == Chunk::SIZE - 1) {
        auto neighborKey = std::make_pair(chunkX, chunkZ + 1);
        auto neighbor = chunks.find(neighborKey);
        if (neighbor != chunks.end()) neighbor->second.regenerateMesh(0);
        else chunks.emplace(neighborKey, Chunk(chunkX, chunkZ + 1)).first->second.regenerateMesh(0);
    }
}

Block World::getBlock(int worldX, int worldY, int worldZ) const {
    int chunkX = worldX / Chunk::SIZE;
    int chunkZ = worldZ / Chunk::SIZE;
    int localX = worldX % Chunk::SIZE;
    int localZ = worldZ % Chunk::SIZE;
    int localY = worldY % Chunk::SIZE;
    if (localX < 0) { localX += Chunk::SIZE; chunkX--; }
    if (localZ < 0) { localZ += Chunk::SIZE; chunkZ--; }
    if (localY < 0) { localY += Chunk::SIZE; }

    auto it = chunks.find(std::make_pair(chunkX, chunkZ));
    if (it != chunks.end() && localY >= 0 && localY < Chunk::SIZE) {
        return it->second.getBlock(localX, localY, localZ);
    }
    return Block(BlockType::AIR);
}

glm::ivec3 World::getLocalOrigin() const {
    return localOrigin;
}

float World::getRadius() const {
    return radius;
}