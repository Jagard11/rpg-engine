// src/voxel/voxel_system_integration.cpp
#include "../../include/voxel/voxel_system_integration.h"
#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include <QSet>
#include <stdexcept>

VoxelSystemIntegration::VoxelSystemIntegration(GameScene* gameScene, QObject* parent)
    : QObject(parent),
      m_world(nullptr),
      m_renderer(nullptr),
      m_sky(nullptr),
      m_gameScene(gameScene) {
    
    // Create components
    try {
        m_world = new VoxelWorld(this);
        m_renderer = new VoxelRenderer();
        
        // Create sky system last
        m_sky = new SkySystem(this);
        
        // Connect signals
        connectSignals();
    } catch (const std::exception& e) {
        qCritical() << "Failed to create voxel system components:" << e.what();
        // Clean up partially initialized components
        delete m_renderer;
        m_renderer = nullptr;
        // m_world and m_sky have 'this' as parent, so they'll be cleaned up automatically
    }
}

VoxelSystemIntegration::~VoxelSystemIntegration() {
    delete m_renderer; // m_renderer doesn't have a parent, so delete it explicitly
}

void VoxelSystemIntegration::initialize() {
    // Check if components exist
    if (!m_world || !m_renderer) {
        qCritical() << "Cannot initialize voxel system: components not created";
        throw std::runtime_error("Voxel system components not created");
    }
    
    try {
        // Initialize OpenGL functions
        initializeOpenGLFunctions();
        
        // Initialize renderer
        m_renderer->initialize();
        m_renderer->setWorld(m_world);
        
        // Initialize sky system if it exists
        if (m_sky) {
            m_sky->initialize();
        } else {
            qWarning() << "Sky system is null during initialization";
        }
    } catch (const std::exception& e) {
        qCritical() << "Failed to initialize voxel system:" << e.what();
        throw; // Rethrow to notify caller
    }
}

void VoxelSystemIntegration::render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix) {
    try {
        // Check component validity
        if (!m_renderer || !m_world) {
            return;
        }

        // Clear color and depth buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Enable depth testing
        glEnable(GL_DEPTH_TEST);
        
        // Enable alpha blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Render sky if it exists
        if (m_sky) {
            try {
                m_sky->render(viewMatrix, projectionMatrix);
            } catch (...) {
                // Silent catch - just continue
            }
        }
        
        // Render voxel world
        try {
            m_renderer->render(viewMatrix, projectionMatrix);
        } catch (...) {
            // Silent catch - just continue
        }
    } catch (...) {
        // Silent catch - keep rendering flow intact
    }
}

void VoxelSystemIntegration::createDefaultWorld() {
    if (!m_world) {
        qCritical() << "Cannot create default world: world component not initialized";
        return;
    }
    
    try {
        // Create room 20x20 with 3m height (safer value)
        m_world->createRoomWithWalls(20, 20, 3);
        
        // Update the game scene to match the voxel world
        updateGameScene();
    } catch (const std::exception& e) {
        qCritical() << "Failed to create default world:" << e.what();
    }
}

// Connect signals
void VoxelSystemIntegration::connectSignals() {
    // Connect world changes to renderer if both exist
    if (m_world && m_renderer) {
        // Only update renderdata when world changes
        connect(m_world, &VoxelWorld::worldChanged, m_renderer, &VoxelRenderer::updateRenderData);
        
        // Only update game scene once on initial world creation
        // We don't need constant updates as voxels are static once created
        QTimer::singleShot(500, this, &VoxelSystemIntegration::updateGameScene);
    }
    
    // Disconnect any existing sky signal connections
    if (m_sky) {
        disconnect(m_sky, &SkySystem::skyColorChanged, nullptr, nullptr);
    }
}

