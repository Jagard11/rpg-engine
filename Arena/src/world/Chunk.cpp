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

void Chunk::generateMesh(bool disableGreedyMeshing) {
    m_meshVertices.clear();
    m_meshIndices.clear();
    
    // Define the six face directions
    const glm::ivec3 directions[6] = {
        glm::ivec3(0, 0, 1),   // Front  (+Z)
        glm::ivec3(0, 0, -1),  // Back   (-Z)
        glm::ivec3(-1, 0, 0),  // Left   (-X)
        glm::ivec3(1, 0, 0),   // Right  (+X)
        glm::ivec3(0, 1, 0),   // Top    (+Y)
        glm::ivec3(0, -1, 0)   // Bottom (-Y)
    };
    
    std::cout << "Generating mesh for chunk (" << m_x << ", " << m_y << ", " << m_z 
              << ") with greedy meshing " << (disableGreedyMeshing ? "DISABLED" : "ENABLED") << std::endl;
    
    // STEP 1: FACE CULLING - Determine which faces need to be rendered
    // Face culling rules:
    // 1. If the adjacent block is transparent (air, glass, etc.), render the face
    // 2. If the adjacent block is a different type than the current block (and current is not transparent), render the face
    // 3. Otherwise, the face is hidden and should be culled
    // This is done BEFORE any greedy meshing - we determine visibility first
    bool needsRender[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE][6] = {false};
    int faceCount = 0;
    
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                if (getBlock(x, y, z) == 0) continue; // Skip air blocks
                
                for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
                    // Get the adjacent block position
                    int checkX = x + static_cast<int>(directions[faceIndex].x);
                    int checkY = y + static_cast<int>(directions[faceIndex].y);
                    int checkZ = z + static_cast<int>(directions[faceIndex].z);
                    
                    // Determine if face is at a chunk boundary
                    bool isChunkBoundary = checkX < 0 || checkX >= CHUNK_SIZE || 
                                         checkY < 0 || checkY >= CHUNK_HEIGHT || 
                                         checkZ < 0 || checkZ >= CHUNK_SIZE;
                    
                    // Only render faces adjacent to transparent blocks or at chunk boundaries
                    // This is the key change - we ONLY render a face if the adjacent block is transparent
                    if (isChunkBoundary) {
                        // For chunk boundaries, check neighboring chunks
                        if (m_world) {
                            // Convert to world coordinates
                            int worldX = m_x * CHUNK_SIZE + checkX;
                            int worldY = m_y * CHUNK_HEIGHT + checkY;
                            int worldZ = m_z * CHUNK_SIZE + checkZ;
                            
                            // Query the world for the block at this position
                            int adjacentBlock = m_world->getBlock(glm::ivec3(worldX, worldY, worldZ));
                            
                            // Get current block type
                            int currentBlock = getBlock(x, y, z);
                            
                            // Only render the face if:
                            // 1. The adjacent block is transparent, OR
                            // 2. The adjacent block is different from the current block (but only for non-transparent blocks)
                            if (isBlockTransparent(adjacentBlock) || 
                                (currentBlock != adjacentBlock && !isBlockTransparent(currentBlock))) {
                                needsRender[x][y][z][faceIndex] = true;
                                faceCount++;
                            }
                        } else {
                            // No world reference, treat as air
                            needsRender[x][y][z][faceIndex] = true;
                            faceCount++;
                        }
                    } else {
                        // Within chunk, check directly
                        int adjacentBlock = m_blocks[checkX][checkY][checkZ];
                        int currentBlock = getBlock(x, y, z);
                        
                        // Only render the face if:
                        // 1. The adjacent block is transparent, OR
                        // 2. The adjacent block is different from the current block (but only for non-transparent blocks)
                        if (isBlockTransparent(adjacentBlock) || 
                            (currentBlock != adjacentBlock && !isBlockTransparent(currentBlock))) {
                            needsRender[x][y][z][faceIndex] = true;
                            faceCount++;
                        }
                    }
                }
            }
        }
    }
    
    std::cout << "Found " << faceCount << " faces to render in chunk (" << m_x << ", " << m_y << ", " << m_z << ")" << std::endl;
    
    // Vertex data for each face type (scaled to unit size)
    std::vector<std::vector<float>> faceVertices = {
        // Front face (+Z) - Purple
        {
            0.0f, 0.0f, 1.0f,  0.667f, 0.5f,    // Bottom-left
            1.0f, 0.0f, 1.0f,  1.0f, 0.5f,      // Bottom-right
            1.0f, 1.0f, 1.0f,  1.0f, 1.0f,      // Top-right
            0.0f, 1.0f, 1.0f,  0.667f, 1.0f     // Top-left
        },
        
        // Back face (-Z) - Yellow
        {
            1.0f, 0.0f, 0.0f,  1.0f, 0.0f,      // Bottom-right
            0.0f, 0.0f, 0.0f,  0.667f, 0.0f,    // Bottom-left
            0.0f, 1.0f, 0.0f,  0.667f, 0.5f,    // Top-left
            1.0f, 1.0f, 0.0f,  1.0f, 0.5f       // Top-right
        },
        
        // Left face (-X) - Green
        {
            0.0f, 0.0f, 0.0f,  0.333f, 0.0f,    // Bottom-back
            0.0f, 0.0f, 1.0f,  0.667f, 0.0f,    // Bottom-front
            0.0f, 1.0f, 1.0f,  0.667f, 0.5f,    // Top-front
            0.0f, 1.0f, 0.0f,  0.333f, 0.5f     // Top-back
        },
        
        // Right face (+X) - Black
        {
            1.0f, 0.0f, 1.0f,  0.333f, 0.5f,    // Bottom-front
            1.0f, 0.0f, 0.0f,  0.0f, 0.5f,      // Bottom-back
            1.0f, 1.0f, 0.0f,  0.0f, 1.0f,      // Top-back
            1.0f, 1.0f, 1.0f,  0.333f, 1.0f     // Top-front
        },
        
        // Top face (+Y) - Red
        {
            0.0f, 1.0f, 1.0f,  0.333f, 0.5f,    // Front-left
            1.0f, 1.0f, 1.0f,  0.667f, 0.5f,    // Front-right
            1.0f, 1.0f, 0.0f,  0.667f, 1.0f,    // Back-right
            0.0f, 1.0f, 0.0f,  0.333f, 1.0f     // Back-left
        },
        
        // Bottom face (-Y) - White
        {
            0.0f, 0.0f, 0.0f,  0.0f, 0.0f,    // Bottom-left back
            1.0f, 0.0f, 0.0f,  0.333f, 0.0f,  // Bottom-right back
            1.0f, 0.0f, 1.0f,  0.333f, 0.5f,  // Top-right front
            0.0f, 0.0f, 1.0f,  0.0f, 0.5f     // Top-left front
        }
    };

    // STEP 2: MESH GENERATION - Now that we know all visible faces, apply appropriate meshing strategy
    if (disableGreedyMeshing) {
        // Simple per-block face generation - no merging, just add each visible face
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    if (getBlock(x, y, z) == 0) continue; // Skip air blocks
                    
                    for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
                        if (needsRender[x][y][z][faceIndex]) {
                            // Add simple face for this block
                            addFace(faceVertices[faceIndex], glm::vec3(x, y, z), directions[faceIndex]);
                        }
                    }
                }
            }
        }
    } else {
        // Apply greedy meshing algorithm to the PRE-CULLED faces
        // Create a 3D visited array to track merged faces
        bool visited[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE][6] = {false};
        
        // Process each face direction separately
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
                // Skip slices at the edge of chunks for the appropriate direction
                // unless we're exactly on the boundary
                if ((normal.x < 0 && wPos == 0) ||
                    (normal.x > 0 && wPos == CHUNK_SIZE - 1) ||
                    (normal.y < 0 && wPos == 0) ||
                    (normal.y > 0 && wPos == CHUNK_HEIGHT - 1) ||
                    (normal.z < 0 && wPos == 0) ||
                    (normal.z > 0 && wPos == CHUNK_SIZE - 1)) {
                    // We're at a chunk boundary in the normal direction, so we handle this specially
                    // Continue processing this slice, but be aware it's at a boundary
                }
                
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
                bool sliceVisited[CHUNK_SIZE][CHUNK_HEIGHT] = {false};
                
                for (int uPos = 0; uPos < CHUNK_SIZE; uPos++) {
                    for (int vPos = 0; vPos < CHUNK_HEIGHT; vPos++) {
                        if (!mask[uPos][vPos] || sliceVisited[uPos][vPos]) {
                            continue;
                        }
                        
                        // Found an unvisited face, start greedy merging
                        int width = 1;
                        int height = 1;
                        int blockType = blockTypes[uPos][vPos];
                        
                        // Expand horizontally (in U direction) as far as possible
                        while (uPos + width < CHUNK_SIZE && 
                              mask[uPos + width][vPos] && 
                              !sliceVisited[uPos + width][vPos] &&
                              blockTypes[uPos + width][vPos] == blockType) {
                            width++;
                        }
                        
                        // Expand vertically (in V direction) as far as possible
                        bool canExpandVertical = true;
                        while (canExpandVertical && vPos + height < CHUNK_HEIGHT) {
                            // Check if the entire row can be expanded
                            for (int u = 0; u < width; u++) {
                                if (!mask[uPos + u][vPos + height] || 
                                    sliceVisited[uPos + u][vPos + height] ||
                                    blockTypes[uPos + u][vPos + height] != blockType) {
                                    canExpandVertical = false;
                                    break;
                                }
                            }
                            
                            if (canExpandVertical) {
                                height++;
                            }
                        }
                        
                        // Mark all merged faces as visited
                        for (int v = 0; v < height; v++) {
                            for (int u = 0; u < width; u++) {
                                sliceVisited[uPos + u][vPos + v] = true;
                                
                                // Convert back to xyz to mark in the main visited array
                                int x = (u == 0) ? (uPos + u) : ((w == 0) ? wPos : (vPos + v));
                                int y = (u == 1) ? (uPos + u) : ((w == 1) ? wPos : (vPos + v));
                                int z = (u == 2) ? (uPos + u) : ((w == 2) ? wPos : (vPos + v));
                                visited[x][y][z][faceIndex] = true;
                            }
                        }
                        
                        // Add the greedy-meshed face to the mesh
                        addGreedyFace(faceVertices[faceIndex], normal, uPos, vPos, wPos, width, height, u, v, w);
                    }
                }
            }
        }
    }

    m_isDirty = false;
}

