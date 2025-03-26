#include "world/Chunk.hpp"
#include <fstream>

Chunk::Chunk(int x, int z)
    : m_x(x)
    , m_z(z)
    , m_isDirty(true)
{
    // Initialize blocks to air (0)
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                m_blocks[x][y][z] = 0;
            }
        }
    }
}

Chunk::~Chunk() {
}

void Chunk::setBlock(int x, int y, int z, int blockType) {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) {
        return;
    }
    
    if (m_blocks[x][y][z] != blockType) {
        m_blocks[x][y][z] = blockType;
        m_isDirty = true;
        m_collisionMeshDirty = true; // Mark collision mesh dirty when blocks change
    }
}

int Chunk::getBlock(int x, int y, int z) const {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) {
        return 0; // Return air for out of bounds
    }
    return m_blocks[x][y][z];
}

void Chunk::generateMesh() {
    m_meshVertices.clear();
    m_meshIndices.clear();

    // Vertex data for a cube face (2 triangles)
    const std::vector<float> frontFace = {
        0.0f, 0.0f, 1.0f,  0.0f, 0.0f,  // Bottom-left
        1.0f, 0.0f, 1.0f,  1.0f, 0.0f,  // Bottom-right
        1.0f, 1.0f, 1.0f,  1.0f, 1.0f,  // Top-right
        0.0f, 1.0f, 1.0f,  0.0f, 1.0f   // Top-left
    };

    const std::vector<float> backFace = {
        1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        1.0f, 1.0f, 0.0f,  0.0f, 1.0f
    };

    const std::vector<float> leftFace = {
        0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
        0.0f, 1.0f, 1.0f,  1.0f, 1.0f,
        0.0f, 1.0f, 0.0f,  0.0f, 1.0f
    };

    const std::vector<float> rightFace = {
        1.0f, 0.0f, 1.0f,  0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,  0.0f, 1.0f
    };

    const std::vector<float> topFace = {
        0.0f, 1.0f, 1.0f,  0.0f, 0.0f,
        1.0f, 1.0f, 1.0f,  1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        0.0f, 1.0f, 0.0f,  0.0f, 1.0f
    };

    const std::vector<float> bottomFace = {
        0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        1.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        0.0f, 0.0f, 1.0f,  0.0f, 1.0f
    };

    // Iterate through all blocks in the chunk
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                if (m_blocks[x][y][z] == 0) continue; // Skip air blocks

                glm::vec3 position(x, y, z);

                // Check each face
                if (shouldRenderFace(x, y, z, glm::vec3(0, 0, 1))) addFace(frontFace, position, glm::vec3(0, 0, 1));
                if (shouldRenderFace(x, y, z, glm::vec3(0, 0, -1))) addFace(backFace, position, glm::vec3(0, 0, -1));
                if (shouldRenderFace(x, y, z, glm::vec3(-1, 0, 0))) addFace(leftFace, position, glm::vec3(-1, 0, 0));
                if (shouldRenderFace(x, y, z, glm::vec3(1, 0, 0))) addFace(rightFace, position, glm::vec3(1, 0, 0));
                if (shouldRenderFace(x, y, z, glm::vec3(0, 1, 0))) addFace(topFace, position, glm::vec3(0, 1, 0));
                if (shouldRenderFace(x, y, z, glm::vec3(0, -1, 0))) addFace(bottomFace, position, glm::vec3(0, -1, 0));
            }
        }
    }

    m_isDirty = false;
}

bool Chunk::shouldRenderFace(int x, int y, int z, const glm::vec3& normal) const {
    // Get the block in the direction of the normal
    int checkX = x + static_cast<int>(normal.x);
    int checkY = y + static_cast<int>(normal.y);
    int checkZ = z + static_cast<int>(normal.z);

    // If the adjacent block is outside the chunk, render the face
    if (checkX < 0 || checkX >= CHUNK_SIZE || 
        checkY < 0 || checkY >= CHUNK_HEIGHT || 
        checkZ < 0 || checkZ >= CHUNK_SIZE) {
        return true;
    }

    // Render the face if the adjacent block is air (transparent)
    return m_blocks[checkX][checkY][checkZ] == 0;
}

