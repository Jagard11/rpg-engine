// ./include/Graphics/MeshGenerator.hpp
#ifndef MESH_GENERATOR_HPP
#define MESH_GENERATOR_HPP

// Always include GLEW first, before any GL headers
#include <GL/glew.h>
#include <vector>
#include <tuple>  // For std::make_tuple and structured bindings
#include <glm/glm.hpp>
#include "World/Block.hpp"
#include "Utils/SphereUtils.hpp"

/**
 * Mesh generation helper class to abstract the complex vertex generation logic
 * for voxel chunks on a spherical planet.
 */
class MeshGenerator {
public:
    struct MeshData {
        std::vector<float> vertices;  // Format: x, y, z, u, v for each vertex
        std::vector<unsigned int> indices;
    };
    
    /**
     * Generate a mesh for a chunk with the given parameters
     * 
     * @param blocks Array of blocks in the chunk
     * @param chunkSize Size of chunk in one dimension (typically 16)
     * @param chunkX Chunk X coordinate in world space
     * @param chunkY Chunk Y coordinate in world space
     * @param chunkZ Chunk Z coordinate in world space
     * @param planetRadius The basic radius of the planet
     * @return MeshData containing vertices and indices for rendering
     */
    static MeshData generateChunkMesh(
        const std::vector<Block>& blocks, 
        int chunkSize,
        int chunkX, 
        int chunkY, 
        int chunkZ,
        float planetRadius
    ) {
        MeshData result;
        
        if (blocks.empty()) {
            return result;
        }
        
        // Get surface radius for consistent calculations
        float surfaceR = SphereUtils::getSurfaceRadius(planetRadius);
        
        // Calculate chunk center distance from world center
        double chunkCenterX = chunkX * chunkSize + chunkSize/2.0;
        double chunkCenterY = chunkY * chunkSize + chunkSize/2.0;
        double chunkCenterZ = chunkZ * chunkSize + chunkSize/2.0;
        double chunkDist = sqrt(chunkCenterX*chunkCenterX + chunkCenterY*chunkCenterY + chunkCenterZ*chunkCenterZ);
        
        // Find all non-air blocks with at least one exposed face
        std::vector<std::tuple<int, int, int, BlockType>> visibleBlocks;
        
        // Define constants
        const glm::ivec3 directions[6] = {
            {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
        };
        
        // Identify visible blocks
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
        
        // Generate mesh for each visible block
        for (const auto& [x, y, z, type] : visibleBlocks) {
            // Calculate world position
            int worldX = chunkX * chunkSize + x;
            int worldY = chunkY * chunkSize + y;
            int worldZ = chunkZ * chunkSize + z;
            
            // Calculate block center with double precision - matching collision
            double wx = static_cast<double>(worldX) + 0.5;
            double wy = static_cast<double>(worldY) + 0.5;
            double wz = static_cast<double>(worldZ) + 0.5;
            double dist = sqrt(wx*wx + wy*wy + wz*wz);
            
            // Determine if block is inside or outside the surface
            bool isInner = (dist < surfaceR);
            
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
                    // Add the face to the mesh
                    addFaceToMesh(result, i, worldX, worldY, worldZ, surfaceR, isInner, chunkX, chunkY, chunkZ, chunkSize);
                }
            }
        }
        
        return result;
    }
    
    /**
     * Generate a simple quad mesh for LOD chunks
     * 
     * @param chunkSize Size of chunk in one dimension
     * @param mergeFactor The LOD merge factor (2, 4, 8, etc.)
     * @return MeshData containing vertices and indices for rendering
     */
    static MeshData generateLODChunkMesh(int chunkSize, int mergeFactor) {
        MeshData result;
        float size = chunkSize * mergeFactor;
        
        // Create a simple quad
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
    
    /**
     * Generate a fallback mesh in case of errors
     * This creates a simple quad that won't crash the renderer
     * 
     * @return MeshData containing a simple quad
     */
    static MeshData generateFallbackMesh() {
        MeshData result;
        
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

private:
    /**
     * Helper function to add a face to the mesh
     * 
     * @param mesh The mesh data to add the face to
     * @param faceIndex The index of the face (0-5 for each direction)
     * @param worldX World X coordinate of the block
     * @param worldY World Y coordinate of the block
     * @param worldZ World Z coordinate of the block
     * @param surfaceR Surface radius of the planet
     * @param isInner Whether the block is below the surface
     * @param chunkX Chunk X coordinate
     * @param chunkY Chunk Y coordinate
     * @param chunkZ Chunk Z coordinate
     * @param chunkSize Size of the chunk in one dimension
     */
    static void addFaceToMesh(
        MeshData& mesh, 
        int faceIndex, 
        int worldX, 
        int worldY, 
        int worldZ, 
        float surfaceR, 
        bool isInner,
        int chunkX,
        int chunkY,
        int chunkZ,
        int chunkSize
    ) {
        // Create cube face vertices in world space
        glm::vec3 v1, v2, v3, v4;
        float uBase = static_cast<float>(faceIndex); // Use face index for UV
        
        switch (faceIndex) {
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
        
        // Project vertices to sphere using SphereUtils
        v1 = SphereUtils::projectToSphere(v1, surfaceR, isInner, faceIndex);
        v2 = SphereUtils::projectToSphere(v2, surfaceR, isInner, faceIndex);
        v3 = SphereUtils::projectToSphere(v3, surfaceR, isInner, faceIndex);
        v4 = SphereUtils::projectToSphere(v4, surfaceR, isInner, faceIndex);
        
        // Validate vertices (avoid NaN/Inf)
        if (!isValidVector(v1) || !isValidVector(v2) || !isValidVector(v3) || !isValidVector(v4)) {
            return; // Skip invalid vertices
        }
        
        // Convert back to chunk-local coordinates
        v1 -= glm::vec3(chunkX * chunkSize, chunkY * chunkSize, chunkZ * chunkSize);
        v2 -= glm::vec3(chunkX * chunkSize, chunkY * chunkSize, chunkZ * chunkSize);
        v3 -= glm::vec3(chunkX * chunkSize, chunkY * chunkSize, chunkZ * chunkSize);
        v4 -= glm::vec3(chunkX * chunkSize, chunkY * chunkSize, chunkZ * chunkSize);
        
        // Get current number of vertices
        unsigned int baseIndex = static_cast<unsigned int>(mesh.vertices.size() / 5);
        
        // Add vertices to mesh
        mesh.vertices.insert(mesh.vertices.end(), {
            v1.x, v1.y, v1.z, uBase, 0.0f,
            v2.x, v2.y, v2.z, uBase, 1.0f,
            v3.x, v3.y, v3.z, uBase + 1.0f, 1.0f,
            v4.x, v4.y, v4.z, uBase + 1.0f, 0.0f
        });
        
        // Add indices for the quad (2 triangles)
        mesh.indices.insert(mesh.indices.end(), {
            baseIndex, baseIndex + 1, baseIndex + 2,
            baseIndex, baseIndex + 2, baseIndex + 3
        });
    }
    
    // Helper function to check if a vector contains NaN or Inf values
    static bool isValidVector(const glm::vec3& v) {
        return !std::isnan(v.x) && !std::isinf(v.x) &&
               !std::isnan(v.y) && !std::isinf(v.y) &&
               !std::isnan(v.z) && !std::isinf(v.z);
    }
};

#endif // MESH_GENERATOR_HPP