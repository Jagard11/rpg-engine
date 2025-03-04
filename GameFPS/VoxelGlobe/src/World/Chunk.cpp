// ./src/World/Chunk.cpp
#include "World/Chunk.hpp"
#include "World/World.hpp"
#include <vector>
#include <iostream>
#include "Debug/DebugManager.hpp"
#include <glm/geometric.hpp>

Chunk::Chunk(int x, int y, int z, int mergeFactor) 
    : chunkX(x), chunkY(y), chunkZ(z), mergeFactor(mergeFactor), 
      blocks(mergeFactor == 1 ? SIZE * SIZE * SIZE : 0), world(nullptr), 
      buffersDirty(true), meshDirty(true), currentLodLevel(-1), buffersInitialized(false) {
    // Empty constructor - terrain generation happens in generateTerrain()
}

void Chunk::setWorld(const World* w) {
    world = w;
}

Block Chunk::getBlock(int x, int y, int z) const {
    if (!world) {
        std::cerr << "Error: World pointer is null in getBlock for chunk (" 
                  << chunkX << ", " << chunkY << ", " << chunkZ << ")" << std::endl;
        return Block(BlockType::AIR);
    }
    
    // For out-of-bounds blocks, query from the world
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) {
        int worldX = chunkX * SIZE + x;
        int worldY = chunkY * SIZE + y;
        int worldZ = chunkZ * SIZE + z;
        return world->getBlock(worldX, worldY, worldZ);
    }
    
    // Return blocks from the loaded chunk
    if (mergeFactor == 1) {
        return blocks[x + y * SIZE + z * SIZE * SIZE];
    }
    
    // Default block type (shouldn't typically get here)
    return Block(BlockType::AIR);
}

void Chunk::setBlock(int x, int y, int z, BlockType type) {
    if (mergeFactor > 1) return;
    if (x >= 0 && x < SIZE && y >= 0 && y < SIZE && z >= 0 && z < SIZE) {
        blocks[x + y * SIZE + z * SIZE * SIZE] = Block(type);
        meshDirty = true;
        buffersDirty = true; // Make sure buffers get updated too
        
        if (DebugManager::getInstance().logBlockPlacement()) {
            std::cout << "Set block in chunk (" << chunkX << ", " << chunkY << ", " << chunkZ 
                      << ") at local pos (" << x << ", " << y << ", " << z 
                      << ") to type " << static_cast<int>(type) << std::endl;
        }
    }
}

void Chunk::generateTerrain() {
    if (mergeFactor > 1 || !world) {
        if (!world) {
            std::cerr << "Error: World pointer is null in generateTerrain for chunk (" 
                      << chunkX << ", " << chunkY << ", " << chunkZ << ")" << std::endl;
        }
        return;
    }
    
    // Get the surface radius
    float surfaceR = world->getRadius() + 8.0f; // Surface at radius + 8
    
    // Calculate world coordinates of chunk origin
    int worldOriginX = chunkX * SIZE;
    int worldOriginY = chunkY * SIZE;
    int worldOriginZ = chunkZ * SIZE;
    
    // Generate terrain based on distance from center
    for (int x = 0; x < SIZE; x++) {
        for (int y = 0; y < SIZE; y++) {
            for (int z = 0; z < SIZE; z++) {
                // Calculate world position
                int worldX = worldOriginX + x;
                int worldY = worldOriginY + y;
                int worldZ = worldOriginZ + z;
                
                // Calculate distance from center
                float dist = sqrt(worldX*worldX + worldY*worldY + worldZ*worldZ);
                
                // Determine block type based on distance from center
                if (dist < surfaceR - 5.0f) {
                    // Deep underground - DIRT
                    blocks[x + y * SIZE + z * SIZE * SIZE] = Block(BlockType::DIRT);
                }
                else if (dist < surfaceR) {
                    // Surface layer - GRASS
                    blocks[x + y * SIZE + z * SIZE * SIZE] = Block(BlockType::GRASS);
                }
                else {
                    // Above surface - AIR
                    blocks[x + y * SIZE + z * SIZE * SIZE] = Block(BlockType::AIR);
                }
            }
        }
    }
    
    // Mark the mesh as needing regeneration
    meshDirty = true;
    buffersDirty = true;

    if (DebugManager::getInstance().logChunkUpdates()) {
        std::cout << "Generated terrain for chunk (" << chunkX << ", " << chunkY << ", " << chunkZ << ")" << std::endl;
    }
}

