// ./VoxelGlobe/include/Chunk.hpp
#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <vector>
#include "World/Block.hpp"

class Chunk {
public:
    static const int SIZE = 16;
    Chunk(int x, int z);
    Block getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockType type);
    void generateTerrain();
    void regenerateMesh();
    const std::vector<float>& getMesh() const;

private:
    int chunkX, chunkZ;
    std::vector<Block> blocks;
    std::vector<float> mesh;
};

#endif