// Helper function to calculate the normal for a face based on vertex positions
glm::vec3 Chunk::calculateFaceNormal(const std::vector<float>& vertices, int startIndex) {
    // Get three of the vertices to calculate cross product
    glm::vec3 v1(vertices[startIndex], vertices[startIndex + 1], vertices[startIndex + 2]);
    glm::vec3 v2(vertices[startIndex + 5], vertices[startIndex + 6], vertices[startIndex + 7]);
    glm::vec3 v3(vertices[startIndex + 10], vertices[startIndex + 11], vertices[startIndex + 12]);
    
    // Calculate vectors along two edges
    glm::vec3 edge1 = v2 - v1;
    glm::vec3 edge2 = v3 - v1;
    
    // Calculate normal with cross product
    glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
    
    return normal;
}

// Helper method to add a greedy-meshed face
void Chunk::addGreedyFace(const std::vector<float>& faceTemplate, const glm::ivec3& normal, 
                          int uStart, int vStart, int wPos, int width, int height, 
                          int uAxis, int vAxis, int wAxis) {
    unsigned int indexOffset = m_meshVertices.size() / 5; // 5 floats per vertex (3 pos + 2 UV)
    
    // Calculate the world coordinates of the face corners
    glm::vec3 pos;
    pos[wAxis] = wPos;
    
    // Add a slight offset in the normal direction to prevent z-fighting at chunk boundaries
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
        vertPos[wAxis] = pos[wAxis] + (normal[wAxis] > 0 ? 1.0f : 0.0f); // Position face properly based on normal
        
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
    
    // Correct winding order for exterior faces
    // First triangle: Bottom-left -> Top-right -> Bottom-right
    m_meshIndices.push_back(indexOffset);     // Bottom-left
    m_meshIndices.push_back(indexOffset + 2); // Top-right
    m_meshIndices.push_back(indexOffset + 1); // Bottom-right
    
    // Second triangle: Bottom-left -> Top-left -> Top-right
    m_meshIndices.push_back(indexOffset);     // Bottom-left
    m_meshIndices.push_back(indexOffset + 3); // Top-left
    m_meshIndices.push_back(indexOffset + 2); // Top-right
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

    // Create a modified position that accounts for face orientation
    glm::vec3 modPosition = position;
    
    // For positive facing normals, we need to offset the face by 1.0
    // This ensures consistency with how addGreedyFace positions vertices
    if (normal.x > 0) modPosition.x += 1.0f;
    if (normal.y > 0) modPosition.y += 1.0f;
    if (normal.z > 0) modPosition.z += 1.0f;

    // Add vertices
    for (size_t i = 0; i < vertices.size(); i += 5) {
        // Position
        m_meshVertices.push_back(vertices[i] + modPosition.x);
        m_meshVertices.push_back(vertices[i + 1] + modPosition.y);
        m_meshVertices.push_back(vertices[i + 2] + modPosition.z);
        // UV coordinates
        m_meshVertices.push_back(vertices[i + 3]);
        m_meshVertices.push_back(vertices[i + 4]);
    }

    // Correct winding order for exterior faces (match addGreedyFace)
    // First triangle: Bottom-left -> Top-right -> Bottom-right
    m_meshIndices.push_back(indexOffset);     // Bottom-left
    m_meshIndices.push_back(indexOffset + 2); // Top-right
    m_meshIndices.push_back(indexOffset + 1); // Bottom-right
    
    // Second triangle: Bottom-left -> Top-left -> Top-right
    m_meshIndices.push_back(indexOffset);     // Bottom-left
    m_meshIndices.push_back(indexOffset + 3); // Top-left
    m_meshIndices.push_back(indexOffset + 2); // Top-right
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

bool Chunk::isBlockTransparent(int blockType) const {
    // Block ID 0 is air, which is transparent
    if (blockType == 0) return true;
    
    // Define other transparent block types here
    // For example, if block ID 2 is glass, 3 is water, etc.
    switch (blockType) {
        case 0:  // Air
            return true;
        case 2:  // Assuming ID 2 is glass
            return true;
        case 3:  // Assuming ID 3 is water
            return true;
        case 4:  // Assuming ID 4 is leaves
            return true;
        default:
            return false;
    }
} 