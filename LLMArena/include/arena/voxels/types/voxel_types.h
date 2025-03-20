// include/voxel/voxel_types.h
#ifndef VOXEL_TYPES_H
#define VOXEL_TYPES_H

#include <QVector3D>
#include <QColor>
#include <QString>
#include <cmath> // Include cmath for floor function

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
    
    // Convert to vector3D
    QVector3D toVector3D() const {
        return QVector3D(x, y, z);
    }
    
    // Check if this is a valid position
    bool isValid() const {
        // Define sensible limits for voxel positions
        const int MAX_COORD = 1000;
        return (x >= -MAX_COORD && x <= MAX_COORD &&
                y >= -MAX_COORD && y <= MAX_COORD &&
                z >= -MAX_COORD && z <= MAX_COORD);
    }
    
    // Create from QVector3D
    static VoxelPos fromVector3D(const QVector3D& vec) {
        return VoxelPos(floor(vec.x()), floor(vec.y()), floor(vec.z()));
    }
};

// Hash function for VoxelPos for use in QHash
inline uint qHash(const VoxelPos& pos, uint seed = 0) {
    return qHash(QString("%1,%2,%3").arg(pos.x).arg(pos.y).arg(pos.z), seed);
}

#endif // VOXEL_TYPES_H