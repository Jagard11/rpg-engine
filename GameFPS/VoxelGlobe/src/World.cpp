// ./GameFPS/VoxelGlobe/src/World.cpp
#include "World.hpp"
#include <cmath>
#include <iostream>
#include "Debug.hpp"

void World::update(const glm::vec3& playerPos) {
    static glm::vec3 lastPlayerPos = glm::vec3(-9999.0f); // Initial value far outside range
    int px = static_cast<int>(playerPos.x / Chunk::SIZE);
    int pz = static_cast<int>(playerPos.z / Chunk::SIZE);
    int lastPx = static_cast<int>(lastPlayerPos.x / Chunk::SIZE);
    int lastPz = static_cast<int>(lastPlayerPos.z / Chunk::SIZE);

    // Only update if player moves to a new chunk (or first run)
    if (px == lastPx && pz == lastPz && !chunks.empty()) {
        return;
    }
    lastPlayerPos = playerPos;

    int renderDist = 8;
    px = glm::clamp(px, -1000, 1000);
    pz = glm::clamp(pz, -1000, 1000);

    for (int face = 0; face < 1; face++) {
        for (int x = px - renderDist; x <= px + renderDist; x++) {
            for (int z = pz - renderDist; z <= pz + renderDist; z++) {
                std::pair<int, int> chunkKey = std::make_pair(x + face * 1000, z);
                if (chunks.find(chunkKey) == chunks.end()) {
                    chunks.emplace(chunkKey, Chunk(x, z));
                    if (g_showDebug) {
                        std::cout << "Added new chunk at (" << x << ", " << z << ")" << std::endl;
                    }
                }
            }
        }
    }
    if (g_showDebug) {
        std::cout << "Chunks updated, count: " << chunks.size() << std::endl;
    }
}

glm::vec3 World::cubeToSphere(int face, int x, int z, float y) const {
    float scale = 1.0f;
    float bx = x * Chunk::SIZE * scale;
    float bz = z * Chunk::SIZE * scale;
    float u = bx;
    float v = bz;
    glm::vec3 pos(u, radius + y * scale, v);
    return pos;
}

float World::findSurfaceHeight(int chunkX, int chunkZ) const {
    auto it = chunks.find(std::make_pair(chunkX, chunkZ));
    if (it == chunks.end()) {
        if (g_showDebug) std::cout << "Chunk (" << chunkX << ", " << chunkZ << ") not found" << std::endl;
        return radius;
    }
    const Chunk& chunk = it->second;
    for (int y = Chunk::SIZE - 1; y >= 0; --y) {
        BlockType type = chunk.getBlock(8, y, 8).type;
        if (type != BlockType::AIR) {
            if (g_showDebug) std::cout << "Surface at y = " << y << " in chunk (" << chunkX << ", " << chunkZ << ")" << std::endl;
            return radius + y * 1.0f;
        }
    }
    if (g_showDebug) std::cout << "No surface in chunk (" << chunkX << ", " << chunkZ << ")" << std::endl;
    return radius;
}

float World::findSurfaceHeight(float x, float z) const {
    int chunkX = static_cast<int>(x / Chunk::SIZE);
    int chunkZ = static_cast<int>(z / Chunk::SIZE);
    int localX = static_cast<int>(x) % Chunk::SIZE;
    int localZ = static_cast<int>(z) % Chunk::SIZE;
    if (localX < 0) localX += Chunk::SIZE;
    if (localZ < 0) localZ += Chunk::SIZE;

    auto it = chunks.find(std::make_pair(chunkX, chunkZ));
    if (it == chunks.end()) {
        if (g_showDebug) std::cout << "Chunk (" << chunkX << ", " << chunkZ << ") not found" << std::endl;
        return radius + 8.0f;
    }
    const Chunk& chunk = it->second;
    for (int y = Chunk::SIZE - 1; y >= 0; --y) {
        if (chunk.getBlock(localX, y, localZ).type != BlockType::AIR) {
            float surfaceY = radius + 8.0f + y + 1.0f;
            if (g_showDebug) std::cout << "Surface at y = " << y << " at (" << x << ", " << z << "), world y = " << surfaceY << std::endl;
            return surfaceY;
        }
    }
    return radius + 8.0f;
}

void World::setBlock(int worldX, int worldY, int worldZ, BlockType type) {
    int chunkX = worldX / Chunk::SIZE;
    int chunkZ = worldZ / Chunk::SIZE;
    int localX = worldX % Chunk::SIZE;
    int localZ = worldZ % Chunk::SIZE;
    if (localX < 0) localX += Chunk::SIZE;
    if (localZ < 0) localZ += Chunk::SIZE;

    std::pair<int, int> chunkKey(chunkX, chunkZ);
    auto it = chunks.find(chunkKey);
    if (it != chunks.end()) {
        it->second.setBlock(localX, worldY, localZ, type);
        if (g_showDebug) {
            std::cout << "Set block at (" << worldX << ", " << worldY << ", " << worldZ 
                      << ") to " << static_cast<int>(type) << " in chunk (" << chunkX << ", " << chunkZ 
                      << ") at local (" << localX << ", " << worldY << ", " << localZ << ")" << std::endl;
        }
    } else if (g_showDebug) {
        std::cout << "Chunk (" << chunkX << ", " << chunkZ << ") not found for setBlock" << std::endl;
    }
}

Block World::getBlock(int worldX, int worldY, int worldZ) const {
    int chunkX = worldX / Chunk::SIZE;
    int chunkZ = worldZ / Chunk::SIZE;
    int localX = worldX % Chunk::SIZE;
    int localZ = worldZ % Chunk::SIZE;
    if (localX < 0) localX += Chunk::SIZE;
    if (localZ < 0) localZ += Chunk::SIZE;

    std::pair<int, int> chunkKey(chunkX, chunkZ);
    auto it = chunks.find(chunkKey);
    if (it != chunks.end()) {
        Block block = it->second.getBlock(localX, worldY, localZ);
        if (g_showDebug && block.type != BlockType::AIR) {
            std::cout << "Get block at (" << worldX << ", " << worldY << ", " << worldZ 
                      << ") in chunk (" << chunkX << ", " << chunkZ << ") local (" << localX << ", " << worldY << ", " << localZ 
                      << ") returned type " << static_cast<int>(block.type) << std::endl;
        }
        return block;
    }
    return Block(BlockType::AIR);
}