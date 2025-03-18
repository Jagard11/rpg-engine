// src/voxel/voxel_system_integration.cpp
#include "../../include/voxel/voxel_system_integration.h"
#include <QDebug>
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
        
        qDebug() << "VoxelSystemIntegration created successfully";
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
        }
        
        qDebug() << "Voxel system initialized successfully";
    } catch (const std::exception& e) {
        qCritical() << "Failed to initialize voxel system:" << e.what();
        throw; // Rethrow to notify caller
    }
}

void VoxelSystemIntegration::render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix) {
    try {
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
            } catch (const std::exception& e) {
                qWarning() << "Error rendering sky:" << e.what();
            }
        }
        
        // Render voxel world if it exists
        if (m_renderer && m_world) {
            try {
                m_renderer->render(viewMatrix, projectionMatrix);
            } catch (const std::exception& e) {
                qWarning() << "Error rendering voxels:" << e.what();
            }
        }
    } catch (const std::exception& e) {
        qCritical() << "Exception during voxel system rendering:" << e.what();
    }
}

void VoxelSystemIntegration::createDefaultWorld() {
    if (!m_world) {
        qCritical() << "Cannot create default world: world component not initialized";
        return;
    }
    
    try {
        qDebug() << "Creating default voxel world";
        
        // Create room 20x20 with 3m height (safer value)
        m_world->createRoomWithWalls(20, 20, 3);
        
        // Update the game scene to match the voxel world
        updateGameScene();
        
        qDebug() << "Default world created successfully";
    } catch (const std::exception& e) {
        qCritical() << "Failed to create default world:" << e.what();
    }
}

void VoxelSystemIntegration::connectSignals() {
    // Connect world changes to renderer if both exist
    if (m_world && m_renderer) {
        connect(m_world, &VoxelWorld::worldChanged, m_renderer, &VoxelRenderer::updateRenderData);
        connect(m_world, &VoxelWorld::worldChanged, this, &VoxelSystemIntegration::updateGameScene);
    }
    
    // Disconnect any existing sky signal connections
    if (m_sky) {
        disconnect(m_sky, &SkySystem::skyColorChanged, nullptr, nullptr);
        
        // Connect sky color changes with a safer approach
        connect(m_sky, &SkySystem::skyColorChanged, [this](const QColor& color) {
            qDebug() << "Sky color changed to" << color.name();
            // Don't modify OpenGL state here - that will happen during render
        });
    }
}

void VoxelSystemIntegration::updateGameScene() {
    if (!m_gameScene || !m_world) {
        qWarning() << "Cannot update game scene: missing components";
        return;
    }
    
    try {
        // Store player entity if it exists
        GameEntity player;
        bool hasPlayer = false;
        
        if (m_gameScene->getEntity("player").id == "player") {
            player = m_gameScene->getEntity("player");
            hasPlayer = true;
        }
        
        // Remove all entities except player
        QVector<QString> entitiesToRemove;
        
        for (const GameEntity& entity : m_gameScene->getAllEntities()) {
            if (entity.id != "player") {
                entitiesToRemove.append(entity.id);
            }
        }
        
        for (const QString& id : entitiesToRemove) {
            m_gameScene->removeEntity(id);
        }
        
        // Re-add player if it existed
        if (hasPlayer) {
            m_gameScene->addEntity(player);
        }
        
        // Add voxel entities, limit to a reasonable number to avoid overload
        const int MAX_ENTITIES = 100;
        QVector<VoxelPos> visibleVoxels = m_world->getVisibleVoxels();
        
        // Create entities for visible voxels up to limit
        int entityCount = 0;
        for (int i = 0; i < qMin(MAX_ENTITIES, visibleVoxels.size()); i++) {
            const VoxelPos& pos = visibleVoxels[i];
            Voxel voxel = m_world->getVoxel(pos);
            
            // Skip air voxels
            if (voxel.type == VoxelType::Air) continue;
            
            // Create a game entity for the voxel
            GameEntity voxelEntity;
            voxelEntity.id = QString("voxel_%1_%2_%3").arg(pos.x).arg(pos.y).arg(pos.z);
            voxelEntity.type = "voxel";
            voxelEntity.position = QVector3D(pos.x, pos.y, pos.z);
            voxelEntity.dimensions = QVector3D(1.0f, 1.0f, 1.0f);
            voxelEntity.isStatic = true;
            
            // Add to game scene
            m_gameScene->addEntity(voxelEntity);
            entityCount++;
        }
        
        qDebug() << "Updated game scene with" << entityCount << "voxel entities";
        
        // Add simplified celestial entities
        if (m_sky) {
            // Add sun
            GameEntity sunEntity;
            sunEntity.id = "sun";
            sunEntity.type = "celestial";
            sunEntity.position = m_sky->getSunPosition();
            sunEntity.dimensions = QVector3D(5.0f, 5.0f, 5.0f);
            sunEntity.isStatic = false;
            m_gameScene->addEntity(sunEntity);
            
            // Add moon
            GameEntity moonEntity;
            moonEntity.id = "moon";
            moonEntity.type = "celestial";
            moonEntity.position = m_sky->getMoonPosition();
            moonEntity.dimensions = QVector3D(3.0f, 3.0f, 3.0f);
            moonEntity.isStatic = false;
            m_gameScene->addEntity(moonEntity);
        }
    } catch (const std::exception& e) {
        qCritical() << "Exception in updateGameScene:" << e.what();
    }
}