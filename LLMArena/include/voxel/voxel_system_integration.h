// include/voxel/voxel_system_integration.h
#ifndef VOXEL_SYSTEM_INTEGRATION_H
#define VOXEL_SYSTEM_INTEGRATION_H

#include "voxel_world.h"
#include "voxel_renderer.h"
#include "sky_system.h"
#include "../game/game_scene.h"

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
    VoxelWorld* getWorld() const { return m_world; }
    
    // Create default world
    void createDefaultWorld();
    
public slots:
    // Update game scene to match voxel world
    void updateGameScene();
    
signals:
    void worldChanged();
    
private:
    // Components
    VoxelWorld* m_world;
    VoxelRenderer* m_renderer;
    SkySystem* m_sky;
    GameScene* m_gameScene;
    
    // Connect signals
    void connectSignals();
};

#endif // VOXEL_SYSTEM_INTEGRATION_H