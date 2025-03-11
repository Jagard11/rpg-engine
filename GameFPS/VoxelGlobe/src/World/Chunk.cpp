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
      buffersDirty(true), meshDirty(true), currentLodLevel(-1), buffersInitialized(false),
      relativeOffset(0.0f) {
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
    
    // Safety check for block array
    if (blocks.empty() || blocks.size() != SIZE * SIZE * SIZE) {
        std::cerr << "Error: Invalid blocks array in generateTerrain for chunk (" 
                  << chunkX << ", " << chunkY << ", " << chunkZ 
                  << "), size: " << blocks.size() << std::endl;
        
        // Resize the blocks array if needed
        try {
            blocks.resize(SIZE * SIZE * SIZE, Block(BlockType::AIR));
        } catch (...) {
            std::cerr << "Failed to resize blocks array" << std::endl;
            return;
        }
    }
    
    // Get the exact surface radius as used in collision
    double surfaceR = SphereUtils::getSurfaceRadiusMeters();
    
    // Calculate world coordinates of chunk origin
    int worldOriginX = chunkX * SIZE;
    int worldOriginY = chunkY * SIZE;
    int worldOriginZ = chunkZ * SIZE;
    
    // Track if any blocks are generated
    bool anyBlocksGenerated = false;
    
    // Generate terrain based on distance from center
    try {
        for (int x = 0; x < SIZE; x++) {
            for (int y = 0; y < SIZE; y++) {
                for (int z = 0; z < SIZE; z++) {
                    // Calculate world position
                    int worldX = worldOriginX + x;
                    int worldY = worldOriginY + y;
                    int worldZ = worldOriginZ + z;
                    
                    // Create a block center position (x+0.5, y+0.5, z+0.5) - same as in collision
                    glm::dvec3 blockCenter(worldX + 0.5, worldY + 0.5, worldZ + 0.5);
                    
                    // Calculate distance from center using double precision
                    double dist = glm::length(blockCenter);
                    
                    // Default to standard elevation check if block is too far from center
                    BlockType blockType;
                    if (dist > 1.5 * surfaceR) {
                        // For extreme distances, just use AIR to avoid calculation issues
                        blockType = BlockType::AIR;
                    } else {
                        try {
                            // Determine block type with height variation
                            blockType = static_cast<BlockType>(
                                SphereUtils::getBlockTypeForElevation(dist, blockCenter)
                            );
                        } catch (...) {
                            // Fallback to standard elevation check on error
                            blockType = static_cast<BlockType>(
                                SphereUtils::getBlockTypeForElevation(dist)
                            );
                        }
                    }
                    
                    if (blockType != BlockType::AIR) {
                        anyBlocksGenerated = true;
                    }
                    
                    // Make sure index is in bounds
                    int index = x + y * SIZE + z * SIZE * SIZE;
                    if (index >= 0 && index < static_cast<int>(blocks.size())) {
                        blocks[index] = Block(blockType);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in terrain generation: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in terrain generation" << std::endl;
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
        try {
            // Create buffer objects only if they haven't been created yet
            glGenVertexArrays(1, &vao);
            if (glGetError() != GL_NO_ERROR) {
                LOG_ERROR(LogCategory::RENDERING, "Error generating VAO for chunk");
                return;
            }
            
            glGenBuffers(1, &vbo);
            if (glGetError() != GL_NO_ERROR) {
                LOG_ERROR(LogCategory::RENDERING, "Error generating VBO for chunk");
                glDeleteVertexArrays(1, &vao);
                return;
            }
            
            glGenBuffers(1, &ebo);
            if (glGetError() != GL_NO_ERROR) {
                LOG_ERROR(LogCategory::RENDERING, "Error generating EBO for chunk");
                glDeleteVertexArrays(1, &vao);
                glDeleteBuffers(1, &vbo);
                return;
            }
            
            buffersInitialized = true;
            
            if (DebugManager::getInstance().logChunkUpdates()) {
                std::cout << "Initialized buffers for chunk (" << chunkX << ", " << chunkY << ", " << chunkZ << ")" << std::endl;
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR(LogCategory::RENDERING, "Exception in initializeBuffers: " + std::string(e.what()));
            buffersInitialized = false;
        }
        catch (...) {
            LOG_ERROR(LogCategory::RENDERING, "Unknown exception in initializeBuffers");
            buffersInitialized = false;
        }
    }
    
    // Update the buffers with current data
    try {
        updateBuffers();
    }
    catch (const std::exception& e) {
        LOG_ERROR(LogCategory::RENDERING, "Exception in updateBuffers: " + std::string(e.what()));
    }
    catch (...) {
        LOG_ERROR(LogCategory::RENDERING, "Unknown exception in updateBuffers");
    }
}

void Chunk::updateBuffers() {
    // Skip if buffers don't need updating or mesh is empty
    if (!buffersDirty || mesh.empty() || !buffersInitialized) {
        return;
    }
    
    try {
        // Safety check for oversized mesh
        const size_t MAX_MESH_SIZE = 10000000; // 10 million elements
        if (mesh.size() > MAX_MESH_SIZE) {
            LOG_WARNING(LogCategory::RENDERING, "Mesh size too large: " + 
                        std::to_string(mesh.size()) + " elements, truncating to " + 
                        std::to_string(MAX_MESH_SIZE));
            mesh.resize(MAX_MESH_SIZE);
        }
        
        // Check OpenGL state before operations
        GLint currentVAO = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
        
        // Bind and update the VAO/VBO/EBO
        glBindVertexArray(vao);
        if (glGetError() != GL_NO_ERROR) {
            LOG_ERROR(LogCategory::RENDERING, "Error binding VAO in updateBuffers");
            return;
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        if (glGetError() != GL_NO_ERROR) {
            LOG_ERROR(LogCategory::RENDERING, "Error binding VBO in updateBuffers");
            return;
        }
        
        glBufferData(GL_ARRAY_BUFFER, mesh.size() * sizeof(float), mesh.data(), GL_STATIC_DRAW);
        if (glGetError() != GL_NO_ERROR) {
            LOG_ERROR(LogCategory::RENDERING, "Error uploading vertex data in updateBuffers");
            return;
        }

        // Only update indices if they're non-empty
        if (!indices.empty()) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            if (glGetError() != GL_NO_ERROR) {
                LOG_ERROR(LogCategory::RENDERING, "Error binding EBO in updateBuffers");
                return;
            }
            
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
            if (glGetError() != GL_NO_ERROR) {
                LOG_ERROR(LogCategory::RENDERING, "Error uploading index data in updateBuffers");
                return;
            }
        }

        // Set up vertex attributes
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // Unbind to restore state
        glBindVertexArray(0);
        
        buffersDirty = false;
        
        if (DebugManager::getInstance().logChunkUpdates()) {
            LOG_DEBUG(LogCategory::RENDERING, "Updated buffers for chunk (" + 
                     std::to_string(chunkX) + ", " + std::to_string(chunkY) + ", " + 
                     std::to_string(chunkZ) + ") with " + std::to_string(mesh.size() / 5) + 
                     " vertices and " + std::to_string(indices.size()) + " indices");
        }
    } catch (const std::exception& e) {
        LOG_ERROR(LogCategory::RENDERING, "Exception in updateBuffers: " + std::string(e.what()));
    } catch (...) {
        LOG_ERROR(LogCategory::RENDERING, "Unknown exception in updateBuffers");
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
        
        // Use MeshGenerator to generate the chunk mesh with proper frustum geometry
        if (mergeFactor == 1) {
            MeshGenerator::MeshData meshData = MeshGenerator::generateChunkMesh(
                blocks, SIZE, chunkX, chunkY, chunkZ
            );
            
            // Copy mesh data to chunk
            mesh = std::move(meshData.vertices);
            indices = std::move(meshData.indices);
        } else {
            // For LOD chunks (merged), create a simple quad using MeshGenerator
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

void Chunk::markMeshDirty() {
    meshDirty = true;
    buffersDirty = true;  // Ensure buffers get updated too
}

void Chunk::updateRelativePosition(int originX, int originY, int originZ) {
    // Calculate the chunk's offset from the current origin in world units
    relativeOffset = glm::vec3(
        (chunkX - originX) * SIZE,
        (chunkY - originY) * SIZE,
        (chunkZ - originZ) * SIZE
    );
}

glm::dvec3 Chunk::getWorldCenter() const {
    // Calculate the world-space center of this chunk using chunk coordinates
    return glm::dvec3(
        chunkX * SIZE + SIZE / 2.0,
        chunkY * SIZE + SIZE / 2.0,
        chunkZ * SIZE + SIZE / 2.0
    );
}

const std::vector<float>& Chunk::getMesh() const {
    return mesh;
}