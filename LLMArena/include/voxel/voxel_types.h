// include/voxel/voxel_types.h
#ifndef VOXEL_TYPES_H
#define VOXEL_TYPES_H

#include <QVector3D>
#include <QColor>
#include <QString>

// Expanded enum for voxel types
enum class VoxelType {
    Air,         // Empty space
    Solid,       // Generic solid block
    Cobblestone, // Cobblestone block
    Grass,       // Grass block
    Dirt         // Dirt block
};

// Structure to represent a single voxel
struct Voxel {
    VoxelType type;
    QColor color;
    QString texturePath; // Path to texture file
    
    Voxel() : type(VoxelType::Air), color(Qt::transparent), texturePath("") {}
    Voxel(VoxelType t, const QColor& c, const QString& tex = "") : type(t), color(c), texturePath(tex) {}
};

// Position in the voxel grid
struct VoxelPos {
    int x, y, z;
    
    VoxelPos() : x(0), y(0), z(0) {}
    VoxelPos(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {}
    
    bool operator==(const VoxelPos& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
    
    // Convert to world coordinates (for rendering)
    QVector3D toWorldPos() const {
        return QVector3D(x, y, z);
    }
};

// Hash function for VoxelPos for use in QHash
inline uint qHash(const VoxelPos& pos, uint seed = 0) {
    return qHash(QString("%1,%2,%3").arg(pos.x).arg(pos.y).arg(pos.z), seed);
}

#endif // VOXEL_TYPES_H