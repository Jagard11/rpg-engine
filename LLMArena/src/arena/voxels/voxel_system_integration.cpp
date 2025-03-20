// src/arena/voxels/voxel_system_integration.cpp
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
      m_gameScene(gameScene),
      m_highlightRenderer(nullptr) {
    
    qDebug() << "Creating VoxelSystemIntegration...";
    
    // Initialize components to null first
    m_world = nullptr;
    m_renderer = nullptr;
    m_highlightRenderer = nullptr;
    m_sky = nullptr;
    
    // Create components in order
    try {
        qDebug() << "Creating VoxelWorld...";
        m_world = new VoxelWorld(this);
        
        qDebug() << "Creating VoxelRenderer...";
        m_renderer = new VoxelRenderer(this);
        
        qDebug() << "Creating VoxelHighlightRenderer...";
        m_highlightRenderer = new VoxelHighlightRenderer(this);
        
        // Create sky system last
        qDebug() << "Creating SkySystem...";
        m_sky = new SkySystem(this);
        
        // Connect signals
        connectSignals();
        
        qDebug() << "VoxelSystemIntegration created successfully";
    } catch (const std::exception& e) {
        qCritical() << "Failed to create voxel system components:" << e.what();
        // Clean up partially initialized components
        if (m_renderer) {
            delete m_renderer;
            m_renderer = nullptr;
        }
        if (m_highlightRenderer) {
            delete m_highlightRenderer;
            m_highlightRenderer = nullptr;
        }
        // Don't delete m_world and m_sky, they have 'this' as parent and will be cleaned up automatically
    }
}

VoxelSystemIntegration::~VoxelSystemIntegration() {
    // m_renderer and m_highlightRenderer now have 'this' as parent
    // so they'll be cleaned up automatically
}

void VoxelSystemIntegration::initialize() {
    // Check if components exist
    if (!m_world || !m_renderer) {
        qCritical() << "Cannot initialize voxel system: components not created";
        return;
    }
    
    // Wrap in try-catch to prevent crashes
    try {
        qDebug() << "Initializing VoxelSystemIntegration...";
        
        // Initialize OpenGL functions
        initializeOpenGLFunctions();
        
        // Initialize renderer
        qDebug() << "Initializing VoxelRenderer...";
        m_renderer->initialize();
        m_renderer->setWorld(m_world);
        
        // Initialize highlight renderer
        if (m_highlightRenderer) {
            qDebug() << "Initializing VoxelHighlightRenderer...";
            m_highlightRenderer->initialize();
        }
        
        // Initialize sky system if it exists
        if (m_sky) {
            qDebug() << "Initializing SkySystem...";
            m_sky->initialize();
        } else {
            qWarning() << "Sky system is null during initialization";
        }
        
        qDebug() << "VoxelSystemIntegration initialization complete";
    } catch (const std::exception& e) {
        qCritical() << "Failed to initialize voxel system:" << e.what();
        // Don't rethrow to avoid crashes
    } catch (...) {
        qCritical() << "Unknown exception in voxel system initialization";
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
        
        // Render voxel highlight if needed
        if (m_highlightRenderer && m_highlightedVoxelPos.isValid() && m_highlightedVoxelFace >= 0) {
            try {
                m_highlightRenderer->render(viewMatrix, projectionMatrix, 
                                          m_highlightedVoxelPos.toVector3D(), 
                                          m_highlightedVoxelFace);
            } catch (...) {
                // Silent catch - just continue
            }
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
        qDebug() << "Creating default world...";
        
        // Create room 10x10 with 2m height (smaller values for safety)
        m_world->createRoomWithWalls(10, 10, 2);
        
        // Update the game scene to match the voxel world
        QTimer::singleShot(0, this, [this]() {
            updateGameScene();
        });
    } catch (const std::exception& e) {
        qCritical() << "Failed to create default world:" << e.what();
    }
}

// Connect signals
void VoxelSystemIntegration::connectSignals() {
    // Connect world changes to renderer if both exist
    if (m_world && m_renderer) {
        qDebug() << "Connecting world signals to renderer...";
        
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
        qDebug() << "Updating game scene from voxel world...";
        
        // First, remove all existing voxel entities from game scene
        QVector<GameEntity> allEntities = m_gameScene->getAllEntities();
        for (const GameEntity& entity : allEntities) {
            if (entity.type == "voxel") {
                m_gameScene->removeEntity(entity.id);
            }
        }
        
        // Get only visible voxels for walls - these are the ones that should have collision
        QVector<VoxelPos> visibleVoxels = m_world->getVisibleVoxels();
        
        // Create a set to track which voxels we've processed
        QSet<QString> processedVoxels;
        
        // Add a collision entity for each visible voxel
        for (const VoxelPos& pos : visibleVoxels) {
            Voxel voxel = m_world->getVoxel(pos);
            
            // Skip air voxels
            if (voxel.type == VoxelType::Air) continue;
            
            // Skip floor voxels (y=0) for collision but keep them for rendering
            if (pos.y == 0) continue;
            
            // Create ID for this voxel
            QString voxelId = QString("voxel_%1_%2_%3").arg(pos.x).arg(pos.y).arg(pos.z);
            processedVoxels.insert(voxelId);
            
            // Create a collision entity for this voxel
            GameEntity voxelEntity;
            voxelEntity.id = voxelId;
            voxelEntity.type = "voxel";
            voxelEntity.position = QVector3D(pos.x, pos.y, pos.z);
            voxelEntity.dimensions = QVector3D(1.0f, 1.0f, 1.0f);
            voxelEntity.isStatic = true;
            
            // Add to game scene
            m_gameScene->addEntity(voxelEntity);
        }
        
        // Debug output
        qDebug() << "Added" << processedVoxels.size() << "collision voxels";
        
        // Update celestial entities (sun and moon)
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
                m_gameScene->updateEntityPosition("moon", moonPos);
            }
        }
    } catch (const std::exception& e) {
        qCritical() << "Exception in updateGameScene:" << e.what();
    }
    
    isUpdating = false;
}

// Set voxel highlight
void VoxelSystemIntegration::setVoxelHighlight(const VoxelPos& pos, int face) {
    m_highlightedVoxelPos = pos;
    m_highlightedVoxelFace = face;
}

// Get highlight position
VoxelPos VoxelSystemIntegration::getHighlightedVoxelPos() const {
    return m_highlightedVoxelPos;
}

// Get highlight face
int VoxelSystemIntegration::getHighlightedVoxelFace() const {
    return m_highlightedVoxelFace;
}

// Get world reference
VoxelWorld* VoxelSystemIntegration::getWorld() const {
    return m_world;
}