#include "world/Chunk.hpp"
#include "world/World.hpp"
#include <fstream>
#include <iostream>

Chunk::Chunk(int x, int y, int z)
    : m_x(x)
    , m_y(y)
    , m_z(z)
    , m_isDirty(true)
    , m_world(nullptr)
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

    // Create a 3D visited array to track merged faces
    bool visited[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE][6] = {false};
    
    // Define the six face directions
    const glm::ivec3 directions[6] = {
        glm::ivec3(0, 0, 1),   // Front  (+Z)
        glm::ivec3(0, 0, -1),  // Back   (-Z)
        glm::ivec3(-1, 0, 0),  // Left   (-X)
        glm::ivec3(1, 0, 0),   // Right  (+X)
        glm::ivec3(0, 1, 0),   // Top    (+Y)
        glm::ivec3(0, -1, 0)   // Bottom (-Y)
    };
    
    // First scan the entire chunk to identify faces that need rendering
    // This helps prevent z-fighting at chunk boundaries
    bool needsRender[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE][6] = {false};
    int faceCount = 0;
    
    std::cout << "Generating mesh for chunk (" << m_x << ", " << m_y << ", " << m_z << ")" << std::endl;
    
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                if (getBlock(x, y, z) == 0) continue; // Skip air blocks
                
                for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
                    if (shouldRenderFace(x, y, z, directions[faceIndex])) {
                        needsRender[x][y][z][faceIndex] = true;
                        faceCount++;
                    }
                }
            }
        }
    }
    
    std::cout << "Found " << faceCount << " faces to render in chunk (" << m_x << ", " << m_y << ", " << m_z << ")" << std::endl;
    
    // Vertex data for each face type (scaled to unit size)
    // For a 48x32 texture atlas with 6 tiles (16x16 each)
    // Layout:
    // White (0,0)     | Green (0.333,0)  | Yellow (0.667,0)
    // Black (0,0.5)   | Red (0.333,0.5)  | Purple (0.667,0.5)
    const std::vector<float> faceVertices[6] = {
        // Front face (+Z) - Purple (0.667,0.5 to 1.0,1.0)
        {
            0.0f, 0.0f, 1.0f,  0.667f, 0.5f,  // Bottom-left
            1.0f, 0.0f, 1.0f,  1.0f, 0.5f,    // Bottom-right
            1.0f, 1.0f, 1.0f,  1.0f, 1.0f,    // Top-right
            0.0f, 1.0f, 1.0f,  0.667f, 1.0f   // Top-left
        },
        // Back face (-Z) - Yellow (0.667,0.0 to 1.0,0.5)
        // Flipped so normals point outward
        {
            1.0f, 0.0f, 0.0f,  0.667f, 0.0f,  // Bottom-right (flipped)
            0.0f, 0.0f, 0.0f,  1.0f, 0.0f,    // Bottom-left (flipped)
            0.0f, 1.0f, 0.0f,  1.0f, 0.5f,    // Top-left (flipped)
            1.0f, 1.0f, 0.0f,  0.667f, 0.5f   // Top-right (flipped)
        },
        // Left face (-X) - Green (0.333,0.0 to 0.667,0.5)
        {
            0.0f, 0.0f, 0.0f,  0.333f, 0.0f,  // Bottom-left back
            0.0f, 0.0f, 1.0f,  0.667f, 0.0f,  // Bottom-right front
            0.0f, 1.0f, 1.0f,  0.667f, 0.5f,  // Top-right front
            0.0f, 1.0f, 0.0f,  0.333f, 0.5f   // Top-left back
        },
        // Right face (+X) - Black (0.0,0.5 to 0.333,1.0)
        // Flipped so normals point outward
        {
            1.0f, 0.0f, 0.0f,  0.0f, 0.5f,    // Bottom-right back (flipped)
            1.0f, 0.0f, 1.0f,  0.333f, 0.5f,  // Bottom-left front (flipped)
            1.0f, 1.0f, 1.0f,  0.333f, 1.0f,  // Top-left front (flipped)
            1.0f, 1.0f, 0.0f,  0.0f, 1.0f     // Top-right back (flipped)
        },
        // Top face (+Y) - Red (0.333,0.5 to 0.667,1.0)
        {
            0.0f, 1.0f, 1.0f,  0.333f, 0.5f,  // Bottom-left front
            1.0f, 1.0f, 1.0f,  0.667f, 0.5f,  // Bottom-right front
            1.0f, 1.0f, 0.0f,  0.667f, 1.0f,  // Top-right back
            0.0f, 1.0f, 0.0f,  0.333f, 1.0f   // Top-left back
        },
        // Bottom face (-Y) - White (0.0,0.0 to 0.333,0.5)
        // Flipped so normals point outward
        {
            0.0f, 0.0f, 1.0f,  0.0f, 0.0f,    // Bottom-right front (flipped)
            1.0f, 0.0f, 1.0f,  0.333f, 0.0f,  // Bottom-left front (flipped)
            1.0f, 0.0f, 0.0f,  0.333f, 0.5f,  // Top-left back (flipped)
            0.0f, 0.0f, 0.0f,  0.0f, 0.5f     // Top-right back (flipped)
        }
    };

    // Apply greedy meshing algorithm for each face direction
    for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
        const glm::ivec3& normal = directions[faceIndex];
        
        // Determine which dimensions to iterate for this face direction
        int u, v, w;
        if (normal.x != 0) {
            // X-axis faces (left/right)
            u = 1; v = 2; w = 0; // u=y, v=z, w=x
        } else if (normal.y != 0) {
            // Y-axis faces (top/bottom)
            u = 0; v = 2; w = 1; // u=x, v=z, w=y
        } else {
            // Z-axis faces (front/back)
            u = 0; v = 1; w = 2; // u=x, v=y, w=z
        }
        
        // For each slice along the normal direction
        for (int wPos = 0; wPos < CHUNK_SIZE; wPos++) {
            // Initialize a mask for this slice
            bool mask[CHUNK_SIZE][CHUNK_HEIGHT] = {false};
            int blockTypes[CHUNK_SIZE][CHUNK_HEIGHT] = {0};
            
            // Set up the mask based on pre-computed face visibility
            for (int uPos = 0; uPos < CHUNK_SIZE; uPos++) {
                for (int vPos = 0; vPos < CHUNK_HEIGHT; vPos++) {
                    // Convert uvw coordinates to xyz
                    int x = (u == 0) ? uPos : ((w == 0) ? wPos : vPos);
                    int y = (u == 1) ? uPos : ((w == 1) ? wPos : vPos);
                    int z = (u == 2) ? uPos : ((w == 2) ? wPos : vPos);
                    
                    // Check if this face needs rendering using pre-computed results
                    if (needsRender[x][y][z][faceIndex]) {
                        // Set mask if the block is not air
                        int blockType = getBlock(x, y, z);
                        if (blockType != 0) {
                            mask[uPos][vPos] = true;
                            blockTypes[uPos][vPos] = blockType;
                        }
                    }
                }
            }
            
            // Now apply greedy meshing to the mask
            for (int uPos = 0; uPos < CHUNK_SIZE; uPos++) {
                for (int vPos = 0; vPos < CHUNK_HEIGHT; vPos++) {
                    if (mask[uPos][vPos]) {
                        // Start with a single cell
                        int width = 1;
                        int height = 1;
                        int currBlockType = blockTypes[uPos][vPos];
                        
                        // Expand width (u-direction) as far as possible
                        while (uPos + width < CHUNK_SIZE && 
                               mask[uPos + width][vPos] && 
                               blockTypes[uPos + width][vPos] == currBlockType) {
                            width++;
                        }
                        
                        // Expand height (v-direction) as far as possible
                        bool canExpandHeight = true;
                        while (canExpandHeight && vPos + height < CHUNK_HEIGHT) {
                            // Check if the entire width can be expanded in height
                            for (int i = 0; i < width; i++) {
                                if (!mask[uPos + i][vPos + height] || 
                                    blockTypes[uPos + i][vPos + height] != currBlockType) {
                                    canExpandHeight = false;
                                    break;
                                }
                            }
                            if (canExpandHeight) {
                                height++;
                            }
                        }
                        
                        // Mark the area as visited in the mask
                        for (int i = 0; i < width; i++) {
                            for (int j = 0; j < height; j++) {
                                mask[uPos + i][vPos + j] = false;
                            }
                        }
                        
                        // Generate the quad for this merged area with appropriate depth bias
                        addGreedyFace(faceVertices[faceIndex], normal, uPos, vPos, wPos, width, height, u, v, w);
                    }
                }
            }
        }
    }

    m_isDirty = false;
}

