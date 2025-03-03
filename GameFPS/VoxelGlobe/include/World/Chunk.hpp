// ./include/World/Chunk.hpp
#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <vector>
#include "World/Block.hpp"

class World;

class Chunk {
public:
    static const int SIZE = 16;
    Chunk(int x, int z);
    void setWorld(const World* w);
    Block getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockType type);
    void generateTerrain();
    void regenerateMesh(int lodLevel); // Updated signature
    const std::vector<float>& getMesh() const;

private:
    int chunkX, chunkZ;
    std::vector<Block> blocks;
    std::vector<float> mesh;
    const World* world;
};

#endif