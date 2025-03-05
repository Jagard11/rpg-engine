// ./src/Graphics/MeshGenerator.cpp
#include "Graphics/MeshGenerator.hpp"
#include "Debug/DebugManager.hpp"
#include <iostream>
#include "World/World.hpp"
#include "Utils/SphereUtils.hpp"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

// Implementation of the static mesh generation methods from MeshGenerator.hpp

MeshGenerator::MeshData MeshGenerator::generateChunkMesh(
    const std::vector<Block>& blocks, 
    int chunkSize,
    int chunkX, 
    int chunkY, 
    int chunkZ
) {
    MeshData result;
    
    if (blocks.empty()) {
        if (DebugManager::getInstance().logChunkUpdates()) {
            std::cout << "Empty blocks array for chunk (" << chunkX << ", " << chunkY << ", " << chunkZ << ")" << std::endl;
        }
        return result;
    }
    
    // Define constants
    const glm::ivec3 directions[6] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
    };
    
    // Identify visible blocks
    std::vector<std::tuple<int, int, int, BlockType>> visibleBlocks;
    
    // Find all non-air blocks with at least one exposed face
    for (int x = 0; x < chunkSize; x++) {
        for (int y = 0; y < chunkSize; y++) {
            for (int z = 0; z < chunkSize; z++) {
                // Get block at this position
                int index = x + y * chunkSize + z * chunkSize * chunkSize;
                
                if (index >= 0 && index < static_cast<int>(blocks.size())) {
                    Block block = blocks[index];
                    
                    if (block.type != BlockType::AIR) {
                        // Check all 6 neighboring directions
                        bool hasVisibleFace = false;
                        
                        for (int i = 0; i < 6; i++) {
                            int nx = x + directions[i].x;
                            int ny = y + directions[i].y;
                            int nz = z + directions[i].z;
                            
                            // If neighbor is out of bounds or is air, this face is visible
                            if (nx < 0 || nx >= chunkSize || ny < 0 || ny >= chunkSize || nz < 0 || nz >= chunkSize) {
                                hasVisibleFace = true;
                                break;
                            }
                            
                            // Check if neighbor block is air
                            int neighborIndex = nx + ny * chunkSize + nz * chunkSize * chunkSize;
                            if (neighborIndex >= 0 && neighborIndex < static_cast<int>(blocks.size())) {
                                if (blocks[neighborIndex].type == BlockType::AIR) {
                                    hasVisibleFace = true;
                                    break;
                                }
                            } else {
                                // Assume visible if index out of bounds
                                hasVisibleFace = true;
                                break;
                            }
                        }
                        
                        if (hasVisibleFace) {
                            visibleBlocks.push_back(std::make_tuple(x, y, z, block.type));
                        }
                    }
                }
            }
        }
    }
    
    // Debug log visible blocks count
    if (DebugManager::getInstance().logChunkUpdates() && visibleBlocks.size() > 0) {
        std::cout << "Found " << visibleBlocks.size() << " visible blocks in chunk (" 
                  << chunkX << ", " << chunkY << ", " << chunkZ << ")" << std::endl;
    }
    
    // Generate mesh for each visible block
    for (const auto& [x, y, z, type] : visibleBlocks) {
        // Generate simple cube vertices directly in local chunk space
        float localX = static_cast<float>(x);
        float localY = static_cast<float>(y);
        float localZ = static_cast<float>(z);
        
        // Check all 6 directions for visible faces
        for (int i = 0; i < 6; i++) {
            // Get neighbor position
            int nx = x + directions[i].x;
            int ny = y + directions[i].y;
            int nz = z + directions[i].z;
            
            // Skip faces that are hidden inside the chunk
            bool faceVisible = false;
            if (nx < 0 || nx >= chunkSize || ny < 0 || ny >= chunkSize || nz < 0 || nz >= chunkSize) {
                faceVisible = true; // Face is at chunk boundary
            } else {
                // Check if neighbor is air
                int neighborIndex = nx + ny * chunkSize + nz * chunkSize * chunkSize;
                if (neighborIndex >= 0 && neighborIndex < static_cast<int>(blocks.size())) {
                    faceVisible = (blocks[neighborIndex].type == BlockType::AIR);
                } else {
                    faceVisible = true; // Assume visible if index out of bounds
                }
            }
            
            if (faceVisible) {
                // Add the face to the mesh using a simple cube geometry
                addCubeFaceToMesh(result, i, localX, localY, localZ, static_cast<int>(type));
            }
        }
    }
    
    return result;
}

MeshGenerator::MeshData MeshGenerator::generateLODChunkMesh(int chunkSize, int mergeFactor) {
    MeshData result;
    float size = chunkSize * mergeFactor;
    
    // Create a simple quad for distant chunks
    result.vertices = {
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        size, 0.0f, 0.0f, 1.0f, 0.0f,
        size, size, size, 1.0f, 1.0f,
        0.0f, size, size, 0.0f, 1.0f
    };
    
    // Add indices for the quad
    result.indices = {
        0, 1, 2,
        0, 2, 3
    };
    
    return result;
}