// Helper method to add a greedy-meshed face
void Chunk::addGreedyFace(const std::vector<float>& faceTemplate, const glm::ivec3& normal, 
                          int uStart, int vStart, int wPos, int width, int height, 
                          int uAxis, int vAxis, int wAxis) {
    unsigned int indexOffset = m_meshVertices.size() / 5; // 5 floats per vertex (3 position + 2 UV)
    
    // Calculate the world coordinates of the face corners
    glm::vec3 pos;
    pos[wAxis] = wPos;
    
    // Add a very slight offset in the normal direction to prevent z-fighting at chunk boundaries
    // This is especially important at the edges of chunks
    bool isEdgeOfChunk = (wPos == 0 || wPos == CHUNK_SIZE - 1);
    float normalOffset = 0.0f;
    
    // Apply a tiny offset if this face is at the edge of a chunk
    if (isEdgeOfChunk) {
        // Offset is in the direction of the normal, away from the block
        // The offset is very small to avoid visual gaps but enough to prevent z-fighting
        normalOffset = normal[wAxis] * 0.002f;
    }
    
    // Create a temporary array to store transformed vertices
    std::vector<float> transformedVerts;
    transformedVerts.reserve(20); // 4 vertices * 5 floats (3 pos + 2 uv)
    
    // Transform the template based on width and height
    for (size_t i = 0; i < faceTemplate.size(); i += 5) {
        float posU = faceTemplate[i + uAxis];
        float posV = faceTemplate[i + vAxis];
        
        // Scale the position by width and height
        float scaledU = posU * width;
        float scaledV = posV * height;
        
        // Set the base position
        glm::vec3 vertPos;
        vertPos[uAxis] = uStart + scaledU;
        vertPos[vAxis] = vStart + scaledV;
        vertPos[wAxis] = pos[wAxis];
        
        // Apply the normal offset to prevent z-fighting
        if (isEdgeOfChunk) {
            vertPos.x += normal.x * normalOffset;
            vertPos.y += normal.y * normalOffset;
            vertPos.z += normal.z * normalOffset;
        }
        
        // Add position
        transformedVerts.push_back(vertPos.x);
        transformedVerts.push_back(vertPos.y);
        transformedVerts.push_back(vertPos.z);
        
        // Add UV coordinates - DON'T scale UV by width/height for atlas texture
        // Just use the template UVs directly to maintain the face color
        transformedVerts.push_back(faceTemplate[i + 3]);
        transformedVerts.push_back(faceTemplate[i + 4]);
    }
    
    // Add all the vertices
    for (float value : transformedVerts) {
        m_meshVertices.push_back(value);
    }
    
    // REVERSED winding order - this flips all face normals
    // First triangle: Bottom-left -> Top-left -> Top-right
    m_meshIndices.push_back(indexOffset);     // Bottom-left
    m_meshIndices.push_back(indexOffset + 3); // Top-left
    m_meshIndices.push_back(indexOffset + 2); // Top-right
    
    // Second triangle: Bottom-left -> Top-right -> Bottom-right
    m_meshIndices.push_back(indexOffset);     // Bottom-left
    m_meshIndices.push_back(indexOffset + 2); // Top-right
    m_meshIndices.push_back(indexOffset + 1); // Bottom-right
}

