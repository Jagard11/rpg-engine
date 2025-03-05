// ./src/World/Chunk.cpp
#include "World/Chunk.hpp"
#include "World/World.hpp"
#include <vector>
#include <iostream>
#include "Debug/DebugManager.hpp"
#include <glm/geometric.hpp>
#include <cmath> // for isnan, isinf
#include "Utils/SphereUtils.hpp" // Added
#include "Graphics/MeshGenerator.hpp" // Added

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
        
        // Add validation for extreme world coordinates
        if (abs(worldX) > 100000 || abs(worldY) > 100000 || abs(worldZ) > 100000) {
            // Coordinates too extreme, assume air
            return Block(BlockType::AIR);
        }
        
        try {
            return world->getBlock(worldX, worldY, worldZ);
        } catch (...) {
            // If anything goes wrong, return AIR
            std::cerr << "Exception caught in getBlock for world coords (" 
                      << worldX << ", " << worldY << ", " << worldZ << ")" << std::endl;
            return Block(BlockType::AIR);
        }
    }
    
    // Return blocks from the loaded chunk
    if (mergeFactor == 1) {
        int index = x + y * SIZE + z * SIZE * SIZE;
        if (index < 0 || index >= static_cast<int>(blocks.size())) {
            // This should never happen with the bounds check above, but just to be safe
            std::cerr << "Error: Block index out of bounds in getBlock for chunk (" 
                      << chunkX << ", " << chunkY << ", " << chunkZ 
                      << ") at local pos (" << x << ", " << y << ", " << z << ")" << std::endl;
            return Block(BlockType::AIR);
        }
        return blocks[index];
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
    
    // Get the exact surface radius as used in collision - precise consistency is critical
    float surfaceR = SphereUtils::getSurfaceRadius(world->getRadius());
    
    // Calculate world coordinates of chunk origin
    int worldOriginX = chunkX * SIZE;
    int worldOriginY = chunkY * SIZE;
    int worldOriginZ = chunkZ * SIZE;
    
    // Safety check for extreme coordinates
    if (abs(worldOriginX) > 100000 || abs(worldOriginY) > 100000 || abs(worldOriginZ) > 100000) {
        std::cerr << "Warning: Extreme chunk coordinates detected in generateTerrain: (" 
                 << chunkX << ", " << chunkY << ", " << chunkZ 
                 << "), skipping terrain generation." << std::endl;
        return;
    }
    
    // Track if any blocks are generated
    bool anyBlocksGenerated = false;
    
    // Generate terrain based on distance from center
    for (int x = 0; x < SIZE; x++) {
        for (int y = 0; y < SIZE; y++) {
            for (int z = 0; z < SIZE; z++) {
                // Calculate world position
                int worldX = worldOriginX + x;
                int worldY = worldOriginY + y;
                int worldZ = worldOriginZ + z;
                
                // Create a block center position (x+0.5, y+0.5, z+0.5) - same as in collision
                glm::vec3 blockCenter = glm::vec3(worldX + 0.5f, worldY + 0.5f, worldZ + 0.5f);
                
                // Calculate distance from center using same precision as collision
                double bx = static_cast<double>(blockCenter.x);
                double by = static_cast<double>(blockCenter.y);
                double bz = static_cast<double>(blockCenter.z);
                double dist = sqrt(bx*bx + by*by + bz*bz);
                
                // Determine block type based on distance from center, using SphereUtils
                BlockType blockType = static_cast<BlockType>(
                    SphereUtils::getBlockTypeForElevation(dist, surfaceR)
                );
                
                if (blockType != BlockType::AIR) {
                    anyBlocksGenerated = true;
                }
                
                // Make sure index is in bounds
                int index = x + y * SIZE + z * SIZE * SIZE;
                if (index >= 0 && index < static_cast<int>(blocks.size())) {
                    blocks[index] = Block(blockType);
                } else {
                    std::cerr << "Error: Block index out of bounds in generateTerrain: " << index 
                              << " (max: " << blocks.size() << ")" << std::endl;
                }
            }
        }
    }
    
    // Mark the mesh as needing regeneration
    meshDirty = true;
    buffersDirty = true;

    if (DebugManager::getInstance().logChunkUpdates()) {
        std::cout << "Generated terrain for chunk (" << chunkX << ", " << chunkY << ", " << chunkZ 
                  << "), contains blocks: " << (anyBlocksGenerated ? "YES" : "NO") 
                  << ", distance from center: " << sqrt(worldOriginX*worldOriginX + 
                                                       worldOriginY*worldOriginY + 
                                                       worldOriginZ*worldOriginZ)
                  << ", surface radius: " << surfaceR << std::endl;
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
    
    // Safety check for oversized mesh
    const size_t MAX_MESH_SIZE = 10000000; // 10 million elements
    if (mesh.size() > MAX_MESH_SIZE) {
        std::cerr << "Error: Mesh size too large in updateBuffers: " << mesh.size() 
                  << " elements, truncating to " << MAX_MESH_SIZE << std::endl;
        mesh.resize(MAX_MESH_SIZE);
    }
    
    try {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, mesh.size() * sizeof(float), mesh.data(), GL_STATIC_DRAW);

        indices.clear();
        // Only generate indices for complete quads (4 vertices each)
        size_t numQuads = mesh.size() / 20; // 5 floats per vertex, 4 vertices per quad
        for (size_t i = 0; i < numQuads; i++) {
            size_t baseIndex = i * 4;
            indices.push_back(baseIndex);
            indices.push_back(baseIndex + 1);
            indices.push_back(baseIndex + 2);
            indices.push_back(baseIndex);
            indices.push_back(baseIndex + 2);
            indices.push_back(baseIndex + 3);
        }
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        buffersDirty = false;
    } catch (const std::exception& e) {
        std::cerr << "Exception in updateBuffers: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in updateBuffers" << std::endl;
    }
    
    if (DebugManager::getInstance().logChunkUpdates()) {
        std::cout << "Updated buffers for chunk (" << chunkX << ", " << chunkY << ", " << chunkZ 
                  << ") with " << mesh.size() / 5 << " vertices and " << indices.size() << " indices" << std::endl;
    }
}