void Chunk::initializeBuffers() {
    if (!buffersInitialized) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        buffersInitialized = true;
        
        if (DebugManager::getInstance().logChunkUpdates()) {
            std::cout << "Initialized buffers for chunk (" << chunkX << ", " << chunkY << ", " << chunkZ << ")" << std::endl;
        }
    }
    updateBuffers();
}

void Chunk::updateBuffers() {
    if (!buffersDirty || mesh.empty()) return;
    
    // Ensure we have a valid VAO before binding
    if (!buffersInitialized) {
        std::cerr << "Error: Attempting to update buffers before initialization for chunk (" 
                  << chunkX << ", " << chunkY << ", " << chunkZ << ")" << std::endl;
        return;
    }
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh.size() * sizeof(float), mesh.data(), GL_STATIC_DRAW);

    indices.clear();
    for (size_t i = 0; i < mesh.size() / 5; i += 4) {
        indices.push_back(i);
        indices.push_back(i + 1);
        indices.push_back(i + 2);
        indices.push_back(i);
        indices.push_back(i + 2);
        indices.push_back(i + 3);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    buffersDirty = false;
    
    if (DebugManager::getInstance().logChunkUpdates()) {
        std::cout << "Updated buffers for chunk (" << chunkX << ", " << chunkY << ", " << chunkZ 
                  << ") with " << mesh.size() / 5 << " vertices and " << indices.size() << " indices" << std::endl;
    }
}

