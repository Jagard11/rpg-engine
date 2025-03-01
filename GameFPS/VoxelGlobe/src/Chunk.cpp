// ./GameFPS/VoxelGlobe/src/Chunk.cpp
#include "Chunk.hpp"

Chunk::Chunk(int x, int z) : chunkX(x), chunkZ(z) {
    blocks.resize(SIZE * SIZE * SIZE);
    generateTerrain();
}

Block Chunk::getBlock(int x, int y, int z) const {
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) return Block();
    return blocks[x + z * SIZE + y * SIZE * SIZE];
}

void Chunk::setBlock(int x, int y, int z, BlockType type) {
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) return;
    blocks[x + z * SIZE + y * SIZE * SIZE] = Block(type);
    regenerateMesh();
}

void Chunk::generateTerrain() {
    for (int x = 0; x < SIZE; x++) {
        for (int z = 0; z < SIZE; z++) {
            for (int y = 0; y < SIZE; y++) {
                if (y < 8) blocks[x + z * SIZE + y * SIZE * SIZE] = Block(BlockType::DIRT);
                else if (y == 8) blocks[x + z * SIZE + y * SIZE * SIZE] = Block(BlockType::GRASS);
                else blocks[x + z * SIZE + y * SIZE * SIZE] = Block(BlockType::AIR);
            }
        }
    }
    regenerateMesh();
}

void Chunk::regenerateMesh() {
    mesh.clear();
    for (int x = 0; x < SIZE; x++) {
        for (int z = 0; z < SIZE; z++) {
            for (int y = 0; y < SIZE; y++) {
                if (getBlock(x, y, z).type != BlockType::AIR) {
                    if (y + 1 >= SIZE || getBlock(x, y + 1, z).type == BlockType::AIR) {
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x),     static_cast<float>(y + 1), static_cast<float>(z),     0.0f, 0.0f,
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z),     1.0f, 0.0f,
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), 1.0f, 1.0f,
                            static_cast<float>(x),     static_cast<float>(y + 1), static_cast<float>(z),     0.0f, 0.0f,
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), 1.0f, 1.0f,
                            static_cast<float>(x),     static_cast<float>(y + 1), static_cast<float>(z + 1), 0.0f, 1.0f
                        });
                    }
                }
            }
        }
    }
}

const std::vector<float>& Chunk::getMesh() const {
    return mesh;
}