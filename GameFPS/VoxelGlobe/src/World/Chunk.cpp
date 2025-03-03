// ./src/World/Chunk.cpp
#include "World/Chunk.hpp"
#include "World/World.hpp"
#include <vector>
#include <iostream>
#include "Debug/DebugManager.hpp"

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
                    // Top face (+Y), clockwise from above - White (debug) or textured
                    if (getBlock(x, y + 1, z).type == BlockType::AIR) {
                        float u = DebugManager::getInstance().useFaceColors() ? 0.0f : 0.0f; // Face ID 0 for debug
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x),     static_cast<float>(y + 1), static_cast<float>(z),     u, 0.0f, // Bottom-left
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z),     DebugManager::getInstance().useFaceColors() ? u : 1.0f, 0.0f, // Bottom-right
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f, // Top-right
                            static_cast<float>(x),     static_cast<float>(y + 1), static_cast<float>(z),     u, 0.0f, // Bottom-left
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f, // Top-right
                            static_cast<float>(x),     static_cast<float>(y + 1), static_cast<float>(z + 1), DebugManager::getInstance().useFaceColors() ? u : 0.0f, 1.0f  // Top-left
                        });
                    }
                    // Bottom face (-Y), clockwise from below - Black (debug) or textured
                    if (getBlock(x, y - 1, z).type == BlockType::AIR) {
                        float u = DebugManager::getInstance().useFaceColors() ? 1.0f : 0.0f; // Face ID 1 for debug
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x),     static_cast<float>(y), static_cast<float>(z),     u, 0.0f,     // Bottom-left
                            static_cast<float>(x + 1), static_cast<float>(y), static_cast<float>(z + 1), DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f,     // Top-right
                            static_cast<float>(x + 1), static_cast<float>(y), static_cast<float>(z),     DebugManager::getInstance().useFaceColors() ? u : 1.0f, 0.0f,     // Bottom-right
                            static_cast<float>(x),     static_cast<float>(y), static_cast<float>(z),     u, 0.0f,     // Bottom-left
                            static_cast<float>(x),     static_cast<float>(y), static_cast<float>(z + 1), DebugManager::getInstance().useFaceColors() ? u : 0.0f, 1.0f,     // Top-left
                            static_cast<float>(x + 1), static_cast<float>(y), static_cast<float>(z + 1), DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f      // Top-right
                        });
                    }
                    // Left face (-X), clockwise from left - Red (debug) or textured
                    if (getBlock(x - 1, y, z).type == BlockType::AIR) {
                        float u = DebugManager::getInstance().useFaceColors() ? 2.0f : 0.0f; // Face ID 2 for debug
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x), static_cast<float>(y),     static_cast<float>(z),     u, 0.0f,     // Bottom-front
                            static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(z),     u, 1.0f,     // Top-front
                            static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(z + 1), DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f,     // Top-back
                            static_cast<float>(x), static_cast<float>(y),     static_cast<float>(z),     u, 0.0f,     // Bottom-front
                            static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(z + 1), DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f,     // Top-back
                            static_cast<float>(x), static_cast<float>(y),     static_cast<float>(z + 1), DebugManager::getInstance().useFaceColors() ? u : 1.0f, 0.0f      // Bottom-back
                        });
                    }
                    // Right face (+X), clockwise from right - Green (debug) or textured
                    if (getBlock(x + 1, y, z).type == BlockType::AIR) {
                        float u = DebugManager::getInstance().useFaceColors() ? 3.0f : 0.0f; // Face ID 3 for debug
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x + 1), static_cast<float>(y),     static_cast<float>(z),     u, 0.0f,     // Bottom-front
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f,     // Top-back
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z),     DebugManager::getInstance().useFaceColors() ? u : 0.0f, 1.0f,     // Top-front
                            static_cast<float>(x + 1), static_cast<float>(y),     static_cast<float>(z),     u, 0.0f,     // Bottom-front
                            static_cast<float>(x + 1), static_cast<float>(y),     static_cast<float>(z + 1), DebugManager::getInstance().useFaceColors() ? u : 1.0f, 0.0f,     // Bottom-back
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f      // Top-back
                        });
                    }
                    // Front face (-Z), clockwise from front - Purple (debug) or textured
                    if (getBlock(x, y, z - 1).type == BlockType::AIR) {
                        float u = DebugManager::getInstance().useFaceColors() ? 4.0f : 0.0f; // Face ID 4 for debug
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x),     static_cast<float>(y),     static_cast<float>(z), u, 0.0f,     // Bottom-left
                            static_cast<float>(x + 1), static_cast<float>(y),     static_cast<float>(z), DebugManager::getInstance().useFaceColors() ? u : 1.0f, 0.0f,     // Bottom-right
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z), DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f,     // Top-right
                            static_cast<float>(x),     static_cast<float>(y),     static_cast<float>(z), u, 0.0f,     // Bottom-left
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z), DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f,     // Top-right
                            static_cast<float>(x),     static_cast<float>(y + 1), static_cast<float>(z), DebugManager::getInstance().useFaceColors() ? u : 0.0f, 1.0f      // Top-left
                        });
                    }
                    // Back face (+Z), clockwise from back - Yellow (debug) or textured
                    if (getBlock(x, y, z + 1).type == BlockType::AIR) {
                        float u = DebugManager::getInstance().useFaceColors() ? 5.0f : 0.0f; // Face ID 5 for debug
                        mesh.insert(mesh.end(), {
                            static_cast<float>(x),     static_cast<float>(y),     static_cast<float>(z + 1), u, 0.0f,     // Bottom-left
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f,     // Top-right
                            static_cast<float>(x + 1), static_cast<float>(y),     static_cast<float>(z + 1), DebugManager::getInstance().useFaceColors() ? u : 1.0f, 0.0f,     // Bottom-right
                            static_cast<float>(x),     static_cast<float>(y),     static_cast<float>(z + 1), u, 0.0f,     // Bottom-left
                            static_cast<float>(x),     static_cast<float>(y + 1), static_cast<float>(z + 1), DebugManager::getInstance().useFaceColors() ? u : 0.0f, 1.0f,     // Top-left
                            static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1), DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f      // Top-right
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