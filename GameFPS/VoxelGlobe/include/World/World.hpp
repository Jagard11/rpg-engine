// ./include/World/World.hpp
#ifndef WORLD_HPP
#define WORLD_HPP

#include "World/Chunk.hpp"
#include <glm/glm.hpp>
#include <unordered_map>
#include <utility>

struct pair_hash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        return std::hash<T1>{}(p.first) ^ std::hash<T2>{}(p.second);
    }
};

class World {
public:
    World();
    void update(const glm::vec3& playerPos);
    std::unordered_map<std::pair<int, int>, Chunk, pair_hash>& getChunks() { return chunks; }
    const std::unordered_map<std::pair<int, int>, Chunk, pair_hash>& getChunks() const { return chunks; }
    glm::vec3 cubeToSphere(int face, int x, int z, float y) const;
    float findSurfaceHeight(float chunkX, float chunkZ) const;
    void setBlock(int worldX, int worldY, int worldZ, BlockType type);
    Block getBlock(int worldX, int worldY, int worldZ) const;
    glm::ivec3 getLocalOrigin() const; // Added
    float getRadius() const; // Added

private:
    std::unordered_map<std::pair<int, int>, Chunk, pair_hash> chunks;
    float radius;
    glm::ivec3 localOrigin; // Added for local coordinate system
};

#endif