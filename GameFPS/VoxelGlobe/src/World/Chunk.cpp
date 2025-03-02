// ./src/World/Chunk.cpp
#include "World/Chunk.hpp"
#include <vector>

Chunk::Chunk(int x, int z) : chunkX(x), chunkZ(z), blocks(SIZE * SIZE * SIZE) {
    generateTerrain();
}

Block Chunk::getBlock(int x, int y, int z) const {
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) return Block(BlockType::AIR);
    return blocks[x + y * SIZE + z * SIZE * SIZE];
}

void Chunk::setBlock(int x, int y, int z, BlockType type) {
    if (x >= 0 && x < SIZE && y >= 0 && y < SIZE && z >= 0 && z < SIZE) {
        blocks[x + y * SIZE + z * SIZE * SIZE] = Block(type);
        regenerateMesh();
    }
}

void Chunk::generateTerrain() {
    for (int x = 0; x < SIZE; x++) {
        for (int z = 0; z < SIZE; z++) {
            for (int y = 0; y < SIZE; y++) {
                if (y < 8) blocks[x + y * SIZE + z * SIZE * SIZE] = Block(BlockType::DIRT);
                else if (y == 8) blocks[x + y * SIZE + z * SIZE * SIZE] = Block(BlockType::GRASS);
                else blocks[x + y * SIZE + z * SIZE * SIZE] = Block(BlockType::AIR);
            }
        }
    }
    regenerateMesh();
}

void Chunk::regenerateMesh() {
    mesh.clear();
    for (int x = 0; x < SIZE; x++) {
        for (int y = 0; y < SIZE; y++) {
            for (int z = 0; z < SIZE; z++) {
                if (getBlock(x, y, z).type != BlockType::AIR) {
                    // Top face (+Y)
                    if (y == SIZE - 1 || getBlock(x, y + 1, z).type == BlockType::AIR) {
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x),     static_cast<float>(y + 1), static_cast<float>(z),     0.0f, 0.0f,
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), 1.0f, 1.0f,
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z),     1.0f, 0.0f,
                            static_cast<float>(x),     static_cast<float>(y + 1), static_cast<float>(z),     0.0f, 0.0f,
                            static_cast<float>(x),     static_cast<float>(y + 1), static_cast<float>(z + 1), 0.0f, 1.0f,
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), 1.0f, 1.0f
                        });
                    }
                    // Bottom face (-Y)
                    if (y == 0 || getBlock(x, y - 1, z).type == BlockType::AIR) {
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x),     static_cast<float>(y), static_cast<float>(z),     0.0f, 0.0f,
                            static_cast<float>(x + 1), static_cast<float>(y), static_cast<float>(z + 1), 1.0f, 1.0f,
                            static_cast<float>(x + 1), static_cast<float>(y), static_cast<float>(z),     1.0f, 0.0f,
                            static_cast<float>(x),     static_cast<float>(y), static_cast<float>(z),     0.0f, 0.0f,
                            static_cast<float>(x),     static_cast<float>(y), static_cast<float>(z + 1), 0.0f, 1.0f,
                            static_cast<float>(x + 1), static_cast<float>(y), static_cast<float>(z + 1), 1.0f, 1.0f
                        });
                    }
                    // Left face (-X)
                    if (x == 0 || getBlock(x - 1, y, z).type == BlockType::AIR) {
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x), static_cast<float>(y),     static_cast<float>(z),     0.0f, 0.0f,
                            static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(z + 1), 1.0f, 1.0f,
                            static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(z),     0.0f, 1.0f,
                            static_cast<float>(x), static_cast<float>(y),     static_cast<float>(z),     0.0f, 0.0f,
                            static_cast<float>(x), static_cast<float>(y),     static_cast<float>(z + 1), 1.0f, 0.0f,
                            static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(z + 1), 1.0f, 1.0f
                        });
                    }
                    // Right face (+X)
                    if (x == SIZE - 1 || getBlock(x + 1, y, z).type == BlockType::AIR) {
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x + 1), static_cast<float>(y),     static_cast<float>(z),     0.0f, 0.0f,
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), 1.0f, 1.0f,
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z),     0.0f, 1.0f,
                            static_cast<float>(x + 1), static_cast<float>(y),     static_cast<float>(z),     0.0f, 0.0f,
                            static_cast<float>(x + 1), static_cast<float>(y),     static_cast<float>(z + 1), 1.0f, 0.0f,
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), 1.0f, 1.0f
                        });
                    }
                    // Front face (-Z)
                    if (z == 0 || getBlock(x, y, z - 1).type == BlockType::AIR) {
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x),     static_cast<float>(y),     static_cast<float>(z), 0.0f, 0.0f,
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z), 1.0f, 1.0f,
                            static_cast<float>(x + 1), static_cast<float>(y),     static_cast<float>(z), 1.0f, 0.0f,
                            static_cast<float>(x),     static_cast<float>(y),     static_cast<float>(z), 0.0f, 0.0f,
                            static_cast<float>(x),     static_cast<float>(y + 1), static_cast<float>(z), 0.0f, 1.0f,
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z), 1.0f, 1.0f
                        });
                    }
                    // Back face (+Z)
                    if (z == SIZE - 1 || getBlock(x, y, z + 1).type == BlockType::AIR) {
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x),     static_cast<float>(y),     static_cast<float>(z + 1), 0.0f, 0.0f,
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), 1.0f, 1.0f,
                            static_cast<float>(x + 1), static_cast<float>(y),     static_cast<float>(z + 1), 1.0f, 0.0f,
                            static_cast<float>(x),     static_cast<float>(y),     static_cast<float>(z + 1), 0.0f, 0.0f,
                            static_cast<float>(x),     static_cast<float>(y + 1), static_cast<float>(z + 1), 0.0f, 1.0f,
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), 1.0f, 1.0f
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