bool Chunk::shouldRenderFace(int x, int y, int z, const glm::vec3& normal) const {
    // Only check if the block itself is valid and not air
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE || 
        m_blocks[x][y][z] == 0) {
        return false;
    }
    
    // With backface culling disabled, we should still only render faces that are
    // adjacent to air or transparent blocks to avoid internal faces.
    
    // Get the adjacent block position
    int checkX = x + static_cast<int>(normal.x);
    int checkY = y + static_cast<int>(normal.y);
    int checkZ = z + static_cast<int>(normal.z);
    
    // If the adjacent position is within this chunk, check directly
    if (checkX >= 0 && checkX < CHUNK_SIZE && 
        checkY >= 0 && checkY < CHUNK_HEIGHT && 
        checkZ >= 0 && checkZ < CHUNK_SIZE) {
        // Only render face if adjacent block is air
        return m_blocks[checkX][checkY][checkZ] == 0;
    }
    
    // If we have a world reference and the block is outside this chunk,
    // convert to world coordinates and query the world
    if (m_world) {
        // Convert to world coordinates
        int worldX = m_x * CHUNK_SIZE + checkX;
        int worldY = m_y * CHUNK_HEIGHT + checkY;
        int worldZ = m_z * CHUNK_SIZE + checkZ;
        
        // Query the world for the block at this position
        int adjacentBlock = m_world->getBlock(glm::ivec3(worldX, worldY, worldZ));
        
        // Only render face if adjacent block is air
        return adjacentBlock == 0;
    }
    
    // Always render edges of the world where there's no neighboring chunk
    return true;
}

