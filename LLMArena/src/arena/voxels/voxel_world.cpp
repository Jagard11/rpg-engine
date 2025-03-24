// src/arena/voxels/voxel_world.cpp
#include "../../../include/arena/voxels/voxel_world.h"
#include <QDebug>

VoxelWorld::VoxelWorld(QObject* parent) : QObject(parent) {
    // Initialize texture paths for voxel types
    m_texturePaths[VoxelType::Dirt] = ":/resources/dirt.png";
    m_texturePaths[VoxelType::Grass] = ":/resources/grass.png";
    m_texturePaths[VoxelType::Cobblestone] = ":/resources/cobblestone.png";
}

Voxel VoxelWorld::getVoxel(int x, int y, int z) const {
    return getVoxel(VoxelPos(x, y, z));
}

Voxel VoxelWorld::getVoxel(const VoxelPos& pos) const {
    // Check if the voxel exists in our storage
    if (m_voxels.contains(pos)) {
        return m_voxels[pos];
    }
    
    // Return air (empty) voxel if not found
    Voxel emptyVoxel;
    emptyVoxel.type = VoxelType::Air;
    emptyVoxel.color = Qt::transparent;
    return emptyVoxel;
}

void VoxelWorld::setVoxel(int x, int y, int z, const Voxel& voxel) {
    setVoxel(VoxelPos(x, y, z), voxel);
}

void VoxelWorld::setVoxel(const VoxelPos& pos, const Voxel& voxel) {
    // Validate position
    if (!pos.isValid()) {
        qWarning() << "Attempted to set voxel at invalid position:" << pos.x << pos.y << pos.z;
        return;
    }
    
    // Check if we're setting an air block (removing a voxel)
    if (voxel.type == VoxelType::Air) {
        // Remove the voxel if it exists
        if (m_voxels.contains(pos)) {
            m_voxels.remove(pos);
        }
    } else {
        // Store the voxel
        m_voxels[pos] = voxel;
        
        // Set texture path if not already set
        if (voxel.texturePath.isEmpty() && m_texturePaths.contains(voxel.type)) {
            m_voxels[pos].texturePath = m_texturePaths[voxel.type];
        }
    }
    
    // Notify that the world has changed
    emit worldChanged();
}

void VoxelWorld::createFlatWorld() {
    // Clear existing voxels
    m_voxels.clear();
    
    // Create a flat terrain at y=0
    const int WORLD_SIZE = 16;
    const int HALF_SIZE = WORLD_SIZE / 2;
    
    // Create dirt foundation
    Voxel dirtVoxel(VoxelType::Dirt, QColor(139, 69, 19));
    dirtVoxel.texturePath = m_texturePaths[VoxelType::Dirt];
    
    // Create grass top layer
    Voxel grassVoxel(VoxelType::Grass, QColor(34, 139, 34));
    grassVoxel.texturePath = m_texturePaths[VoxelType::Grass];
    
    // Create grid of voxels
    for (int x = -HALF_SIZE; x < HALF_SIZE; x++) {
        for (int z = -HALF_SIZE; z < HALF_SIZE; z++) {
            // Set dirt for bottom layer
            setVoxel(x, -1, z, dirtVoxel);
            
            // Set grass for top layer
            setVoxel(x, 0, z, grassVoxel);
        }
    }
    
    emit worldChanged();
}

