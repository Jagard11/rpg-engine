// ./GameFPS/VoxelGlobe/src/World.cpp
#include "World.hpp"
#include <cmath>
#include <iostream>
#include "Debug.hpp"

void World::update(const glm::vec3& playerPos) {
    int px = static_cast<int>(playerPos.x / Chunk::SIZE);
    int pz = static_cast<int>(playerPos.z / Chunk::SIZE);
    int renderDist = 8;
    chunks.clear();

    px = glm::clamp(px, -1000, 1000);
    pz = glm::clamp(pz, -1000, 1000);

    for (int face = 0; face < 1; face++) {
        for (int x = px - renderDist; x <= px + renderDist; x++) {
            for (int z = pz - renderDist; z <= pz + renderDist; z++) {
                chunks.emplace(std::make_pair(x + face * 1000, z), Chunk(x, z));
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
    if (g_showDebug) {
        std::cout << "Chunk Pos: " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
    }
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

void World::setBlock(int worldX, int worldY, int worldZ, BlockType type) {
    int chunkX = worldX / Chunk::SIZE;
    int chunkZ = worldZ / Chunk::SIZE;
    int localX = worldX % Chunk::SIZE;
    int localZ = worldZ % Chunk::SIZE;
    if (localX < 0) localX += Chunk::SIZE;
    if (localZ < 0) localZ += Chunk::SIZE;

    auto it = chunks.find(std::make_pair(chunkX, chunkZ));
    if (it != chunks.end()) {
        it->second.setBlock(localX, worldY, localZ, type);
    }
}

Block World::getBlock(int worldX, int worldY, int worldZ) const {
    int chunkX = worldX / Chunk::SIZE;
    int chunkZ = worldZ / Chunk::SIZE;
    int localX = worldX % Chunk::SIZE;
    int localZ = worldZ % Chunk::SIZE;
    if (localX < 0) localX += Chunk::SIZE;
    if (localZ < 0) localZ += Chunk::SIZE;

    auto it = chunks.find(std::make_pair(chunkX, chunkZ));
    if (it != chunks.end()) {
        return it->second.getBlock(localX, worldY, localZ);
    }
    return Block(BlockType::AIR);
}