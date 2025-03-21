// src/arena/voxels/chunk/chunk_coordinate.cpp
#include "../../../include/arena/voxels/chunk/chunk_coordinate.h"
#include <cmath>

ChunkCoordinate ChunkCoordinate::fromWorldPosition(const QVector3D& worldPosition) {
    // Convert world position to chunk coordinates
    int chunkX = std::floor(worldPosition.x() / CHUNK_SIZE);
    int chunkY = std::floor(worldPosition.y() / CHUNK_SIZE);
    int chunkZ = std::floor(worldPosition.z() / CHUNK_SIZE);
    
    return ChunkCoordinate(chunkX, chunkY, chunkZ);
}

QVector3D ChunkCoordinate::toWorldPosition(int localX, int localY, int localZ) const {
    // Convert local coordinates to world coordinates
    float worldX = m_x * CHUNK_SIZE + localX;
    float worldY = m_y * CHUNK_SIZE + localY;
    float worldZ = m_z * CHUNK_SIZE + localZ;
    
    return QVector3D(worldX, worldY, worldZ);
}

QVector3D ChunkCoordinate::getMinCorner() const {
    // Get minimum corner in world coordinates
    return QVector3D(m_x * CHUNK_SIZE, m_y * CHUNK_SIZE, m_z * CHUNK_SIZE);
}

QVector3D ChunkCoordinate::getMaxCorner() const {
    // Get maximum corner in world coordinates
    return QVector3D((m_x + 1) * CHUNK_SIZE, (m_y + 1) * CHUNK_SIZE, (m_z + 1) * CHUNK_SIZE);
}

QVector3D ChunkCoordinate::getCenter() const {
    // Get center in world coordinates
    return QVector3D(
        m_x * CHUNK_SIZE + CHUNK_SIZE / 2.0f,
        m_y * CHUNK_SIZE + CHUNK_SIZE / 2.0f,
        m_z * CHUNK_SIZE + CHUNK_SIZE / 2.0f
    );
}

float ChunkCoordinate::distanceTo(const ChunkCoordinate& other) const {
    // Calculate Euclidean distance
    return std::sqrt(distanceSquaredTo(other));
}

float ChunkCoordinate::distanceSquaredTo(const ChunkCoordinate& other) const {
    // Calculate squared Euclidean distance
    float dx = m_x - other.m_x;
    float dy = m_y - other.m_y;
    float dz = m_z - other.m_z;
    
    return dx * dx + dy * dy + dz * dz;
}

int ChunkCoordinate::manhattanDistanceTo(const ChunkCoordinate& other) const {
    // Calculate Manhattan distance
    return std::abs(m_x - other.m_x) + std::abs(m_y - other.m_y) + std::abs(m_z - other.m_z);
}

bool ChunkCoordinate::isNeighbor(const ChunkCoordinate& other) const {
    // Check if chunks are neighbors (adjacent)
    int dx = std::abs(m_x - other.m_x);
    int dy = std::abs(m_y - other.m_y);
    int dz = std::abs(m_z - other.m_z);
    
    // Chunks are neighbors if they differ by at most 1 in each dimension
    // and they're not the same chunk
    return dx <= 1 && dy <= 1 && dz <= 1 && (dx > 0 || dy > 0 || dz > 0);
}

ChunkCoordinate ChunkCoordinate::offset(int dx, int dy, int dz) const {
    // Create a new coordinate offset by the given amount
    return ChunkCoordinate(m_x + dx, m_y + dy, m_z + dz);
}

std::vector<ChunkCoordinate> ChunkCoordinate::getAllNeighbors() const {
    std::vector<ChunkCoordinate> neighbors;
    neighbors.reserve(26); // 3³-1 = 26 neighbors
    
    // Loop through all 27 positions in a 3x3x3 cube
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                // Skip the center (this chunk)
                if (dx == 0 && dy == 0 && dz == 0) {
                    continue;
                }
                
                // Add the neighbor
                neighbors.push_back(offset(dx, dy, dz));
            }
        }
    }
    
    return neighbors;
}

std::vector<ChunkCoordinate> ChunkCoordinate::getFaceNeighbors() const {
    std::vector<ChunkCoordinate> neighbors;
    neighbors.reserve(6);
    
    // Add the 6 face-adjacent neighbors
    neighbors.push_back(offset( 1,  0,  0));
    neighbors.push_back(offset(-1,  0,  0));
    neighbors.push_back(offset( 0,  1,  0));
    neighbors.push_back(offset( 0, -1,  0));
    neighbors.push_back(offset( 0,  0,  1));
    neighbors.push_back(offset( 0,  0, -1));
    
    return neighbors;
}