void VoxelWorld::createRoomWithWalls(int width, int length, int height) {
    // Ensure reasonable dimensions
    width = qMax(4, qMin(width, 128));
    length = qMax(4, qMin(length, 128));
    height = qMax(2, qMin(height, 64));
    
    // Calculate center offset
    int offsetX = -width / 2;
    int offsetZ = -length / 2;
    
    // Clear existing voxels
    m_voxels.clear();
    
    // Create voxel types with proper textures
    Voxel floorVoxel(VoxelType::Cobblestone, QColor(128, 128, 128));
    floorVoxel.texturePath = m_texturePaths[VoxelType::Cobblestone];
    
    Voxel wallVoxel(VoxelType::Cobblestone, QColor(100, 100, 100));
    wallVoxel.texturePath = m_texturePaths[VoxelType::Cobblestone];
    
    // Create floor
    generateFloor(0, width, length, floorVoxel);
    
    // Create walls
    // South wall (negative Z)
    generateWall(offsetX, offsetZ, offsetX + width, offsetZ, 1, height, wallVoxel);
    
    // North wall (positive Z)
    generateWall(offsetX, offsetZ + length, offsetX + width, offsetZ + length, 1, height, wallVoxel);
    
    // West wall (negative X)
    generateWall(offsetX, offsetZ, offsetX, offsetZ + length, 1, height, wallVoxel);
    
    // East wall (positive X)
    generateWall(offsetX + width, offsetZ, offsetX + width, offsetZ + length, 1, height, wallVoxel);
    
    // Notify that world has changed
    emit worldChanged();
}

bool VoxelWorld::isVoxelVisible(const VoxelPos& pos) const {
    // A voxel is visible if it's not air and at least one of its six neighbors is air
    Voxel voxel = getVoxel(pos);
    
    // Air voxels are never visible (they're empty space)
    if (voxel.type == VoxelType::Air) {
        return false;
    }
    
    // Check if the voxel has at least one empty neighbor
    return hasEmptyNeighbor(pos);
}

QVector<VoxelPos> VoxelWorld::getVisibleVoxels() const {
    QVector<VoxelPos> visibleVoxels;
    
    // Check each voxel in our storage
    for (auto it = m_voxels.constBegin(); it != m_voxels.constEnd(); ++it) {
        const VoxelPos& pos = it.key();
        
        // Add to the list if the voxel is visible (has at least one empty neighbor)
        if (isVoxelVisible(pos)) {
            visibleVoxels.append(pos);
        }
    }
    
    return visibleVoxels;
}

bool VoxelWorld::hasEmptyNeighbor(const VoxelPos& pos) const {
    // Check all six neighbor positions
    const VoxelPos neighbors[] = {
        VoxelPos(pos.x + 1, pos.y, pos.z), // Right
        VoxelPos(pos.x - 1, pos.y, pos.z), // Left
        VoxelPos(pos.x, pos.y + 1, pos.z), // Top
        VoxelPos(pos.x, pos.y - 1, pos.z), // Bottom
        VoxelPos(pos.x, pos.y, pos.z + 1), // Front
        VoxelPos(pos.x, pos.y, pos.z - 1)  // Back
    };
    
    // Check if any neighbor is air (empty)
    for (int i = 0; i < 6; i++) {
        if (getVoxel(neighbors[i]).type == VoxelType::Air) {
            return true;
        }
    }
    
    return false;
}

void VoxelWorld::generateFloor(int y, int width, int length, const Voxel& voxel) {
    int offsetX = -width / 2;
    int offsetZ = -length / 2;
    
    for (int x = 0; x < width; x++) {
        for (int z = 0; z < length; z++) {
            setVoxel(offsetX + x, y, offsetZ + z, voxel);
        }
    }
}

void VoxelWorld::generateWall(int x1, int z1, int x2, int z2, int y1, int y2, const Voxel& voxel) {
    // Ensure proper order of coordinates
    if (x1 > x2) std::swap(x1, x2);
    if (z1 > z2) std::swap(z1, z2);
    if (y1 > y2) std::swap(y1, y2);
    
    // Check for vertical wall
    if (x1 == x2) {
        // Generate voxels along z-axis
        for (int z = z1; z <= z2; z++) {
            for (int y = y1; y < y2; y++) {
                setVoxel(x1, y, z, voxel);
            }
        }
    }
    // Check for horizontal wall
    else if (z1 == z2) {
        // Generate voxels along x-axis
        for (int x = x1; x <= x2; x++) {
            for (int y = y1; y < y2; y++) {
                setVoxel(x, y, z1, voxel);
            }
        }
    }
    // Diagonal walls not supported in this simple implementation
    else {
        qWarning() << "Diagonal walls not supported";
    }
}