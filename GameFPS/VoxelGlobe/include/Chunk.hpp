#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "Block.hpp"
#include <vector>

class Chunk {
public:
    static const int SIZE = 16;
    Chunk(int x, int z);
    Block getBlock(int x, int y, int z) const;
    const std::vector<float>& getMesh() const { return mesh; }
    void generateTerrain();

private:
    std::vector<Block> blocks;
    std::vector<float> mesh; // pos (3), uv (2)
    int chunkX, chunkZ;
};

#endif