// src/arena/voxels/chunk/chunk.cpp
#include "../../../include/arena/voxels/chunk/chunk.h"
#include <QDebug>

Chunk::Chunk(const ChunkCoordinate& coordinate)
    : m_coordinate(coordinate), m_modified(false) {
    // Initialize with current time
    m_lastAccessTime = QDateTime::currentDateTime();
}

Voxel Chunk::getVoxel(int x, int y, int z) const {
    // Update last access time (mutable)
    const_cast<Chunk*>(this)->m_lastAccessTime = QDateTime::currentDateTime();
    
    // Pass through to octree
    return m_octree.getVoxel(x, y, z);
}

bool Chunk::setVoxel(int x, int y, int z, const Voxel& voxel) {
    // Update last access time
    m_lastAccessTime = QDateTime::currentDateTime();
    
    // Pass through to octree
    bool changed = m_octree.setVoxel(x, y, z, voxel);
    
    // Mark as modified if changed
    if (changed) {
        m_modified = true;
    }
    
    return changed;
}

bool Chunk::isVoxelVisible(int x, int y, int z) const {
    // Update last access time (mutable)
    const_cast<Chunk*>(this)->m_lastAccessTime = QDateTime::currentDateTime();
    
    // Pass through to octree
    return m_octree.isVoxelVisible(x, y, z);
}

std::vector<VoxelPos> Chunk::getVisibleVoxels() const {
    // Update last access time (mutable)
    const_cast<Chunk*>(this)->m_lastAccessTime = QDateTime::currentDateTime();
    
    // Pass through to octree
    return m_octree.getVisibleVoxels();
}

size_t Chunk::calculateMemoryUsage() const {
    // Start with size of this object
    size_t memoryUsage = sizeof(Chunk);
    
    // Add octree memory usage
    memoryUsage += m_octree.calculateMemoryUsage();
    
    return memoryUsage;
}

bool Chunk::isEmpty() const {
    // Update last access time (mutable)
    const_cast<Chunk*>(this)->m_lastAccessTime = QDateTime::currentDateTime();
    
    // Pass through to octree
    return m_octree.isEmpty();
}

void Chunk::optimize() {
    // Pass through to octree
    m_octree.optimize();
}

std::pair<ChunkCoordinate, bool> Chunk::setVoxelExtended(int x, int y, int z, const Voxel& voxel) {
    // Check if coordinates are within this chunk
    if (x >= 0 && x < ChunkCoordinate::CHUNK_SIZE && 
        y >= 0 && y < ChunkCoordinate::CHUNK_SIZE && 
        z >= 0 && z < ChunkCoordinate::CHUNK_SIZE) {
        // Set voxel in this chunk
        bool changed = setVoxel(x, y, z, voxel);
        return {m_coordinate, changed};
    }
    
    // Calculate which neighboring chunk this voxel belongs to
    int chunkOffsetX = 0;
    int chunkOffsetY = 0;
    int chunkOffsetZ = 0;
    
    // Compute the chunk offset based on local coordinates
    if (x < 0) {
        chunkOffsetX = -1;
        x += ChunkCoordinate::CHUNK_SIZE;
    } else if (x >= ChunkCoordinate::CHUNK_SIZE) {
        chunkOffsetX = 1;
        x -= ChunkCoordinate::CHUNK_SIZE;
    }
    
    if (y < 0) {
        chunkOffsetY = -1;
        y += ChunkCoordinate::CHUNK_SIZE;
    } else if (y >= ChunkCoordinate::CHUNK_SIZE) {
        chunkOffsetY = 1;
        y -= ChunkCoordinate::CHUNK_SIZE;
    }
    
    if (z < 0) {
        chunkOffsetZ = -1;
        z += ChunkCoordinate::CHUNK_SIZE;
    } else if (z >= ChunkCoordinate::CHUNK_SIZE) {
        chunkOffsetZ = 1;
        z -= ChunkCoordinate::CHUNK_SIZE;
    }
    
    // Calculate the coordinate of the neighboring chunk
    ChunkCoordinate neighborCoord = m_coordinate.offset(chunkOffsetX, chunkOffsetY, chunkOffsetZ);
    
    // Return the neighbor chunk coordinate and flag that it needs an update
    return {neighborCoord, true};
}