#include "World.hpp"

void World::update(const glm::vec3& playerPos) {
    int px = static_cast<int>(playerPos.x / Chunk::SIZE);
    int pz = static_cast<int>(playerPos.z / Chunk::SIZE);
    int renderDist = 8;
    chunks.clear(); // Reload for simplicity
    for (int x = px - renderDist; x <= px + renderDist; x++) {
        for (int z = pz - renderDist; z <= pz + renderDist; z++) {
            chunks.emplace(std::make_pair(x, z), Chunk(x, z));
        }
    }
}