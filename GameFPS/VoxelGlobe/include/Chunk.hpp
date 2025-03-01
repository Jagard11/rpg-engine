#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <vector>
#include "Block.hpp"

class Chunk {
public:
    static const int SIZE = 16; // Assuming 16; adjust if different
    Chunk(int x, int z);
    Block getBlock(int x, int y, int z) const;
    void generateTerrain();
    const std::vector<float>& getMesh() const;

private:
    int chunkX, chunkZ;
    std::vector<Block> blocks;
    std::vector<float> mesh; // Vertex data (x, y, z, u, v)
};

#endif