MeshGenerator::MeshData MeshGenerator::generateFallbackMesh() {
    MeshData result;
    
    // Create a simple quad as fallback
    result.vertices = {
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f
    };
    
    result.indices = {
        0, 1, 2,
        0, 2, 3
    };
    
    return result;
}

// Add a simple cube face to the mesh
void MeshGenerator::addCubeFaceToMesh(
    MeshData& mesh,
    int faceIndex,
    float x,
    float y,
    float z,
    int blockType
) {
    // Define the 8 corners of a unit cube
    static const float cubeVertices[8][3] = {
        {0.0f, 0.0f, 0.0f}, // 0: bottom-left-back
        {1.0f, 0.0f, 0.0f}, // 1: bottom-right-back
        {1.0f, 1.0f, 0.0f}, // 2: top-right-back
        {0.0f, 1.0f, 0.0f}, // 3: top-left-back
        {0.0f, 0.0f, 1.0f}, // 4: bottom-left-front
        {1.0f, 0.0f, 1.0f}, // 5: bottom-right-front
        {1.0f, 1.0f, 1.0f}, // 6: top-right-front
        {0.0f, 1.0f, 1.0f}  // 7: top-left-front
    };

    // Define indices for each face of the cube
    static const int faceIndices[6][4] = {
        {1, 5, 6, 2}, // +X face (right)
        {0, 3, 7, 4}, // -X face (left)
        {3, 2, 6, 7}, // +Y face (top)
        {0, 4, 5, 1}, // -Y face (bottom)
        {4, 7, 6, 5}, // +Z face (front)
        {0, 1, 2, 3}  // -Z face (back)
    };

    // Get texture coordinates based on block type
    float tileSize = 0.25f; // Each tile is 1/4 of the texture atlas
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    
    // Set the texture offset based on block type
    // IMPORTANT: Fixed texture mapping to make GRASS blocks display correctly
    switch (static_cast<BlockType>(blockType)) {
        case BlockType::GRASS:
            offsetX = 0.0f; // First tile (green in the texture atlas)
            offsetY = 0.0f;
            break;
        case BlockType::DIRT:
            offsetX = tileSize; // Second tile (brown in the texture atlas)
            offsetY = 0.0f;
            break;
        default:
            offsetX = 0.0f;
            offsetY = 0.0f;
            break;
    }
    
    // Each face should use the full tile, not just a corner
    float texCoords[4][2] = {
        {offsetX,         offsetY},         // Bottom-left of the tile
        {offsetX+tileSize, offsetY},         // Bottom-right of the tile
        {offsetX+tileSize, offsetY+tileSize}, // Top-right of the tile
        {offsetX,         offsetY+tileSize}  // Top-left of the tile
    };

    // Get current number of vertices in the mesh
    unsigned int baseIndex = static_cast<unsigned int>(mesh.vertices.size() / 5);

    // Add vertices for this face
    for (int i = 0; i < 4; i++) {
        int vertIndex = faceIndices[faceIndex][i];
        
        // Add vertex position with block offset
        mesh.vertices.push_back(x + cubeVertices[vertIndex][0]);
        mesh.vertices.push_back(y + cubeVertices[vertIndex][1]);
        mesh.vertices.push_back(z + cubeVertices[vertIndex][2]);
        
        // Add texture coordinates
        mesh.vertices.push_back(texCoords[i][0]);
        mesh.vertices.push_back(texCoords[i][1]);
    }
    
    // Add indices for the quad (2 triangles)
    mesh.indices.push_back(baseIndex);
    mesh.indices.push_back(baseIndex + 1);
    mesh.indices.push_back(baseIndex + 2);
    
    mesh.indices.push_back(baseIndex);
    mesh.indices.push_back(baseIndex + 2);
    mesh.indices.push_back(baseIndex + 3);
}

// This function is now a wrapper for the cube face method above
void MeshGenerator::addFrustumFaceToMesh(
    MeshData& mesh, 
    int faceIndex, 
    int localX, 
    int localY, 
    int localZ, 
    double distFromCenter,
    int blockType,
    int chunkX,
    int chunkY,
    int chunkZ,
    int chunkSize
) {
    // For compatibility, simply forward to the new method
    addCubeFaceToMesh(mesh, faceIndex, static_cast<float>(localX), static_cast<float>(localY), 
                      static_cast<float>(localZ), blockType);
}

bool MeshGenerator::isValidVector(const glm::dvec3& v) {
    return !std::isnan(v.x) && !std::isinf(v.x) &&
           !std::isnan(v.y) && !std::isinf(v.y) &&
           !std::isnan(v.z) && !std::isinf(v.z);
}