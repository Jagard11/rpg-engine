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
        // Calculate world position
        int worldX = chunkX * chunkSize + x;
        int worldY = chunkY * chunkSize + y;
        int worldZ = chunkZ * chunkSize + z;
        
        // Calculate block center in world space (using double precision)
        glm::dvec3 blockCenter(
            static_cast<double>(worldX) + 0.5,
            static_cast<double>(worldY) + 0.5,
            static_cast<double>(worldZ) + 0.5
        );
        
        // Calculate distance from center (using double precision for accuracy)
        double distFromCenter = glm::length(blockCenter);
        
        // EARTH-SCALE FIX: Handle extreme distances gracefully
        if (std::isnan(distFromCenter) || std::isinf(distFromCenter) || distFromCenter > 1.0e10) {
            if (DebugManager::getInstance().logChunkUpdates()) {
                std::cout << "Warning: Extreme block position detected at " 
                          << worldX << ", " << worldY << ", " << worldZ 
                          << " (distance: " << distFromCenter << ")" << std::endl;
            }
            continue;
        }
        
        // EARTH-SCALE FIX: Ensure distance from center is non-zero to avoid division by zero
        if (distFromCenter < 0.001) {
            distFromCenter = 0.001;
        }
        
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
                // Add the face to the mesh - using the frustum shape
                addFrustumFaceToMesh(result, i, x, y, z, distFromCenter, 
                                     static_cast<int>(type), chunkX, chunkY, chunkZ, chunkSize);
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
    // EARTH-SCALE FIX: Calculate world coordinates
    double worldX = chunkX * chunkSize + localX;
    double worldY = chunkY * chunkSize + localY;
    double worldZ = chunkZ * chunkSize + localZ;
    
    // EARTH-SCALE FIX: Get the voxel width at this distance from center
    double voxelWidth = SphereUtils::getVoxelWidthAt(distFromCenter);
    
    // Calculate block center in world space
    glm::dvec3 blockCenter(worldX + 0.5, worldY + 0.5, worldZ + 0.5);
    
    // Calculate direction from center to block
    glm::dvec3 dirFromCenter = glm::normalize(blockCenter);
    
    // EARTH-SCALE FIX: Calculate the tapering factor - lower bound it to avoid degenerate geometry
    double taperingFactor = 0.95;  // Default conservative value
    
    // Only calculate actual tapering if we're far enough from center to avoid instability
    if (distFromCenter > 100.0) {
        double innerRadius = distFromCenter - PlanetConfig::VOXEL_HEIGHT_METERS;
        double innerWidth = SphereUtils::getVoxelWidthAt(innerRadius);
        taperingFactor = innerWidth / voxelWidth;
        
        // Safety clamp to avoid degenerate geometry
        taperingFactor = std::max(0.8, std::min(1.0, taperingFactor));
    }
    
    // Create the frustum vertices
    glm::dvec3 vert1, vert2, vert3, vert4;
    
    // Texture coordinates based on block type
    float texU = 0.0f, texV = 0.0f;
    
    // EARTH-SCALE FIX: Set texture coordinates based on block type
    switch (static_cast<BlockType>(blockType)) {
        case BlockType::DIRT:
            texU = 0.0f;
            texV = 0.0f;
            break;
        case BlockType::GRASS:
            texU = 0.25f;
            texV = 0.0f;
            break;
        default:
            texU = 0.0f;
            texV = 0.0f;
            break;
    }
    
    // Create orthogonal basis for the block
    glm::dvec3 up = dirFromCenter;  // Up is away from planet center
    
    // EARTH-SCALE FIX: Create stable right and forward vectors
    glm::dvec3 reference = (std::abs(up.y) > 0.99) ? glm::dvec3(1.0, 0.0, 0.0) : glm::dvec3(0.0, 1.0, 0.0);
    glm::dvec3 right = glm::normalize(glm::cross(reference, up));
    glm::dvec3 forward = glm::normalize(glm::cross(up, right));
    
    // Half sizes for top and bottom faces
    double halfSize = 0.5; // Standard half-size
    double bottomHalfSize = halfSize * taperingFactor; // Tapered for bottom face
    
    // EARTH-SCALE FIX: Generate vertices in local coordinates (relative to chunk)
    // This avoids extreme coordinate values that cause precision issues
    // We're building the mesh in chunk-local space, but transforming by a planet-centric basis
    
    // Generate vertices for this face
    switch (faceIndex) {
        case 0: // +X face
            vert1 = glm::dvec3(localX + 1, localY    , localZ    ) + bottomHalfSize * (up - right - forward);
            vert2 = glm::dvec3(localX + 1, localY    , localZ + 1) + bottomHalfSize * (up - right + forward);
            vert3 = glm::dvec3(localX + 1, localY + 1, localZ + 1) + halfSize * (up + right + forward);
            vert4 = glm::dvec3(localX + 1, localY + 1, localZ    ) + halfSize * (up + right - forward);
            break;
        case 1: // -X face
            vert1 = glm::dvec3(localX    , localY    , localZ    ) + bottomHalfSize * (up - right - forward);
            vert2 = glm::dvec3(localX    , localY    , localZ + 1) + bottomHalfSize * (up - right + forward);
            vert3 = glm::dvec3(localX    , localY + 1, localZ + 1) + halfSize * (up + right + forward);
            vert4 = glm::dvec3(localX    , localY + 1, localZ    ) + halfSize * (up + right - forward);
            break;
        case 2: // +Y face (top)
            vert1 = glm::dvec3(localX    , localY + 1, localZ    ) + halfSize * (up + right - forward);
            vert2 = glm::dvec3(localX    , localY + 1, localZ + 1) + halfSize * (up + right + forward);
            vert3 = glm::dvec3(localX + 1, localY + 1, localZ + 1) + halfSize * (up + right + forward);
            vert4 = glm::dvec3(localX + 1, localY + 1, localZ    ) + halfSize * (up + right - forward);
            break;
        case 3: // -Y face (bottom)
            vert1 = glm::dvec3(localX    , localY    , localZ    ) + bottomHalfSize * (up - right - forward);
            vert2 = glm::dvec3(localX    , localY    , localZ + 1) + bottomHalfSize * (up - right + forward);
            vert3 = glm::dvec3(localX + 1, localY    , localZ + 1) + bottomHalfSize * (up - right + forward);
            vert4 = glm::dvec3(localX + 1, localY    , localZ    ) + bottomHalfSize * (up - right - forward);
            break;
        case 4: // +Z face
            vert1 = glm::dvec3(localX    , localY    , localZ + 1) + bottomHalfSize * (up - right + forward);
            vert2 = glm::dvec3(localX + 1, localY    , localZ + 1) + bottomHalfSize * (up - right + forward);
            vert3 = glm::dvec3(localX + 1, localY + 1, localZ + 1) + halfSize * (up + right + forward);
            vert4 = glm::dvec3(localX    , localY + 1, localZ + 1) + halfSize * (up + right + forward);
            break;
        case 5: // -Z face
            vert1 = glm::dvec3(localX    , localY    , localZ    ) + bottomHalfSize * (up - right - forward);
            vert2 = glm::dvec3(localX + 1, localY    , localZ    ) + bottomHalfSize * (up - right - forward);
            vert3 = glm::dvec3(localX + 1, localY + 1, localZ    ) + halfSize * (up + right - forward);
            vert4 = glm::dvec3(localX    , localY + 1, localZ    ) + halfSize * (up + right - forward);
            break;
    }
    
    // Validate vertices (avoid NaN/Inf)
    if (!isValidVector(vert1) || !isValidVector(vert2) || !isValidVector(vert3) || !isValidVector(vert4)) {
        if (DebugManager::getInstance().logChunkUpdates()) {
            std::cout << "Warning: Invalid frustum vertex detected for block at " 
                     << worldX << ", " << worldY << ", " << worldZ << std::endl;
        }
        return; // Skip invalid vertices
    }
    
    // Get current number of vertices
    unsigned int baseIndex = static_cast<unsigned int>(mesh.vertices.size() / 5);
    
    // UV coordinates for the face
    float u0 = texU;
    float u1 = texU + 0.25f; // Texture atlas cell width (assuming 4x4 atlas)
    float v0 = texV;
    float v1 = texV + 0.25f; // Texture atlas cell height
    
    // EARTH-SCALE FIX: Adjust vertex addition - we're adding the local coordinates directly to the mesh
    // Add vertices to mesh (converting from double to float)
    mesh.vertices.push_back(static_cast<float>(vert1.x));
    mesh.vertices.push_back(static_cast<float>(vert1.y));
    mesh.vertices.push_back(static_cast<float>(vert1.z));
    mesh.vertices.push_back(u0);  // Texture U
    mesh.vertices.push_back(v0);  // Texture V
    
    mesh.vertices.push_back(static_cast<float>(vert2.x));
    mesh.vertices.push_back(static_cast<float>(vert2.y));
    mesh.vertices.push_back(static_cast<float>(vert2.z));
    mesh.vertices.push_back(u1);  // Texture U
    mesh.vertices.push_back(v0);  // Texture V
    
    mesh.vertices.push_back(static_cast<float>(vert3.x));
    mesh.vertices.push_back(static_cast<float>(vert3.y));
    mesh.vertices.push_back(static_cast<float>(vert3.z));
    mesh.vertices.push_back(u1);  // Texture U
    mesh.vertices.push_back(v1);  // Texture V
    
    mesh.vertices.push_back(static_cast<float>(vert4.x));
    mesh.vertices.push_back(static_cast<float>(vert4.y));
    mesh.vertices.push_back(static_cast<float>(vert4.z));
    mesh.vertices.push_back(u0);  // Texture U
    mesh.vertices.push_back(v1);  // Texture V
    
    // Add indices for the quad (2 triangles)
    mesh.indices.push_back(baseIndex);
    mesh.indices.push_back(baseIndex + 1);
    mesh.indices.push_back(baseIndex + 2);
    
    mesh.indices.push_back(baseIndex);
    mesh.indices.push_back(baseIndex + 2);
    mesh.indices.push_back(baseIndex + 3);
}

bool MeshGenerator::isValidVector(const glm::dvec3& v) {
    return !std::isnan(v.x) && !std::isinf(v.x) &&
           !std::isnan(v.y) && !std::isinf(v.y) &&
           !std::isnan(v.z) && !std::isinf(v.z);
}