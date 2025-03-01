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
    for (int x = 0; x < SIZE; x++) {
        for (int z = 0; z < SIZE; z++) {
            if (getBlock(x, 8, z).type == BlockType::GRASS) {
                float y = 0.0f; // Render at chunk base; surface height handled by cubeToSphere
                mesh.insert(mesh.end(), {
                    static_cast<float>(x) * voxelSize,     y * voxelSize, static_cast<float>(z) * voxelSize,     0.0f, 0.0f,
                    static_cast<float>(x + 1) * voxelSize, y * voxelSize, static_cast<float>(z) * voxelSize,     1.0f, 0.0f,
                    static_cast<float>(x + 1) * voxelSize, y * voxelSize, static_cast<float>(z + 1) * voxelSize, 1.0f, 1.0f,
                    static_cast<float>(x) * voxelSize,     y * voxelSize, static_cast<float>(z) * voxelSize,     0.0f, 0.0f,
                    static_cast<float>(x + 1) * voxelSize, y * voxelSize, static_cast<float>(z + 1) * voxelSize, 1.0f, 1.0f,
                    static_cast<float>(x) * voxelSize,     y * voxelSize, static_cast<float>(z + 1) * voxelSize, 0.0f, 1.0f
                });
            }
        }
    }
}