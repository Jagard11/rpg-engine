// ./src/World/Chunk.cpp
#include "World/Chunk.hpp"
#include "World/World.hpp"
#include <vector>
#include <iostream>
#include "Debug/DebugManager.hpp"
#include <glm/geometric.hpp>
#include <cmath> // for isnan, isinf

// Constants for collision and rendering alignment
const float COLLISION_OFFSET = 0.25f; // Same as in Movement.cpp

// Helper function to check if a vector contains NaN or Inf values
bool isValidVector(const glm::vec3& v) {
    return !std::isnan(v.x) && !std::isinf(v.x) &&
           !std::isnan(v.y) && !std::isinf(v.y) &&
           !std::isnan(v.z) && !std::isinf(v.z);
}

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
    float surfaceR = world->getRadius() + 8.0f;
    
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
                
                // Determine block type based on distance from center
                BlockType blockType;
                
                // EXACT same rules as used in collision detection - complete consistency
                if (dist < surfaceR - 5.0) {
                    // Deep underground - DIRT
                    blockType = BlockType::DIRT;
                    anyBlocksGenerated = true;
                }
                else if (dist < surfaceR) {
                    // Surface layer - GRASS (8 units thick, matching collision)
                    blockType = BlockType::GRASS;
                    anyBlocksGenerated = true;
                }
                else {
                    // Above surface - AIR
                    blockType = BlockType::AIR;
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

// Improved sphere projection function - EXACTLY matching collision detection for perfect alignment
glm::vec3 projectToSphere(const glm::vec3& worldPos, float surfaceR, bool isInner, int faceType) {
    // Safety checks for invalid input
    if (glm::length(worldPos) < 0.001f) {
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }
    
    // Get the voxel's integer block position (floor for consistency with collision)
    glm::ivec3 blockPos = glm::ivec3(floor(worldPos.x), floor(worldPos.y), floor(worldPos.z));
    
    // Calculate block center in world space (x+0.5, y+0.5, z+0.5)
    glm::vec3 blockCenter = glm::vec3(blockPos) + glm::vec3(0.5f);
    
    // Get normalized direction from world origin to block center
    glm::vec3 blockDir = glm::normalize(blockCenter);
    
    // Calculate distance from center to block center - with double precision
    double bx = static_cast<double>(blockCenter.x);
    double by = static_cast<double>(blockCenter.y);
    double bz = static_cast<double>(blockCenter.z);
    double blockDistance = sqrt(bx*bx + by*by + bz*bz);
    
    // Calculate height layer using floor to ensure consistent layers
    int heightLayer = static_cast<int>(floor(blockDistance - surfaceR));
    
    // Base radius for this height layer (exact match to collision detection)
    float baseRadius = surfaceR + static_cast<float>(heightLayer);
    
    // Select appropriate radius based on face type
    float radius;
    
    // Face types:
    // 2 = top face (+Y), 3 = bottom face (-Y), others = side faces
    if (faceType == 2) { // Top face (+Y)
        // Exactly 1.0 unit above the base radius - matching collision
        radius = baseRadius + 1.0f;
    } else if (faceType == 3) { // Bottom face (-Y)
        // Exactly at the base radius - matching collision 
        radius = baseRadius;
    } else { // Side faces
        // For side faces, use exact local Y position (0.0 to 1.0)
        float localY = worldPos.y - blockPos.y;
        radius = baseRadius + localY;
    }
    
    // Project the vertex onto the sphere at the calculated radius
    // Using the block's center direction ensures perfect alignment
    return blockDir * radius;
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
        
        // Get surface radius and add the collision offset for perfect visual-collision alignment
        // This ensures that rendered terrain appears exactly where collision happens
        float surfaceR = world->getRadius() + 8.0f;
        float renderSurfaceR = surfaceR + COLLISION_OFFSET; // Add collision offset for visual rendering
        
        // Chunk center distance from world center
        double chunkCenterX = chunkX * SIZE + SIZE/2.0;
        double chunkCenterY = chunkY * SIZE + SIZE/2.0;
        double chunkCenterZ = chunkZ * SIZE + SIZE/2.0;
        double chunkDist = sqrt(chunkCenterX*chunkCenterX + chunkCenterY*chunkCenterY + chunkCenterZ*chunkCenterZ);
        
        if (mergeFactor == 1) {
            // Track blocks that have visible faces
            std::vector<std::tuple<int, int, int, BlockType>> visibleBlocks;
            
            // Find all non-air blocks with at least one exposed face
            for (int x = 0; x < SIZE; x++) {
                for (int y = 0; y < SIZE; y++) {
                    for (int z = 0; z < SIZE; z++) {
                        try {
                            Block block = getBlock(x, y, z);
                            if (block.type != BlockType::AIR) {
                                // Check all 6 neighboring directions
                                const glm::ivec3 directions[6] = {
                                    {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
                                };
                                
                                bool hasVisibleFace = false;
                                for (int i = 0; i < 6; i++) {
                                    int nx = x + directions[i].x;
                                    int ny = y + directions[i].y;
                                    int nz = z + directions[i].z;
                                    
                                    // If neighbor is out of bounds or is air, this face is visible
                                    if (nx < 0 || nx >= SIZE || ny < 0 || ny >= SIZE || nz < 0 || nz >= SIZE) {
                                        hasVisibleFace = true;
                                        break;
                                    }
                                    
                                    // Check if neighbor block is air
                                    try {
                                        Block neighborBlock = getBlock(nx, ny, nz);
                                        if (neighborBlock.type == BlockType::AIR) {
                                            hasVisibleFace = true;
                                            break;
                                        }
                                    } catch (...) {
                                        // Assume visible if exception occurs
                                        hasVisibleFace = true;
                                        break;
                                    }
                                }
                                
                                if (hasVisibleFace) {
                                    visibleBlocks.push_back(std::make_tuple(x, y, z, block.type));
                                }
                            }
                        } catch (...) {
                            continue; // Skip blocks that cause exceptions
                        }
                    }
                }
            }
            
            // Generate mesh for each visible block
            for (const auto& [x, y, z, type] : visibleBlocks) {
                // Calculate world position
                int worldX = chunkX * SIZE + x;
                int worldY = chunkY * SIZE + y;
                int worldZ = chunkZ * SIZE + z;
                
                // Calculate block center with double precision - matching collision
                double wx = static_cast<double>(worldX) + 0.5;
                double wy = static_cast<double>(worldY) + 0.5;
                double wz = static_cast<double>(worldZ) + 0.5;
                double dist = sqrt(wx*wx + wy*wy + wz*wz);
                
                // Determine if block is inside or outside the surface
                bool isInner = (dist < renderSurfaceR); // Use renderSurfaceR instead of surfaceR
                
                // Check surrounding faces for visibility
                const glm::ivec3 directions[6] = {
                    {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
                };
                
                for (int i = 0; i < 6; i++) {
                    // Get neighbor position
                    int nx = x + directions[i].x;
                    int ny = y + directions[i].y;
                    int nz = z + directions[i].z;
                    
                    // Skip faces that are hidden inside the chunk
                    bool faceVisible = false;
                    if (nx < 0 || nx >= SIZE || ny < 0 || ny >= SIZE || nz < 0 || nz >= SIZE) {
                        faceVisible = true; // Face is at chunk boundary
                    } else {
                        try {
                            // Check if neighbor is air
                            Block neighborBlock = getBlock(nx, ny, nz);
                            faceVisible = (neighborBlock.type == BlockType::AIR);
                        } catch (...) {
                            faceVisible = true; // Assume visible if exception
                        }
                    }
                    
                    if (faceVisible) {
                        // Create cube face vertices in world space
                        // CRITICAL: Use EXACT 1.0 unit dimensions
                        glm::vec3 v1, v2, v3, v4;
                        float uBase = static_cast<float>(i); // Use face index for UV
                        
                        switch (i) {
                            case 0: // +X face
                                v1 = glm::vec3(worldX + 1.0f, worldY, worldZ);
                                v2 = glm::vec3(worldX + 1.0f, worldY + 1.0f, worldZ);
                                v3 = glm::vec3(worldX + 1.0f, worldY + 1.0f, worldZ + 1.0f);
                                v4 = glm::vec3(worldX + 1.0f, worldY, worldZ + 1.0f);
                                break;
                            case 1: // -X face
                                v1 = glm::vec3(worldX, worldY, worldZ);
                                v2 = glm::vec3(worldX, worldY, worldZ + 1.0f);
                                v3 = glm::vec3(worldX, worldY + 1.0f, worldZ + 1.0f);
                                v4 = glm::vec3(worldX, worldY + 1.0f, worldZ);
                                break;
                            case 2: // +Y face
                                v1 = glm::vec3(worldX, worldY + 1.0f, worldZ);
                                v2 = glm::vec3(worldX + 1.0f, worldY + 1.0f, worldZ);
                                v3 = glm::vec3(worldX + 1.0f, worldY + 1.0f, worldZ + 1.0f);
                                v4 = glm::vec3(worldX, worldY + 1.0f, worldZ + 1.0f);
                                break;
                            case 3: // -Y face
                                v1 = glm::vec3(worldX, worldY, worldZ);
                                v2 = glm::vec3(worldX, worldY, worldZ + 1.0f);
                                v3 = glm::vec3(worldX + 1.0f, worldY, worldZ + 1.0f);
                                v4 = glm::vec3(worldX + 1.0f, worldY, worldZ);
                                break;
                            case 4: // +Z face
                                v1 = glm::vec3(worldX, worldY, worldZ + 1.0f);
                                v2 = glm::vec3(worldX + 1.0f, worldY, worldZ + 1.0f);
                                v3 = glm::vec3(worldX + 1.0f, worldY + 1.0f, worldZ + 1.0f);
                                v4 = glm::vec3(worldX, worldY + 1.0f, worldZ + 1.0f);
                                break;
                            case 5: // -Z face
                                v1 = glm::vec3(worldX, worldY, worldZ);
                                v2 = glm::vec3(worldX + 1.0f, worldY, worldZ);
                                v3 = glm::vec3(worldX + 1.0f, worldY + 1.0f, worldZ);
                                v4 = glm::vec3(worldX, worldY + 1.0f, worldZ);
                                break;
                        }
                        
                        // Project vertices to sphere using identical method as collision
                        // Use renderSurfaceR instead of surfaceR for perfect alignment
                        v1 = projectToSphere(v1, renderSurfaceR, isInner, i);
                        v2 = projectToSphere(v2, renderSurfaceR, isInner, i);
                        v3 = projectToSphere(v3, renderSurfaceR, isInner, i);
                        v4 = projectToSphere(v4, renderSurfaceR, isInner, i);
                        
                        // Validate vertices (avoid NaN/Inf)
                        if (!isValidVector(v1) || !isValidVector(v2) || 
                            !isValidVector(v3) || !isValidVector(v4)) {
                            continue; // Skip invalid vertices
                        }
                        
                        // Convert back to chunk-local coordinates
                        v1 -= glm::vec3(chunkX * SIZE, chunkY * SIZE, chunkZ * SIZE);
                        v2 -= glm::vec3(chunkX * SIZE, chunkY * SIZE, chunkZ * SIZE);
                        v3 -= glm::vec3(chunkX * SIZE, chunkY * SIZE, chunkZ * SIZE);
                        v4 -= glm::vec3(chunkX * SIZE, chunkY * SIZE, chunkZ * SIZE);
                        
                        // Add vertices to mesh
                        mesh.insert(mesh.end(), {
                            v1.x, v1.y, v1.z, uBase, 0.0f,
                            v2.x, v2.y, v2.z, uBase, 1.0f,
                            v3.x, v3.y, v3.z, uBase + 1.0f, 1.0f,
                            v4.x, v4.y, v4.z, uBase + 1.0f, 0.0f
                        });
                    }
                }
            }
        } else {
            // For merged chunks, create a simple quad
            float chunkSize = SIZE * mergeFactor;
            mesh = {
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                chunkSize, 0.0f, 0.0f, 1.0f, 0.0f,
                chunkSize, chunkSize, chunkSize, 1.0f, 1.0f,
                0.0f, chunkSize, chunkSize, 0.0f, 1.0f
            };
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
        mesh = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            0.0f, 1.0f, 0.0f, 0.0f, 1.0f
        };
        meshDirty = false;
        buffersDirty = true;
    } catch (...) {
        std::cerr << "Unknown exception in regenerateMesh" << std::endl;
        // Create a minimal valid mesh to avoid rendering crashes
        mesh = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            0.0f, 1.0f, 0.0f, 0.0f, 1.0f
        };
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