void Chunk::addFace(const std::vector<float>& vertices, const glm::vec3& position, const glm::vec3& normal) {
    unsigned int indexOffset = m_meshVertices.size() / 5; // 5 floats per vertex (3 position + 2 UV)

    // Add vertices
    for (size_t i = 0; i < vertices.size(); i += 5) {
        // Position
        m_meshVertices.push_back(vertices[i] + position.x);
        m_meshVertices.push_back(vertices[i + 1] + position.y);
        m_meshVertices.push_back(vertices[i + 2] + position.z);
        // UV coordinates
        m_meshVertices.push_back(vertices[i + 3]);
        m_meshVertices.push_back(vertices[i + 4]);
    }

    // Add indices for two triangles
    m_meshIndices.push_back(indexOffset);
    m_meshIndices.push_back(indexOffset + 1);
    m_meshIndices.push_back(indexOffset + 2);
    m_meshIndices.push_back(indexOffset);
    m_meshIndices.push_back(indexOffset + 2);
    m_meshIndices.push_back(indexOffset + 3);
}

bool Chunk::serialize(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Write chunk position
    file.write(reinterpret_cast<const char*>(&m_x), sizeof(m_x));
    file.write(reinterpret_cast<const char*>(&m_z), sizeof(m_z));

    // Write block data
    file.write(reinterpret_cast<const char*>(m_blocks.data()), 
               sizeof(int) * CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE);

    return true;
}

bool Chunk::deserialize(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Read chunk position
    file.read(reinterpret_cast<char*>(&m_x), sizeof(m_x));
    file.read(reinterpret_cast<char*>(&m_z), sizeof(m_z));

    // Read block data
    file.read(reinterpret_cast<char*>(m_blocks.data()),
              sizeof(int) * CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE);

    m_isDirty = true;
    return true;
}

std::vector<AABB> Chunk::buildColliderMesh() const {
    // Return cached collision mesh if not dirty
    if (!m_collisionMeshDirty && !m_collisionMesh.empty()) {
        return m_collisionMesh;
    }
    
    std::vector<AABB> colliderVolumes;
    
    // 3D array to track which blocks have been processed already
    bool processed[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE] = {{{false}}};
    
    // First pass: Combine blocks along X axis
    for (int y = 0; y < CHUNK_HEIGHT; ++y) {
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            for (int x = 0; x < CHUNK_SIZE; ++x) {
                // Skip if already processed or not solid
                int blockID = getBlock(x, y, z);
                if (processed[x][y][z] || !isBlockSolid(blockID)) {
                    continue;
                }
                
                // Find max extent in X direction
                int maxX = x;
                while (maxX + 1 < CHUNK_SIZE && 
                       !processed[maxX + 1][y][z] && 
                       isBlockSolid(getBlock(maxX + 1, y, z)) &&
                       getBlock(maxX + 1, y, z) == blockID) {
                    maxX++;
                }
                
                // Find max extent in Z direction
                int maxZ = z;
                bool canExpandZ = true;
                
                while (canExpandZ && maxZ + 1 < CHUNK_SIZE) {
                    // Check if the entire range from x to maxX can be expanded in Z
                    for (int testX = x; testX <= maxX; testX++) {
                        if (processed[testX][y][maxZ + 1] || 
                            !isBlockSolid(getBlock(testX, y, maxZ + 1)) ||
                            getBlock(testX, y, maxZ + 1) != blockID) {
                            canExpandZ = false;
                            break;
                        }
                    }
                    
                    if (canExpandZ) {
                        maxZ++;
                    }
                }
                
                // Find max extent in Y direction
                int maxY = y;
                bool canExpandY = true;
                
                while (canExpandY && maxY + 1 < CHUNK_HEIGHT) {
                    // Check if the entire XZ plane can be expanded in Y
                    for (int testZ = z; testZ <= maxZ; testZ++) {
                        for (int testX = x; testX <= maxX; testX++) {
                            if (processed[testX][maxY + 1][testZ] || 
                                !isBlockSolid(getBlock(testX, maxY + 1, testZ)) ||
                                getBlock(testX, maxY + 1, testZ) != blockID) {
                                canExpandY = false;
                                break;
                            }
                        }
                        if (!canExpandY) break;
                    }
                    
                    if (canExpandY) {
                        maxY++;
                    }
                }
                
                // Mark all blocks in this volume as processed
                for (int testY = y; testY <= maxY; testY++) {
                    for (int testZ = z; testZ <= maxZ; testZ++) {
                        for (int testX = x; testX <= maxX; testX++) {
                            processed[testX][testY][testZ] = true;
                        }
                    }
                }
                
                // Create a single AABB for this merged volume
                glm::vec3 worldMin(x + m_x * CHUNK_SIZE, y, z + m_z * CHUNK_SIZE);
                glm::vec3 worldMax(maxX + 1 + m_x * CHUNK_SIZE, maxY + 1, maxZ + 1 + m_z * CHUNK_SIZE);
                
                colliderVolumes.emplace_back(worldMin, worldMax);
            }
        }
    }
    
    // Cache the result
    m_collisionMesh = colliderVolumes;
    m_collisionMeshDirty = false;
    
    return colliderVolumes;
} 