void VoxelSystemIntegration::updateGameScene() {
    // Prevent frequent updates using static timestamp
    static qint64 lastUpdateTime = 0;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    // Only update every 5 seconds maximum
    if (currentTime - lastUpdateTime < 5000) {
        return;
    }
    lastUpdateTime = currentTime;

    if (!m_gameScene || !m_world) {
        return;
    }
    
    // Static flag to prevent recursive updates
    static bool isUpdating = false;
    if (isUpdating) {
        return;
    }
    
    isUpdating = true;
    
    try {
        // Get all visible voxels
        QVector<VoxelPos> visibleVoxels = m_world->getVisibleVoxels();
        
        // Limit to a reasonable number to avoid overload
        const int MAX_VOXELS = 100;
        int voxelCount = qMin(MAX_VOXELS, visibleVoxels.size());
        
        // Keep track of current entities
        QSet<QString> existingVoxelEntities;
        QSet<QString> voxelsToKeep;
        
        // Get existing voxel entities
        QVector<GameEntity> allEntities = m_gameScene->getAllEntities();
        for (const GameEntity& entity : allEntities) {
            if (entity.type == "voxel") {
                existingVoxelEntities.insert(entity.id);
            }
        }
        
        // Process visible voxels
        for (int i = 0; i < voxelCount; i++) {
            const VoxelPos& pos = visibleVoxels[i];
            Voxel voxel = m_world->getVoxel(pos);
            
            // Skip air voxels
            if (voxel.type == VoxelType::Air) continue;
            
            // Create ID for this voxel
            QString voxelId = QString("voxel_%1_%2_%3").arg(pos.x).arg(pos.y).arg(pos.z);
            voxelsToKeep.insert(voxelId);
            
            // Check if this voxel entity already exists
            if (existingVoxelEntities.contains(voxelId)) {
                // Entity exists, no need to re-add it
                continue;
            }
            
            // Create a new entity for this voxel
            GameEntity voxelEntity;
            voxelEntity.id = voxelId;
            voxelEntity.type = "voxel";
            voxelEntity.position = QVector3D(pos.x, pos.y, pos.z);
            voxelEntity.dimensions = QVector3D(1.0f, 1.0f, 1.0f);
            voxelEntity.isStatic = true;
            
            // Add to game scene
            m_gameScene->addEntity(voxelEntity);
        }
        
        // Remove voxel entities that are no longer visible
        QSet<QString> voxelsToRemove = existingVoxelEntities - voxelsToKeep;
        for (const QString& voxelId : voxelsToRemove) {
            m_gameScene->removeEntity(voxelId);
        }
        
        // Update celestial entities (sun and moon)
        // For these, use direct position updates instead of removing/re-adding
        if (m_sky) {
            // Update sun position
            GameEntity sunEntity = m_gameScene->getEntity("sun");
            QVector3D sunPos = m_sky->getSunPosition();
            
            if (sunEntity.id.isEmpty()) {
                // Create new sun entity if it doesn't exist
                sunEntity.id = "sun";
                sunEntity.type = "celestial";
                sunEntity.dimensions = QVector3D(5.0f, 5.0f, 5.0f);
                sunEntity.isStatic = false;
                sunEntity.position = sunPos;
                m_gameScene->addEntity(sunEntity);
            } else if ((sunEntity.position - sunPos).length() > 0.1f) {
                // Only update position if it changed significantly
                // Use updateEntityPosition method directly to avoid remove/add cycle
                m_gameScene->updateEntityPosition("sun", sunPos);
            }
            
            // Update moon position
            GameEntity moonEntity = m_gameScene->getEntity("moon");
            QVector3D moonPos = m_sky->getMoonPosition();
            
            if (moonEntity.id.isEmpty()) {
                // Create new moon entity if it doesn't exist
                moonEntity.id = "moon";
                moonEntity.type = "celestial";
                moonEntity.dimensions = QVector3D(3.0f, 3.0f, 3.0f);
                moonEntity.isStatic = false;
                moonEntity.position = moonPos;
                m_gameScene->addEntity(moonEntity);
            } else if ((moonEntity.position - moonPos).length() > 0.1f) {
                // Only update position if it changed significantly
                // Use updateEntityPosition method directly to avoid remove/add cycle
                m_gameScene->updateEntityPosition("moon", moonPos);
            }
        }
    } catch (const std::exception& e) {
        qCritical() << "Exception in updateGameScene:" << e.what();
    }
    
    isUpdating = false;
}