// include/arena/voxels/voxel_world.h
#ifndef VOXEL_WORLD_H
#define VOXEL_WORLD_H

#include "types/voxel_types.h"
#include "../system/performance_settings.h"
#include <QObject>
#include <QHash>
#include <QVector>
#include <QMap>

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
    
    // Get all voxels (not just visible ones)
    const QHash<VoxelPos, Voxel>& getAllVoxels() const { return m_voxels; }
    
signals:
    void worldChanged();
    
private:
    // Store voxels in a sparse data structure
    QHash<VoxelPos, Voxel> m_voxels;
    
    // Map of texture paths for each voxel type
    QMap<VoxelType, QString> m_texturePaths;
    
    // Performance settings reference
    PerformanceSettings* m_perfSettings;
    
    // Check if a voxel is visible (has at least one empty neighbor)
    bool hasEmptyNeighbor(const VoxelPos& pos) const;
    
    // Helper function to generate floor and walls
    void generateFloor(int y, int width, int length, const Voxel& voxel);
    void generateWall(int x1, int z1, int x2, int z2, int y1, int y2, const Voxel& voxel);
};

#endif // VOXEL_WORLD_H