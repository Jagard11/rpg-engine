// src/voxel/voxel_world.cpp
#include "../../include/voxel/voxel_world.h"
#include <QDebug>
#include <QDateTime>
#include <QDir>

VoxelWorld::VoxelWorld(QObject* parent) : QObject(parent) {
    // Initialize with an empty world
    // Set texture paths
    m_texturePaths[VoxelType::Cobblestone] = QDir::currentPath() + "/resources/cobblestone.png";
    m_texturePaths[VoxelType::Grass] = QDir::currentPath() + "/resources/grass.png";
    m_texturePaths[VoxelType::Dirt] = QDir::currentPath() + "/resources/dirt.png";
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
            // Create a copy of the voxel with the texture path if needed
            Voxel finalVoxel = voxel;
            if (finalVoxel.texturePath.isEmpty() && m_texturePaths.contains(finalVoxel.type)) {
                finalVoxel.texturePath = m_texturePaths[finalVoxel.type];
            }
            m_voxels[pos] = finalVoxel;
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
    Voxel grass(VoxelType::Grass, QColor(34, 139, 34), m_texturePaths[VoxelType::Grass]); // Forest green
    
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
    
    // Create floor with grass and dirt
    Voxel grassVoxel(VoxelType::Grass, QColor(34, 139, 34), m_texturePaths[VoxelType::Grass]);
    Voxel dirtVoxel(VoxelType::Dirt, QColor(160, 82, 45), m_texturePaths[VoxelType::Dirt]);
    
    // Create grass in the center (2/3 of the floor)
    int grassWidth = (width * 2) / 3;
    int grassLength = (length * 2) / 3;
    int grassHalfWidth = grassWidth / 2;
    int grassHalfLength = grassLength / 2;
    
    // Generate the floor with grass in center and dirt on perimeter
    for (int x = -halfWidth; x < halfWidth; x++) {
        for (int z = -halfLength; z < halfLength; z++) {
            // Check if this is in the center grass area
            if (x >= -grassHalfWidth && x < grassHalfWidth &&
                z >= -grassHalfLength && z < grassHalfLength) {
                setVoxel(x, 0, z, grassVoxel);
            } else {
                setVoxel(x, 0, z, dirtVoxel);
            }
        }
    }
    
    // Create walls with cobblestone
    Voxel wallVoxel(VoxelType::Cobblestone, QColor(192, 192, 192), m_texturePaths[VoxelType::Cobblestone]);
    
    // North wall (positive Z)
    generateWall(-halfWidth, halfLength-1, halfWidth-1, halfLength-1, 1, height, wallVoxel);
    
    // South wall (negative Z)
    generateWall(-halfWidth, -halfLength, halfWidth-1, -halfLength, 1, height, wallVoxel);
    
    // East wall (positive X)
    generateWall(halfWidth-1, -halfLength, halfWidth-1, halfLength-1, 1, height, wallVoxel);
    
    // West wall (negative X)
    generateWall(-halfWidth, -halfLength, -halfWidth, halfLength-1, 1, height, wallVoxel);
    
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