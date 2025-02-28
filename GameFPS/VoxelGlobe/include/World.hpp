#ifndef WORLD_HPP
#define WORLD_HPP

#include "Chunk.hpp"
#include <glm/glm.hpp>  // Explicit GLM include
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
    World() {}
    void update(const glm::vec3& playerPos);
    const std::unordered_map<std::pair<int, int>, Chunk, pair_hash>& getChunks() const { return chunks; }

private:
    std::unordered_map<std::pair<int, int>, Chunk, pair_hash> chunks;
};

#endif