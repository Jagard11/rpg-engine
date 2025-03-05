// ./include/Graphics/MeshGenerator.hpp
#ifndef MESH_GENERATOR_HPP
#define MESH_GENERATOR_HPP

#include <GL/glew.h>
#include <vector>
#include <tuple>
#include <glm/glm.hpp>
#include "World/Block.hpp"
#include "Utils/PlanetConfig.hpp"

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
     * Creates frustum-shaped voxels that taper toward the planet core
     * 
     * @param blocks Array of blocks in the chunk
     * @param chunkSize Size of chunk in one dimension (typically 16)
     * @param chunkX Chunk X coordinate in world space
     * @param chunkY Chunk Y coordinate in world space
     * @param chunkZ Chunk Z coordinate in world space
     * @return MeshData containing vertices and indices for rendering
     */
    static MeshData generateChunkMesh(
        const std::vector<Block>& blocks, 
        int chunkSize,
        int chunkX, 
        int chunkY, 
        int chunkZ
    );
    
    /**
     * Generate a simple quad mesh for LOD chunks
     * 
     * @param chunkSize Size of chunk in one dimension
     * @param mergeFactor The LOD merge factor (2, 4, 8, etc.)
     * @return MeshData containing vertices and indices for rendering
     */
    static MeshData generateLODChunkMesh(int chunkSize, int mergeFactor);
    
    /**
     * Generate a fallback mesh in case of errors
     * This creates a simple quad that won't crash the renderer
     * 
     * @return MeshData containing a simple quad
     */
    static MeshData generateFallbackMesh();

private:
    /**
     * Add a simple cube face to the mesh (for consistent appearance with collision)
     * 
     * @param mesh The mesh data to add the face to
     * @param faceIndex The index of the face (0-5 for each direction)
     * @param x Local X coordinate of the block within the chunk
     * @param y Local Y coordinate of the block within the chunk
     * @param z Local Z coordinate of the block within the chunk
     * @param blockType The type of the block
     */
    static void addCubeFaceToMesh(
        MeshData& mesh,
        int faceIndex,
        float x,
        float y,
        float z,
        int blockType
    );

    /**
     * Helper function to add a frustum-shaped face to the mesh
     * Creates voxel faces that taper toward the planet core
     * Optimized for Earth-scale rendering with proper block shaping
     * 
     * @param mesh The mesh data to add the face to
     * @param faceIndex The index of the face (0-5 for each direction)
     * @param worldX World X coordinate of the block
     * @param worldY World Y coordinate of the block
     * @param worldZ World Z coordinate of the block
     * @param distFromCenter Distance from planet center in meters
     * @param blockType The type of the block
     * @param chunkX Chunk X coordinate
     * @param chunkY Chunk Y coordinate
     * @param chunkZ Chunk Z coordinate
     * @param chunkSize Size of the chunk in one dimension
     */
    static void addFrustumFaceToMesh(
        MeshData& mesh, 
        int faceIndex, 
        int worldX, 
        int worldY, 
        int worldZ, 
        double distFromCenter,
        int blockType,
        int chunkX,
        int chunkY,
        int chunkZ,
        int chunkSize
    );
    
    // Helper function to check if a vector contains NaN or Inf values
    static bool isValidVector(const glm::dvec3& v);
};

#endif // MESH_GENERATOR_HPP