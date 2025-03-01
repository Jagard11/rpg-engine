#include "World.hpp"
#include <cmath>
#include <iostream>
#include <ios> // Explicitly include for streamsize
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
            for (int z = pz - renderDist; z <= px + renderDist; z++) {
                chunks.emplace(std::make_pair(x + face * 1000, z), Chunk(x, z));
            }
        }
    }
}

glm::vec3 World::cubeToSphere(int face, int x, int z, float y) const {
    float bx = x * Chunk::SIZE;
    float bz = z * Chunk::SIZE;
    float scale = 0.5f;
    float u = bx * scale;
    float v = bz * scale;
    glm::vec3 pos(u, radius + y, v); // y=8 becomes 1599.55
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
        if (g_showDebug) std::cout << "Checking y = " << y << ": " << static_cast<int>(type) << std::endl;
        if (type != BlockType::AIR) {
            if (g_showDebug) std::cout << "Surface at y = " << y << " in chunk (" << chunkX << ", " << chunkZ << ")" << std::endl;
            return radius + y; // y=8 returns 1599.55
        }
    }
    if (g_showDebug) std::cout << "No surface in chunk (" << chunkX << ", " << chunkZ << ")" << std::endl;
    return radius;
}