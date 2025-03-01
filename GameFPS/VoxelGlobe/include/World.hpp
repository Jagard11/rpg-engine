// ./GameFPS/VoxelGlobe/include/World.hpp
#ifndef WORLD_HPP
#define WORLD_HPP

#include "Chunk.hpp"
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
    World() : radius(1591.55f) {}
    void update(const glm::vec3& playerPos);
    const std::unordered_map<std::pair<int, int>, Chunk, pair_hash>& getChunks() const { return chunks; }
    glm::vec3 cubeToSphere(int face, int x, int z, float y) const;
    float findSurfaceHeight(int chunkX, int chunkZ) const; // New method

private:
    std::unordered_map<std::pair<int, int>, Chunk, pair_hash> chunks;
    float radius; // 10 km circumference -> ~1591.55 m radius
};

#endif