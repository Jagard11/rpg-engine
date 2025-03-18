// src/voxel/voxel_world.cpp
#include "../../include/voxel/voxel_world.h"
#include <QDebug>
#include <QDateTime>

VoxelWorld::VoxelWorld(QObject* parent) : QObject(parent) {
    // Initialize with an empty world
}

Voxel VoxelWorld::getVoxel(int x, int y, int z) const {
    return getVoxel(VoxelPos(x, y, z));
}

Voxel VoxelWorld::getVoxel(const VoxelPos& pos) const {
    return m_voxels.contains(pos) ? m_voxels[pos] : Voxel();
}

void VoxelWorld::setVoxel(int x, int y, int z, const Voxel& voxel) {
    setVoxel(VoxelPos(x, y, z), voxel);
}

void VoxelWorld::setVoxel(const VoxelPos& pos, const Voxel& voxel) {
    // Check if voxel actually changed to prevent unnecessary updates
    bool changed = false;
    
    if (voxel.type == VoxelType::Air) {
        if (m_voxels.contains(pos)) {
            m_voxels.remove(pos); // Don't store air blocks
            changed = true;
        }
    } else {
        if (!m_voxels.contains(pos) || m_voxels[pos].type != voxel.type || m_voxels[pos].color != voxel.color) {
            m_voxels[pos] = voxel;
            changed = true;
        }
    }
    
    // Signal that the world has changed only if there was an actual change
    if (changed) {
        emit worldChanged();
    }
}

void VoxelWorld::createFlatWorld() {
    // Clear existing voxels
    m_voxels.clear();
    
    // Create a flat terrain at y=0 with some color
    Voxel grass(VoxelType::Solid, QColor(34, 139, 34)); // Forest green
    
    // Generate 50x50 flat world
    for (int x = -25; x < 25; x++) {
        for (int z = -25; z < 25; z++) {
            setVoxel(x, 0, z, grass);
        }
    }
    
    emit worldChanged();
}

void VoxelWorld::createRoomWithWalls(int width, int length, int height) {
    // Clear existing voxels
    m_voxels.clear();
    
    // Calculate room boundaries
    int halfWidth = width / 2;
    int halfLength = length / 2;
    
    // Create floor
    Voxel floorVoxel(VoxelType::Solid, QColor(160, 82, 45)); // Brown
    generateFloor(0, width, length, floorVoxel);
    
    // Create walls
    Voxel wallVoxel(VoxelType::Solid, QColor(192, 192, 192)); // Light gray
    
    // North wall (positive Z)
    generateWall(-halfWidth, halfLength, halfWidth, halfLength, 1, height, wallVoxel);
    
    // South wall (negative Z)
    generateWall(-halfWidth, -halfLength, halfWidth, -halfLength, 1, height, wallVoxel);
    
    // East wall (positive X)
    generateWall(halfWidth, -halfLength, halfWidth, halfLength, 1, height, wallVoxel);
    
    // West wall (negative X)
    generateWall(-halfWidth, -halfLength, -halfWidth, halfLength, 1, height, wallVoxel);
    
    emit worldChanged();
}

void VoxelWorld::generateFloor(int y, int width, int length, const Voxel& voxel) {
    int halfWidth = width / 2;
    int halfLength = length / 2;
    
    for (int x = -halfWidth; x < halfWidth; x++) {
        for (int z = -halfLength; z < halfLength; z++) {
            setVoxel(x, y, z, voxel);
        }
    }
}

void VoxelWorld::generateWall(int x1, int z1, int x2, int z2, int y1, int y2, const Voxel& voxel) {
    // Determine direction and length
    int dx = (x2 > x1) ? 1 : (x2 < x1) ? -1 : 0;
    int dz = (z2 > z1) ? 1 : (z2 < z1) ? -1 : 0;
    
    int steps = 0;
    if (dx != 0) {
        steps = abs(x2 - x1);
    } else {
        steps = abs(z2 - z1);
    }
    
    // Generate voxels along the wall
    for (int i = 0; i <= steps; i++) {
        int x = x1 + dx * i;
        int z = z1 + dz * i;
        
        // Generate voxels vertically for this wall segment
        for (int y = y1; y < y2; y++) {
            setVoxel(x, y, z, voxel);
        }
    }
}

bool VoxelWorld::isVoxelVisible(const VoxelPos& pos) const {
    return hasEmptyNeighbor(pos);
}

bool VoxelWorld::hasEmptyNeighbor(const VoxelPos& pos) const {
    // Check all 6 neighbors
    static const VoxelPos neighbors[] = {
        VoxelPos(1, 0, 0), VoxelPos(-1, 0, 0),
        VoxelPos(0, 1, 0), VoxelPos(0, -1, 0),
        VoxelPos(0, 0, 1), VoxelPos(0, 0, -1)
    };
    
    for (int i = 0; i < 6; i++) {
        VoxelPos neighborPos(pos.x + neighbors[i].x, pos.y + neighbors[i].y, pos.z + neighbors[i].z);
        if (!m_voxels.contains(neighborPos) || m_voxels[neighborPos].type == VoxelType::Air) {
            return true;
        }
    }
    
    return false;
}

QVector<VoxelPos> VoxelWorld::getVisibleVoxels() const {
    QVector<VoxelPos> visibleVoxels;
    
    // Iterate through all voxels and check visibility
    for (auto it = m_voxels.constBegin(); it != m_voxels.constEnd(); ++it) {
        if (isVoxelVisible(it.key())) {
            visibleVoxels.append(it.key());
        }
    }
    
    return visibleVoxels;
}