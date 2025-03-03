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
            int worldY = y;
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
        regenerateMesh(0); // Default to full detail on block change
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
    regenerateMesh(0);
}

void Chunk::regenerateMesh(int lodLevel) {
    mesh.clear();
    if (!world) return;

    glm::vec3 chunkBase = world->cubeToSphere(0, chunkX, chunkZ, 0.0f);
    float radius = world->getRadius();
    int step = (lodLevel == 0) ? 1 : (lodLevel == 1) ? 2 : 4; // LOD: full, half, quarter resolution

    for (int x = 0; x < SIZE; x += step) {
        for (int y = 0; y < SIZE; y += step) {
            for (int z = 0; z < SIZE; z += step) {
                if (getBlock(x, y, z).type != BlockType::AIR) {
                    glm::vec3 voxelPos(x, y, z);
                    glm::vec3 globalPos = chunkBase + voxelPos;
                    float dist = glm::length(globalPos);
                    float taperFactor = (radius - step) / radius;
                    glm::vec3 dir = glm::normalize(globalPos);

                    glm::vec3 v000(x, y, z);
                    glm::vec3 v001(x, y, z + step);
                    glm::vec3 v010(x, y + step, z);
                    glm::vec3 v011(x, y + step, z + step);
                    glm::vec3 v100(x + step, y, z);
                    glm::vec3 v101(x + step, y, z + step);
                    glm::vec3 v110(x + step, y + step, z);
                    glm::vec3 v111(x + step, y + step, z + step);

                    v000 = v000 + (dir * (dist - radius) * (1.0f - taperFactor));
                    v001 = v001 + (dir * (dist - radius) * (1.0f - taperFactor));
                    v100 = v100 + (dir * (dist - radius) * (1.0f - taperFactor));
                    v101 = v101 + (dir * (dist - radius) * (1.0f - taperFactor));

                    // Top face (+Y)
                    if (getBlock(x, y + step, z).type == BlockType::AIR) {
                        float u = DebugManager::getInstance().useFaceColors() ? 0.0f : 0.0f;
                        mesh.insert(mesh.end(), {
                            v010.x, v010.y, v010.z, u, 0.0f,
                            v110.x, v110.y, v110.z, DebugManager::getInstance().useFaceColors() ? u : 1.0f, 0.0f,
                            v111.x, v111.y, v111.z, DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f,
                            v010.x, v010.y, v010.z, u, 0.0f,
                            v111.x, v111.y, v111.z, DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f,
                            v011.x, v011.y, v011.z, DebugManager::getInstance().useFaceColors() ? u : 0.0f, 1.0f
                        });
                    }
                    // Bottom face (-Y)
                    if (getBlock(x, y - step, z).type == BlockType::AIR) {
                        float u = DebugManager::getInstance().useFaceColors() ? 1.0f : 0.0f;
                        mesh.insert(mesh.end(), {
                            v000.x, v000.y, v000.z, u, 0.0f,
                            v101.x, v101.y, v101.z, DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f,
                            v100.x, v100.y, v100.z, DebugManager::getInstance().useFaceColors() ? u : 1.0f, 0.0f,
                            v000.x, v000.y, v000.z, u, 0.0f,
                            v001.x, v001.y, v001.z, DebugManager::getInstance().useFaceColors() ? u : 0.0f, 1.0f,
                            v101.x, v101.y, v101.z, DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f
                        });
                    }
                    // Left face (-X)
                    if (getBlock(x - step, y, z).type == BlockType::AIR) {
                        float u = DebugManager::getInstance().useFaceColors() ? 2.0f : 0.0f;
                        mesh.insert(mesh.end(), {
                            v000.x, v000.y, v000.z, u, 0.0f,
                            v010.x, v010.y, v010.z, u, 1.0f,
                            v011.x, v011.y, v011.z, DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f,
                            v000.x, v000.y, v000.z, u, 0.0f,
                            v011.x, v011.y, v011.z, DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f,
                            v001.x, v001.y, v001.z, DebugManager::getInstance().useFaceColors() ? u : 1.0f, 0.0f
                        });
                    }
                    // Right face (+X)
                    if (getBlock(x + step, y, z).type == BlockType::AIR) {
                        float u = DebugManager::getInstance().useFaceColors() ? 3.0f : 0.0f;
                        mesh.insert(mesh.end(), {
                            v100.x, v100.y, v100.z, u, 0.0f,
                            v111.x, v111.y, v111.z, DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f,
                            v110.x, v110.y, v110.z, DebugManager::getInstance().useFaceColors() ? u : 0.0f, 1.0f,
                            v100.x, v100.y, v100.z, u, 0.0f,
                            v101.x, v101.y, v101.z, DebugManager::getInstance().useFaceColors() ? u : 1.0f, 0.0f,
                            v111.x, v111.y, v111.z, DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f
                        });
                    }
                    // Front face (-Z)
                    if (getBlock(x, y, z - step).type == BlockType::AIR) {
                        float u = DebugManager::getInstance().useFaceColors() ? 4.0f : 0.0f;
                        mesh.insert(mesh.end(), {
                            v000.x, v000.y, v000.z, u, 0.0f,
                            v100.x, v100.y, v100.z, DebugManager::getInstance().useFaceColors() ? u : 1.0f, 0.0f,
                            v110.x, v110.y, v110.z, DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f,
                            v000.x, v000.y, v000.z, u, 0.0f,
                            v110.x, v110.y, v110.z, DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f,
                            v010.x, v010.y, v010.z, DebugManager::getInstance().useFaceColors() ? u : 0.0f, 1.0f
                        });
                    }
                    // Back face (+Z)
                    if (getBlock(x, y, z + step).type == BlockType::AIR) {
                        float u = DebugManager::getInstance().useFaceColors() ? 5.0f : 0.0f;
                        mesh.insert(mesh.end(), {
                            v001.x, v001.y, v001.z, u, 0.0f,
                            v111.x, v111.y, v111.z, DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f,
                            v101.x, v101.y, v101.z, DebugManager::getInstance().useFaceColors() ? u : 1.0f, 0.0f,
                            v001.x, v001.y, v001.z, u, 0.0f,
                            v011.x, v011.y, v011.z, DebugManager::getInstance().useFaceColors() ? u : 0.0f, 1.0f,
                            v111.x, v111.y, v111.z, DebugManager::getInstance().useFaceColors() ? u : 1.0f, 1.0f
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