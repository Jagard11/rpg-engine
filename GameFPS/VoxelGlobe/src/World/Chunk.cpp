// ./src/World/Chunk.cpp
#include "World/Chunk.hpp"
#include "World/World.hpp"
#include <vector>
#include <iostream>
#include "Core/Debug.hpp"

Chunk::Chunk(int x, int z) : chunkX(x), chunkZ(z), blocks(SIZE * SIZE * SIZE), world(nullptr) {
    generateTerrain();
}

void Chunk::setWorld(const World* w) {
    world = w;
}

Block Chunk::getBlock(int x, int y, int z) const {
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) {
        if (world) {
            int worldX = chunkX * SIZE + x;
            int worldY = 1500 + y; // FLOOR_HEIGHT = 1500
            int worldZ = chunkZ * SIZE + z;
            return world->getBlock(worldX, worldY, worldZ);
        }
        return Block(BlockType::AIR);
    }
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
                    if (g_showDebug) {
                        std::cout << "Regenerating mesh for block at (" << x << ", " << y << ", " << z 
                                  << ") in chunk (" << chunkX << ", " << chunkZ << ")" << std::endl;
                    }

                    // Top face (+Y), clockwise from above
                    if (getBlock(x, y + 1, z).type == BlockType::AIR) {
                        if (g_showDebug) std::cout << "Exposing top face at (" << x << ", " << y + 1 << ", " << z << ")" << std::endl;
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x),     static_cast<float>(y + 1), static_cast<float>(z),     0.0f, 0.0f, // v0
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), 1.0f, 1.0f, // v1
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z),     1.0f, 0.0f, // v2
                            static_cast<float>(x),     static_cast<float>(y + 1), static_cast<float>(z),     0.0f, 0.0f, // v0
                            static_cast<float>(x),     static_cast<float>(y + 1), static_cast<float>(z + 1), 0.0f, 1.0f, // v3
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), 1.0f, 1.0f  // v1
                        });
                    }
                    // Bottom face (-Y), clockwise from below
                    if (getBlock(x, y - 1, z).type == BlockType::AIR) {
                        if (g_showDebug) std::cout << "Exposing bottom face at (" << x << ", " << y - 1 << ", " << z << ")" << std::endl;
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x),     static_cast<float>(y), static_cast<float>(z),     0.0f, 0.0f,     // v0
                            static_cast<float>(x + 1), static_cast<float>(y), static_cast<float>(z + 1), 1.0f, 1.0f,     // v1
                            static_cast<float>(x + 1), static_cast<float>(y), static_cast<float>(z),     1.0f, 0.0f,     // v2
                            static_cast<float>(x),     static_cast<float>(y), static_cast<float>(z),     0.0f, 0.0f,     // v0
                            static_cast<float>(x),     static_cast<float>(y), static_cast<float>(z + 1), 0.0f, 1.0f,     // v3
                            static_cast<float>(x + 1), static_cast<float>(y), static_cast<float>(z + 1), 1.0f, 1.0f      // v1
                        });
                    }
                    // Left face (-X), clockwise from left
                    if (getBlock(x - 1, y, z).type == BlockType::AIR) {
                        if (g_showDebug) std::cout << "Exposing left face at (" << x - 1 << ", " << y << ", " << z << ")" << std::endl;
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x), static_cast<float>(y),     static_cast<float>(z),     0.0f, 0.0f,     // v0
                            static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(z + 1), 1.0f, 1.0f,     // v1
                            static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(z),     0.0f, 1.0f,     // v2
                            static_cast<float>(x), static_cast<float>(y),     static_cast<float>(z),     0.0f, 0.0f,     // v0
                            static_cast<float>(x), static_cast<float>(y),     static_cast<float>(z + 1), 1.0f, 0.0f,     // v3
                            static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(z + 1), 1.0f, 1.0f      // v1
                        });
                    }
                    // Right face (+X), clockwise from right
                    if (getBlock(x + 1, y, z).type == BlockType::AIR) {
                        if (g_showDebug) std::cout << "Exposing right face at (" << x + 1 << ", " << y << ", " << z << ")" << std::endl;
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x + 1), static_cast<float>(y),     static_cast<float>(z),     0.0f, 0.0f,     // v0
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), 1.0f, 1.0f,     // v1
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z),     0.0f, 1.0f,     // v2
                            static_cast<float>(x + 1), static_cast<float>(y),     static_cast<float>(z),     0.0f, 0.0f,     // v0
                            static_cast<float>(x + 1), static_cast<float>(y),     static_cast<float>(z + 1), 1.0f, 0.0f,     // v3
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), 1.0f, 1.0f      // v1
                        });
                    }
                    // Front face (-Z), clockwise from front
                    if (getBlock(x, y, z - 1).type == BlockType::AIR) {
                        if (g_showDebug) std::cout << "Exposing front face at (" << x << ", " << y << ", " << z - 1 << ")" << std::endl;
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x),     static_cast<float>(y),     static_cast<float>(z), 0.0f, 0.0f,     // v0
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z), 1.0f, 1.0f,     // v1
                            static_cast<float>(x + 1), static_cast<float>(y),     static_cast<float>(z), 1.0f, 0.0f,     // v2
                            static_cast<float>(x),     static_cast<float>(y),     static_cast<float>(z), 0.0f, 0.0f,     // v0
                            static_cast<float>(x),     static_cast<float>(y + 1), static_cast<float>(z), 0.0f, 1.0f,     // v3
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z), 1.0f, 1.0f      // v1
                        });
                    }
                    // Back face (+Z), clockwise from back
                    if (getBlock(x, y, z + 1).type == BlockType::AIR) {
                        if (g_showDebug) std::cout << "Exposing back face at (" << x << ", " << y << ", " << z + 1 << ")" << std::endl;
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x),     static_cast<float>(y),     static_cast<float>(z + 1), 0.0f, 0.0f,     // v0
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), 1.0f, 1.0f,     // v1
                            static_cast<float>(x + 1), static_cast<float>(y),     static_cast<float>(z + 1), 1.0f, 0.0f,     // v2
                            static_cast<float>(x),     static_cast<float>(y),     static_cast<float>(z + 1), 0.0f, 0.0f,     // v0
                            static_cast<float>(x),     static_cast<float>(y + 1), static_cast<float>(z + 1), 0.0f, 1.0f,     // v3
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), 1.0f, 1.0f      // v1
                        });
                    }
                }
            }
        }
    }
    if (g_showDebug && !mesh.empty()) {
        std::cout << "Chunk (" << chunkX << ", " << chunkZ << ") mesh size: " << mesh.size() / 5 << " vertices" << std::endl;
    }
}

const std::vector<float>& Chunk::getMesh() const {
    return mesh;
}