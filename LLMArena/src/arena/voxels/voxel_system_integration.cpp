// src/arena/voxels/voxel_system_integration.cpp
#include "../../../include/arena/voxels/voxel_system_integration.h"
#include "../../../include/arena/voxels/world/voxel_world_system.h"
#include "../../../include/arena/skybox/skybox_core.h"
#include "../../../include/arena/ui/voxel_highlight_renderer.h"
#include "../../../include/arena/game/player_controller.h"
#include "../../../include/arena/core/arena_core.h"

#include <QDebug>
#include <QMatrix4x4>
#include <QOpenGLContext>

VoxelSystemIntegration::VoxelSystemIntegration(GameScene* gameScene, QObject* parent)
    : QObject(parent),
      m_gameScene(gameScene),
      m_world(nullptr),
      m_renderer(nullptr),
      m_sky(nullptr),
      m_highlightedVoxelPos(0, 0, 0),  // Initialize with zero vector
      m_highlightedVoxelFace(-1)
{
    qDebug() << "Creating VoxelSystemIntegration...";
    
    // Create voxel world for testing
    // Note: We're just creating the world object here, not initializing OpenGL resources
    m_world = new VoxelWorld(this);
    qDebug() << "Creating VoxelWorld...";
    
    // Create renderer but don't initialize it yet
    m_renderer = new VoxelRenderer(this);
    qDebug() << "Creating VoxelRenderer...";
    
    // Create voxel highlight renderer but don't initialize it yet
    m_highlightRenderer = new VoxelHighlightRenderer(this);
    qDebug() << "Creating VoxelHighlightRenderer...";
    
    // Create sky system but don't initialize it yet
    m_sky = new SkySystem(this);
    qDebug() << "Creating SkySystem...";
    
    // Connect signals
    connectSignals();
    qDebug() << "Connecting world signals to renderer...";
    
    qDebug() << "VoxelSystemIntegration created successfully";
}

VoxelSystemIntegration::~VoxelSystemIntegration()
{
    // The parent-child relationship will handle deletion of these objects
}

void VoxelSystemIntegration::initialize()
{
    // Safety check to ensure we have an OpenGL context
    if (!QOpenGLContext::currentContext()) {
        qWarning() << "No OpenGL context available during VoxelSystemIntegration initialization";
        return;
    }
    
    qDebug() << "Initializing VoxelSystemIntegration...";
    
    // Initialize renderer
    try {
        qDebug() << "Initializing VoxelRenderer...";
        initializeOpenGLFunctions();
        m_renderer->setWorld(m_world);
        m_renderer->initialize();
    } catch (const std::exception& e) {
        qCritical() << "Exception initializing renderer:" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception initializing renderer";
    }
    
    // Initialize highlight renderer
    try {
        qDebug() << "Initializing VoxelHighlightRenderer...";
        m_highlightRenderer->initialize();
    } catch (const std::exception& e) {
        qCritical() << "Exception initializing highlight renderer:" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception initializing highlight renderer";
    }
    
    // Initialize sky
    try {
        qDebug() << "Initializing SkySystem...";
        m_sky->initialize();
    } catch (const std::exception& e) {
        qCritical() << "Exception initializing sky system:" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception initializing sky system";
    }
    
    qDebug() << "VoxelSystemIntegration initialization complete";
}

void VoxelSystemIntegration::render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix)
{
    // Skip if not ready
    if (!QOpenGLContext::currentContext()) {
        return;
    }
    
    // Render sky first (background)
    if (m_sky) {
        try {
            m_sky->render(viewMatrix, projectionMatrix);
        } catch (const std::exception& e) {
            qWarning() << "Exception rendering sky:" << e.what();
        } catch (...) {
            qWarning() << "Unknown exception rendering sky";
        }
    }
    
    // Render voxel world
    if (m_renderer && m_world) {
        try {
            m_renderer->render(viewMatrix, projectionMatrix);
        } catch (const std::exception& e) {
            qWarning() << "Exception rendering voxels:" << e.what();
        } catch (...) {
            qWarning() << "Unknown exception rendering voxels";
        }
    }
    
    // Render voxel highlight if enabled
    if (m_highlightRenderer && m_highlightedVoxelFace != -1) {
        try {
            m_highlightRenderer->render(viewMatrix, projectionMatrix, 
                                      m_highlightedVoxelPos, m_highlightedVoxelFace);
        } catch (const std::exception& e) {
            qWarning() << "Exception rendering highlight:" << e.what();
        } catch (...) {
            qWarning() << "Unknown exception rendering highlight";
        }
    }
}

VoxelWorld* VoxelSystemIntegration::getWorld() const
{
    return m_world;
}

void VoxelSystemIntegration::createDefaultWorld()
{
    if (!m_world) {
        qWarning() << "No voxel world available";
        return;
    }
    
    qDebug() << "Creating default world...";
    // Create a simple world with a room
    m_world->createRoomWithWalls(10, 10, 5);
    
    // Add some voxels for testing
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            // Create a platform
            if (i < 3 && j < 3) {
                // Platform
                VoxelPos pos(i, 1, j);
                Voxel voxel(VoxelType::Cobblestone, QColor(128, 128, 128));
                m_world->setVoxel(pos, voxel);
            }
            
            // Create some columns
            if ((i == 4 && j == 4) || (i == 0 && j == 4) || (i == 4 && j == 0)) {
                for (int h = 0; h < 3; h++) {
                    VoxelPos pos(i, h, j);
                    Voxel voxel((h == 0) ? VoxelType::Cobblestone : 
                               ((h == 1) ? VoxelType::Dirt : VoxelType::Grass),
                               (h == 0) ? QColor(128, 128, 128) :
                               ((h == 1) ? QColor(139, 69, 19) : QColor(34, 139, 34)));
                    m_world->setVoxel(pos, voxel);
                }
            }
        }
    }
    
    // Update game scene with new world data
    updateGameScene();
}

