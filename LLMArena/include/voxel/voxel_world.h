// include/voxel/voxel_world.h
#ifndef VOXEL_WORLD_H
#define VOXEL_WORLD_H

#include "voxel_types.h"
#include <QObject>
#include <QHash>
#include <QVector>

// Class to manage the voxel world
class VoxelWorld : public QObject {
    Q_OBJECT
    
public:
    explicit VoxelWorld(QObject* parent = nullptr);
    
    // Voxel access
    Voxel getVoxel(int x, int y, int z) const;
    Voxel getVoxel(const VoxelPos& pos) const;
    void setVoxel(int x, int y, int z, const Voxel& voxel);
    void setVoxel(const VoxelPos& pos, const Voxel& voxel);
    
    // World generation
    void createFlatWorld();
    void createRoomWithWalls(int width, int length, int height);
    
    // Visibility check for rendering optimization
    bool isVoxelVisible(const VoxelPos& pos) const;
    
    // Get all visible voxels for rendering
    QVector<VoxelPos> getVisibleVoxels() const;
    
signals:
    void worldChanged();
    
private:
    // Store voxels in a sparse data structure
    QHash<VoxelPos, Voxel> m_voxels;
    
    // Check if a voxel is visible (has at least one empty neighbor)
    bool hasEmptyNeighbor(const VoxelPos& pos) const;
    
    // Helper function to generate floor and walls
    void generateFloor(int y, int width, int length, const Voxel& voxel);
    void generateWall(int x1, int z1, int x2, int z2, int y1, int y2, const Voxel& voxel);
};

#endif // VOXEL_WORLD_H