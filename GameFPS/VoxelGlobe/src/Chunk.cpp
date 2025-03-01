#include "Chunk.hpp"

Chunk::Chunk(int x, int z) : chunkX(x), chunkZ(z) {
    blocks.resize(SIZE * SIZE * SIZE);
    generateTerrain();
}

Block Chunk::getBlock(int x, int y, int z) const {
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) return Block();
    return blocks[x + z * SIZE + y * SIZE * SIZE];
}

void Chunk::generateTerrain() {
    for (int x = 0; x < SIZE; x++) {
        for (int z = 0; z < SIZE; z++) {
            for (int y = 0; y < SIZE; y++) {
                if (y < 8) blocks[x + z * SIZE + y * SIZE * SIZE] = Block(BlockType::DIRT);
                else if (y == 8) blocks[x + z * SIZE + y * SIZE * SIZE] = Block(BlockType::GRASS);
            }
        }
    }
    float voxelSize = 1.0f; // 1 unit per voxel, matching Minecraft
    mesh.clear(); // Ensure we start fresh
    for (int x = 0; x < SIZE; x++) {
        for (int z = 0; z < SIZE; z++) {
            if (getBlock(x, 8, z).type == BlockType::GRASS) {
                float y = 0.0f; // Base height; scaled by World::cubeToSphere
                mesh.insert(mesh.end(), {
                    static_cast<float>(x),     y, static_cast<float>(z),     0.0f, 0.0f,
                    static_cast<float>(x + 1), y, static_cast<float>(z),     1.0f, 0.0f,
                    static_cast<float>(x + 1), y, static_cast<float>(z + 1), 1.0f, 1.0f,
                    static_cast<float>(x),     y, static_cast<float>(z),     0.0f, 0.0f,
                    static_cast<float>(x + 1), y, static_cast<float>(z + 1), 1.0f, 1.0f,
                    static_cast<float>(x),     y, static_cast<float>(z + 1), 0.0f, 1.0f
                });
            }
        }
    }
}

const std::vector<float>& Chunk::getMesh() const {
    return mesh;
}