void VoxelSystemIntegration::createSphericalPlanet(float radius, float terrainHeight, unsigned int seed)
{
    // To be implemented
}

void VoxelSystemIntegration::setVoxelHighlight(const VoxelPos& pos, int face)
{
    m_highlightedVoxelPos = pos.toVector3D();
    m_highlightedVoxelFace = face;
}

VoxelPos VoxelSystemIntegration::getHighlightedVoxelPos() const
{
    return VoxelPos::fromVector3D(m_highlightedVoxelPos);
}

int VoxelSystemIntegration::getHighlightedVoxelFace() const
{
    return m_highlightedVoxelFace;
}

bool VoxelSystemIntegration::raycast(const QVector3D& origin, const QVector3D& direction, float maxDistance,
                                 QVector3D& outHitPos, QVector3D& outHitNormal, Voxel& outVoxel)
{
    // Simple raycast for now (will be improved later)
    float step = 0.1f;
    float distance = 0.0f;
    
    while (distance < maxDistance) {
        QVector3D position = origin + direction * distance;
        VoxelPos voxelPos = VoxelPos::fromVector3D(position);
        
        Voxel voxel = m_world->getVoxel(voxelPos);
        if (voxel.type != VoxelType::Air) {
            // Found a hit
            outHitPos = position;
            
            // Determine hit normal (crude approximation)
            float dx = position.x() - floor(position.x()) - 0.5f;
            float dy = position.y() - floor(position.y()) - 0.5f;
            float dz = position.z() - floor(position.z()) - 0.5f;
            
            if (fabs(dx) > fabs(dy) && fabs(dx) > fabs(dz)) {
                outHitNormal = QVector3D(dx > 0 ? 1 : -1, 0, 0);
            } else if (fabs(dy) > fabs(dx) && fabs(dy) > fabs(dz)) {
                outHitNormal = QVector3D(0, dy > 0 ? 1 : -1, 0);
            } else {
                outHitNormal = QVector3D(0, 0, dz > 0 ? 1 : -1);
            }
            
            outVoxel = voxel;
            return true;
        }
        
        distance += step;
    }
    
    return false;
}

bool VoxelSystemIntegration::placeVoxel(const QVector3D& hitPos, const QVector3D& normal, const Voxel& voxel)
{
    // Calculate voxel position to place at
    QVector3D placePos = hitPos + normal * 0.5f;
    VoxelPos voxelPos = VoxelPos::fromVector3D(placePos);
    
    // Check if position is valid
    if (!voxelPos.isValid()) {
        return false;
    }
    
    // Place the voxel
    m_world->setVoxel(voxelPos, voxel);
    
    // Update game scene
    updateGameScene();
    
    return true;
}

bool VoxelSystemIntegration::removeVoxel(const QVector3D& hitPos)
{
    // Calculate voxel position to remove
    VoxelPos voxelPos = VoxelPos::fromVector3D(hitPos);
    
    // Check if position is valid
    if (!voxelPos.isValid()) {
        return false;
    }
    
    // Create an air voxel (empty space)
    Voxel airVoxel(VoxelType::Air, Qt::transparent);
    
    // Replace the voxel with air
    m_world->setVoxel(voxelPos, airVoxel);
    
    // Update game scene
    updateGameScene();
    
    return true;
}

void VoxelSystemIntegration::updateGameScene()
{
    if (!m_gameScene) {
        return;
    }
    
    // Get all solid voxels
    const QHash<VoxelPos, Voxel>& allVoxels = m_world->getAllVoxels();
    int addedVoxels = 0;
    
    // Add solid voxels to scene as collision entities
    for (auto it = allVoxels.constBegin(); it != allVoxels.constEnd(); ++it) {
        const VoxelPos& pos = it.key();
        const Voxel& voxel = it.value();
        
        if (voxel.type != VoxelType::Air) {
            // Create a game entity for collision
            GameEntity voxelEntity;
            voxelEntity.id = QString("voxel_%1_%2_%3").arg(pos.x).arg(pos.y).arg(pos.z);
            voxelEntity.type = "voxel";
            voxelEntity.position = QVector3D(pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f);
            voxelEntity.dimensions = QVector3D(1.0f, 1.0f, 1.0f);
            voxelEntity.isStatic = true;
            
            // Add to scene
            m_gameScene->addEntity(voxelEntity);
            addedVoxels++;
        }
    }
    
    emit worldChanged();
}

void VoxelSystemIntegration::streamChunksAroundPlayer(const QVector3D& playerPosition)
{
    // To be implemented when chunk streaming is added
}

void VoxelSystemIntegration::connectSignals()
{
    if (m_world) {
        connect(m_world, &VoxelWorld::worldChanged, 
                m_renderer, &VoxelRenderer::updateRenderData);
        
        connect(m_world, &VoxelWorld::worldChanged, 
                this, &VoxelSystemIntegration::updateGameScene);
    }
}