void Chunk::regenerateMesh(int lodLevel) {
    try {
        if (DebugManager::getInstance().logChunkUpdates()) {
            std::cout << "Starting mesh regeneration for chunk (" << chunkX << ", " << chunkY << ", " << chunkZ << ")" << std::endl;
        }
        
        if (!world) {
            std::cerr << "Error: World pointer is null in regenerateMesh" << std::endl;
            return;
        }
        
        // Always regenerate the mesh when requested
        mesh.clear();
        indices.clear();
        
        // Use MeshGenerator to generate the chunk mesh
        if (mergeFactor == 1) {
            MeshGenerator::MeshData meshData = MeshGenerator::generateChunkMesh(
                blocks, SIZE, chunkX, chunkY, chunkZ, world->getRadius()
            );
            
            // Copy mesh data to chunk
            mesh = std::move(meshData.vertices);
            indices = std::move(meshData.indices);
        } else {
            // For merged chunks, create a simple quad using MeshGenerator
            MeshGenerator::MeshData meshData = MeshGenerator::generateLODChunkMesh(SIZE, mergeFactor);
            mesh = std::move(meshData.vertices);
            indices = std::move(meshData.indices);
        }
        
        currentLodLevel = lodLevel;
        meshDirty = false;
        buffersDirty = true;
        
        if (DebugManager::getInstance().logChunkUpdates()) {
            std::cout << "Completed mesh regeneration for chunk (" << chunkX << ", " << chunkY << ", " << chunkZ 
                    << ") with " << mesh.size() / 5 << " vertices" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in regenerateMesh: " << e.what() << std::endl;
        // Create a minimal valid mesh to avoid rendering crashes
        MeshGenerator::MeshData fallbackMesh = MeshGenerator::generateFallbackMesh();
        mesh = std::move(fallbackMesh.vertices);
        indices = std::move(fallbackMesh.indices);
        meshDirty = false;
        buffersDirty = true;
    } catch (...) {
        std::cerr << "Unknown exception in regenerateMesh" << std::endl;
        // Create a minimal valid mesh to avoid rendering crashes
        MeshGenerator::MeshData fallbackMesh = MeshGenerator::generateFallbackMesh();
        mesh = std::move(fallbackMesh.vertices);
        indices = std::move(fallbackMesh.indices);
        meshDirty = false;
        buffersDirty = true;
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