void Chunk::regenerateMesh(int lodLevel) {
    if (!world) {
        std::cerr << "Error: World pointer is null in regenerateMesh for chunk (" 
                  << chunkX << ", " << chunkY << ", " << chunkZ << ")" << std::endl;
        return;
    }
    
    // Always regenerate the mesh when requested, even if not "dirty"
    // Remove the condition: if (!meshDirty && lodLevel == currentLodLevel) return;
    
    mesh.clear();
    
    // Debug output for mesh generation
    if (DebugManager::getInstance().logChunkUpdates()) {
        std::cout << "Regenerating mesh for chunk (" << chunkX << ", " << chunkY << ", " << chunkZ << ")" << std::endl;
    }
    
    if (mergeFactor == 1) {
        // Generate mesh for each block in the chunk
        for (int x = 0; x < SIZE; x++) {
            for (int y = 0; y < SIZE; y++) {
                for (int z = 0; z < SIZE; z++) {
                    Block block = getBlock(x, y, z);
                    if (block.type != BlockType::AIR) {
                        // Check all 6 faces
                        const glm::ivec3 directions[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
                        for (int i = 0; i < 6; i++) {
                            glm::ivec3 neighbor(x + directions[i].x, y + directions[i].y, z + directions[i].z);
                            if (neighbor.x < 0 || neighbor.x >= SIZE || neighbor.y < 0 || neighbor.y >= SIZE || 
                                neighbor.z < 0 || neighbor.z >= SIZE || getBlock(neighbor.x, neighbor.y, neighbor.z).type == BlockType::AIR) {
                                float uBase = DebugManager::getInstance().useFaceColors() ? static_cast<float>(i) : 0.0f;
                                if (i == 0) { // +X face
                                    mesh.insert(mesh.end(), {static_cast<float>(x) + 1.0f, static_cast<float>(y), static_cast<float>(z), uBase, 0.0f,
                                                             static_cast<float>(x) + 1.0f, static_cast<float>(y) + 1.0f, static_cast<float>(z), uBase, 1.0f,
                                                             static_cast<float>(x) + 1.0f, static_cast<float>(y) + 1.0f, static_cast<float>(z) + 1.0f, uBase + 1.0f, 1.0f,
                                                             static_cast<float>(x) + 1.0f, static_cast<float>(y), static_cast<float>(z) + 1.0f, uBase + 1.0f, 0.0f});
                                } else if (i == 1) { // -X face
                                    mesh.insert(mesh.end(), {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), uBase, 0.0f,
                                                             static_cast<float>(x), static_cast<float>(y), static_cast<float>(z) + 1.0f, uBase + 1.0f, 0.0f,
                                                             static_cast<float>(x), static_cast<float>(y) + 1.0f, static_cast<float>(z) + 1.0f, uBase + 1.0f, 1.0f,
                                                             static_cast<float>(x), static_cast<float>(y) + 1.0f, static_cast<float>(z), uBase, 1.0f});
                                } else if (i == 2) { // +Y face
                                    mesh.insert(mesh.end(), {static_cast<float>(x), static_cast<float>(y) + 1.0f, static_cast<float>(z), uBase, 0.0f,
                                                             static_cast<float>(x) + 1.0f, static_cast<float>(y) + 1.0f, static_cast<float>(z), uBase + 1.0f, 0.0f,
                                                             static_cast<float>(x) + 1.0f, static_cast<float>(y) + 1.0f, static_cast<float>(z) + 1.0f, uBase + 1.0f, 1.0f,
                                                             static_cast<float>(x), static_cast<float>(y) + 1.0f, static_cast<float>(z) + 1.0f, uBase, 1.0f});
                                } else if (i == 3) { // -Y face
                                    mesh.insert(mesh.end(), {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), uBase, 0.0f,
                                                             static_cast<float>(x), static_cast<float>(y), static_cast<float>(z) + 1.0f, uBase, 1.0f,
                                                             static_cast<float>(x) + 1.0f, static_cast<float>(y), static_cast<float>(z) + 1.0f, uBase + 1.0f, 1.0f,
                                                             static_cast<float>(x) + 1.0f, static_cast<float>(y), static_cast<float>(z), uBase + 1.0f, 0.0f});
                                } else if (i == 4) { // +Z face
                                    mesh.insert(mesh.end(), {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z) + 1.0f, uBase, 0.0f,
                                                             static_cast<float>(x) + 1.0f, static_cast<float>(y), static_cast<float>(z) + 1.0f, uBase + 1.0f, 0.0f,
                                                             static_cast<float>(x) + 1.0f, static_cast<float>(y) + 1.0f, static_cast<float>(z) + 1.0f, uBase + 1.0f, 1.0f,
                                                             static_cast<float>(x), static_cast<float>(y) + 1.0f, static_cast<float>(z) + 1.0f, uBase, 1.0f});
                                } else if (i == 5) { // -Z face
                                    mesh.insert(mesh.end(), {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), uBase, 0.0f,
                                                             static_cast<float>(x), static_cast<float>(y) + 1.0f, static_cast<float>(z), uBase, 1.0f,
                                                             static_cast<float>(x) + 1.0f, static_cast<float>(y) + 1.0f, static_cast<float>(z), uBase + 1.0f, 1.0f,
                                                             static_cast<float>(x) + 1.0f, static_cast<float>(y), static_cast<float>(z), uBase + 1.0f, 0.0f});
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        // For merged chunks, create a simple quad at the surface level
        float chunkSize = SIZE * mergeFactor;
        
        // Create a simple quad for debugging visualization
        mesh = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            chunkSize, 0.0f, 0.0f, 1.0f, 0.0f,
            chunkSize, chunkSize, chunkSize, 1.0f, 1.0f,
            0.0f, chunkSize, chunkSize, 0.0f, 1.0f
        };
    }

    currentLodLevel = lodLevel;
    meshDirty = false;
    buffersDirty = true;  // Make sure buffers get updated with new mesh

    if (DebugManager::getInstance().logChunkUpdates()) {
        std::cout << "Mesh regenerated for chunk (" << chunkX << ", " << chunkY << ", " << chunkZ 
                << "), Vertices: " << mesh.size() / 5 << ", LOD: " << lodLevel << std::endl;
    }
}

void Chunk::regenerateMesh() {
    regenerateMesh(0);
}

const std::vector<float>& Chunk::getMesh() const {
    return mesh;
}

void Chunk::markMeshDirty() {
    meshDirty = true;
    buffersDirty = true;  // Ensure buffers get updated too
}