int Chunk::getAdjacentBlock(int x, int y, int z, const glm::vec3& normal) const {
    // Calculate adjacent block position
    int checkX = x + static_cast<int>(normal.x);
    int checkY = y + static_cast<int>(normal.y);
    int checkZ = z + static_cast<int>(normal.z);

    // If the adjacent block is within this chunk, check directly
    if (checkX >= 0 && checkX < CHUNK_SIZE && 
        checkY >= 0 && checkY < CHUNK_HEIGHT && 
        checkZ >= 0 && checkZ < CHUNK_SIZE) {
        return m_blocks[checkX][checkY][checkZ];
    }
    
    // If we have a world reference and the block is outside this chunk,
    // convert to world coordinates and query the world
    if (m_world) {
        // Convert to world coordinates
        int worldX = m_x * CHUNK_SIZE + checkX;
        int worldY = m_y * CHUNK_HEIGHT + checkY;
        int worldZ = m_z * CHUNK_SIZE + checkZ;
        
        // Query the world for the block at this position
        return m_world->getBlock(glm::ivec3(worldX, worldY, worldZ));
    }
    
    // If we don't have a world reference or can't find the block,
    // assume it's air (0) which means the face should be rendered
    return 0;
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

    // Add indices for two triangles with REVERSED winding order to match addGreedyFace
    // First triangle: Bottom-left -> Top-left -> Top-right
    m_meshIndices.push_back(indexOffset);     // Bottom-left
    m_meshIndices.push_back(indexOffset + 3); // Top-left
    m_meshIndices.push_back(indexOffset + 2); // Top-right
    
    // Second triangle: Bottom-left -> Top-right -> Bottom-right
    m_meshIndices.push_back(indexOffset);     // Bottom-left
    m_meshIndices.push_back(indexOffset + 2); // Top-right
    m_meshIndices.push_back(indexOffset + 1); // Bottom-right
}

bool Chunk::serialize(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Write chunk position (3D now)
    file.write(reinterpret_cast<const char*>(&m_x), sizeof(m_x));
    file.write(reinterpret_cast<const char*>(&m_y), sizeof(m_y));
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

    // Read chunk position (3D now)
    file.read(reinterpret_cast<char*>(&m_x), sizeof(m_x));
    file.read(reinterpret_cast<char*>(&m_y), sizeof(m_y));
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
                glm::vec3 worldMin(x + m_x * CHUNK_SIZE, y + m_y * CHUNK_HEIGHT, z + m_z * CHUNK_SIZE);
                glm::vec3 worldMax(maxX + 1 + m_x * CHUNK_SIZE, maxY + 1 + m_y * CHUNK_HEIGHT, maxZ + 1 + m_z * CHUNK_SIZE);
                
                colliderVolumes.emplace_back(worldMin, worldMax);
            }
        }
    }
    
    // Cache the result
    m_collisionMesh = colliderVolumes;
    m_collisionMeshDirty = false;
    
    return colliderVolumes;
} 