// include/arena/voxels/voxel_system_integration.h
#ifndef VOXEL_SYSTEM_INTEGRATION_H
#define VOXEL_SYSTEM_INTEGRATION_H

#include "voxel_world.h"
#include "voxel_renderer.h"
#include "../skybox/skybox_core.h"
#include "../core/arena_core.h"
#include "../ui/voxel_highlight_renderer.h"
#include "world/voxel_world_system.h"

#include <QObject>
#include <QOpenGLFunctions>

// Class to integrate voxel system with existing game
class VoxelSystemIntegration : public QObject, protected QOpenGLFunctions {
    Q_OBJECT
    
public:
    explicit VoxelSystemIntegration(GameScene* gameScene, QObject* parent = nullptr);
    ~VoxelSystemIntegration();
    
    // Initialize resources
    void initialize();
    
    // Render voxel world and sky
    void render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix);
    
    // Get world for direct access
    VoxelWorld* getWorld() const;
    
    // Get world system for advanced operations
    VoxelWorldSystem* getWorldSystem() const { return m_worldSystem.get(); }
    
    // Create default world
    void createDefaultWorld();
    
    // Create spherical planet
    void createSphericalPlanet(float radius, float terrainHeight, unsigned int seed);
    
    // Set voxel highlight
    void setVoxelHighlight(const VoxelPos& pos, int face);
    
    // Get highlight information
    VoxelPos getHighlightedVoxelPos() const;
    int getHighlightedVoxelFace() const;
    
    // Raycast from a point in a direction
    bool raycast(const QVector3D& origin, const QVector3D& direction, float maxDistance,
                QVector3D& outHitPos, QVector3D& outHitNormal, Voxel& outVoxel);
    
    // Place voxel at the raycast hit position (offset by normal)
    bool placeVoxel(const QVector3D& hitPos, const QVector3D& normal, const Voxel& voxel);
    
    // Remove voxel at the raycast hit position
    bool removeVoxel(const QVector3D& hitPos);
    
    // Get the surface height at a specific XZ coordinate
    float getSurfaceHeightAt(float x, float z) const;
    
public slots:
    // Update game scene to match voxel world
    void updateGameScene();
    
    // Stream chunks around player position
    void streamChunksAroundPlayer(const QVector3D& playerPosition);
    
signals:
    void worldChanged();
    void chunkLoaded(const ChunkCoordinate& coordinate);
    void chunkUnloaded(const ChunkCoordinate& coordinate);
    
private:
    // Components
    std::unique_ptr<VoxelWorldSystem> m_worldSystem;
    VoxelWorld* m_world;  // Owned by current architecture, will be replaced
    VoxelRenderer* m_renderer;
    SkySystem* m_sky;
    GameScene* m_gameScene;
    VoxelHighlightRenderer* m_highlightRenderer;
    
    // Voxel highlight data
    QVector3D m_highlightedVoxelPos;
    int m_highlightedVoxelFace;
    
    // Connect signals
    void connectSignals();
};

#endif // VOXEL_SYSTEM_INTEGRATION_H