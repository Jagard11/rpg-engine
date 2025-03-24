// src/arena/voxels/voxel_system_integration.cpp
#include "../../../include/arena/voxels/voxel_system_integration.h"
#include "../../../include/arena/ui/voxel_highlight_renderer.h"
#include <QDebug>
#include <QDir>

VoxelSystemIntegration::VoxelSystemIntegration(GameScene* gameScene, QObject* parent)
    : QObject(parent),
      m_world(nullptr),
      m_renderer(nullptr),
      m_sky(nullptr),
      m_gameScene(gameScene),
      m_highlightRenderer(nullptr),
      m_highlightedVoxelFace(-1)
{
    qDebug() << "Creating VoxelSystemIntegration...";
    
    // Create voxel world
    qDebug() << "Creating VoxelWorld...";
    m_world = new VoxelWorld(this);
    
    // Create renderer
    qDebug() << "Creating VoxelRenderer...";
    m_renderer = new VoxelRenderer(this);
    
    // Create voxel highlight renderer
    qDebug() << "Creating VoxelHighlightRenderer...";
    m_highlightRenderer = new VoxelHighlightRenderer(this);
    
    // Create sky system
    qDebug() << "Creating SkySystem...";
    m_sky = new SkySystem(this);
    
    // Connect signals
    qDebug() << "Connecting world signals to renderer...";
    connectSignals();
    
    qDebug() << "VoxelSystemIntegration created successfully";
}

VoxelSystemIntegration::~VoxelSystemIntegration()
{
    // World, renderer, and sky are QObjects with parent-child relationships
    // that will be automatically cleaned up
    
    // Clean up world system which is a unique_ptr
    m_worldSystem.reset();
}

void VoxelSystemIntegration::initialize()
{
    qDebug() << "Initializing VoxelSystemIntegration...";
    
    try {
        // Initialize OpenGL functions
        initializeOpenGLFunctions();
        
        // Initialize renderer with world reference
        qDebug() << "Initializing VoxelRenderer...";
        m_renderer->setWorld(m_world);
        m_renderer->initialize();
        
        // Initialize highlight renderer
        qDebug() << "Initializing VoxelHighlightRenderer...";
        m_highlightRenderer->initialize();
        
        // Initialize sky system
        qDebug() << "Initializing SkySystem...";
        m_sky->initialize();
        
        // Connect any remaining signals
        connectSignals();
        
        qDebug() << "VoxelSystemIntegration initialization complete";
    }
    catch (const std::exception& e) {
        qCritical() << "Exception during VoxelSystemIntegration initialization:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown exception during VoxelSystemIntegration initialization";
    }
}

void VoxelSystemIntegration::render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix)
{
    // Safety checks before rendering
    if (!m_renderer || !m_sky || !m_highlightRenderer) {
        return;
    }
    
    try {
        // Render sky background first
        m_sky->render(viewMatrix, projectionMatrix);
        
        // Render voxel world
        m_renderer->render(viewMatrix, projectionMatrix);
        
        // Render voxel highlight if needed
        if (m_highlightedVoxelFace >= 0) {
            m_highlightRenderer->render(viewMatrix, projectionMatrix, 
                                       m_highlightedVoxelPos, m_highlightedVoxelFace);
        }
    }
    catch (const std::exception& e) {
        qWarning() << "Exception during VoxelSystemIntegration rendering:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception during VoxelSystemIntegration rendering";
    }
}

VoxelWorld* VoxelSystemIntegration::getWorld() const
{
    return m_world;
}

void VoxelSystemIntegration::createDefaultWorld()
{
    qDebug() << "Creating default world...";
    
    try {
        // Create a simple room with walls for now
        m_world->createRoomWithWalls(16, 16, 4);
        
        // Create a WorldSystem if needed later
        // m_worldSystem = std::make_unique<VoxelWorldSystem>(this);
        // m_worldSystem->initialize(VoxelWorldSystem::WorldType::Flat, 12345);
        
        // Update game scene to match voxel world
        updateGameScene();
        
        emit worldChanged();
    }
    catch (const std::exception& e) {
        qCritical() << "Exception creating default world:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown exception creating default world";
    }
}

void VoxelSystemIntegration::createSphericalPlanet(float radius, float terrainHeight, unsigned int seed)
{
    try {
        if (!m_worldSystem) {
            m_worldSystem = std::make_unique<VoxelWorldSystem>(this);
        }
        
        m_worldSystem->initialize(VoxelWorldSystem::WorldType::Spherical, seed);
        
        // Configure spherical planet parameters
        // This would typically modify the generator's parameters
        
        // Update game scene to match voxel world
        updateGameScene();
        
        emit worldChanged();
    }
    catch (const std::exception& e) {
        qCritical() << "Exception creating spherical planet:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown exception creating spherical planet";
    }
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

bool VoxelSystemIntegration::raycast(const QVector3D& origin, const QVector3D& direction, 
                                   float maxDistance, QVector3D& outHitPos, 
                                   QVector3D& outHitNormal, Voxel& outVoxel)
{
    // Simple raycast implementation for voxel world
    const float stepSize = 0.1f; // Step size for ray marching
    
    // Normalize direction
    QVector3D rayDir = direction.normalized();
    
    // Perform ray marching
    for (float distance = 0.0f; distance < maxDistance; distance += stepSize) {
        // Calculate current position along ray
        QVector3D currentPos = origin + rayDir * distance;
        
        // Get voxel at current position
        Voxel voxel = m_world->getVoxel(
            floor(currentPos.x()), 
            floor(currentPos.y()), 
            floor(currentPos.z())
        );
        
        // Check if voxel is solid (not air)
        if (voxel.type != VoxelType::Air) {
            // Calculate hit position
            outHitPos = currentPos;
            
            // Calculate normal by checking neighboring voxels
            // This is a simple approach - checking in the 6 main directions
            QVector3D normal(0, 0, 0);
            
            // Check x-axis
            if (m_world->getVoxel(floor(currentPos.x()) - 1, floor(currentPos.y()), floor(currentPos.z())).type == VoxelType::Air) {
                normal.setX(-1);
            } else if (m_world->getVoxel(floor(currentPos.x()) + 1, floor(currentPos.y()), floor(currentPos.z())).type == VoxelType::Air) {
                normal.setX(1);
            }
            
            // Check y-axis
            if (m_world->getVoxel(floor(currentPos.x()), floor(currentPos.y()) - 1, floor(currentPos.z())).type == VoxelType::Air) {
                normal.setY(-1);
            } else if (m_world->getVoxel(floor(currentPos.x()), floor(currentPos.y()) + 1, floor(currentPos.z())).type == VoxelType::Air) {
                normal.setY(1);
            }
            
            // Check z-axis
            if (m_world->getVoxel(floor(currentPos.x()), floor(currentPos.y()), floor(currentPos.z()) - 1).type == VoxelType::Air) {
                normal.setZ(-1);
            } else if (m_world->getVoxel(floor(currentPos.x()), floor(currentPos.y()), floor(currentPos.z()) + 1).type == VoxelType::Air) {
                normal.setZ(1);
            }
            
            // If normal is still zero, use the ray direction as a fallback
            if (normal.lengthSquared() < 0.01f) {
                normal = -rayDir;
            } else {
                normal.normalize();
            }
            
            outHitNormal = normal;
            outVoxel = voxel;
            
            return true;
        }
    }
    
    // No hit
    return false;
}

bool VoxelSystemIntegration::placeVoxel(const QVector3D& hitPos, const QVector3D& normal, const Voxel& voxel)
{
    // Calculate position for new voxel (adjacent to hit position in direction of normal)
    QVector3D newPos = hitPos + normal * 0.5f;
    
    // Round to nearest voxel position
    int x = static_cast<int>(floor(newPos.x()));
    int y = static_cast<int>(floor(newPos.y()));
    int z = static_cast<int>(floor(newPos.z()));
    
    // Place the voxel
    m_world->setVoxel(x, y, z, voxel);
    
    // Update game scene
    updateGameScene();
    
    emit worldChanged();
    
    return true;
}

bool VoxelSystemIntegration::removeVoxel(const QVector3D& hitPos)
{
    // Round to nearest voxel position
    int x = static_cast<int>(floor(hitPos.x()));
    int y = static_cast<int>(floor(hitPos.y()));
    int z = static_cast<int>(floor(hitPos.z()));
    
    // Create an air voxel (empty space)
    Voxel airVoxel;
    airVoxel.type = VoxelType::Air;
    
    // Remove the voxel (replace with air)
    m_world->setVoxel(x, y, z, airVoxel);
    
    // Update game scene
    updateGameScene();
    
    emit worldChanged();
    
    return true;
}

void VoxelSystemIntegration::updateGameScene()
{
    // Safety check
    if (!m_gameScene || !m_world) {
        return;
    }

    try {
        // Get all visible voxels
        QVector<VoxelPos> visibleVoxels = m_world->getVisibleVoxels();
        
        // Remove existing voxel entities
        QStringList voxelEntities;
        for (auto it = 0; it < visibleVoxels.size(); ++it) {
            voxelEntities.append(QString("voxel_%1_%2_%3")
                              .arg(visibleVoxels[it].x)
                              .arg(visibleVoxels[it].y)
                              .arg(visibleVoxels[it].z));
        }
        
        // Add voxel entities to the game scene
        for (const VoxelPos& pos : visibleVoxels) {
            // Skip air voxels
            Voxel voxel = m_world->getVoxel(pos);
            if (voxel.type == VoxelType::Air) {
                continue;
            }
            
            // Create entity ID
            QString entityId = QString("voxel_%1_%2_%3")
                             .arg(pos.x)
                             .arg(pos.y)
                             .arg(pos.z);
            
            // Create game entity
            GameEntity entity;
            entity.id = entityId;
            entity.type = "voxel";
            entity.position = QVector3D(pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f); // Center of voxel
            entity.dimensions = QVector3D(1.0f, 1.0f, 1.0f); // Standard voxel size
            entity.isStatic = true;
            
            // Add to game scene
            m_gameScene->addEntity(entity);
        }
    }
    catch (const std::exception& e) {
        qWarning() << "Exception updating game scene:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception updating game scene";
    }
}

void VoxelSystemIntegration::streamChunksAroundPlayer(const QVector3D& playerPosition)
{
    // If using the chunk-based VoxelWorldSystem, we would update loaded chunks here
    if (m_worldSystem) {
        try {
            m_worldSystem->updateAroundViewer(playerPosition);
        }
        catch (const std::exception& e) {
            qWarning() << "Exception streaming chunks:" << e.what();
        }
        catch (...) {
            qWarning() << "Unknown exception streaming chunks";
        }
    }
}

void VoxelSystemIntegration::connectSignals()
{
    // Connect world signals to renderer
    // These signal connections ensure the renderer updates when the world changes
    if (m_world && m_renderer) {
        connect(m_world, &VoxelWorld::worldChanged, m_renderer, &VoxelRenderer::updateRenderData);
        connect(m_world, &VoxelWorld::worldChanged, this, &VoxelSystemIntegration::updateGameScene);
    }
    
    // Connect world system signals if available
    if (m_worldSystem) {
        connect(m_worldSystem.get(), &VoxelWorldSystem::chunkLoaded, 
                this, &VoxelSystemIntegration::chunkLoaded);
        connect(m_worldSystem.get(), &VoxelWorldSystem::chunkUnloaded, 
                this, &VoxelSystemIntegration::chunkUnloaded);
        connect(m_worldSystem.get(), &VoxelWorldSystem::chunkModified, 
                this, &VoxelSystemIntegration::updateGameScene);
        connect(m_worldSystem.get(), &VoxelWorldSystem::memoryUsageChanged, 
                [this](size_t usage, size_t maxUsage) {
                    qDebug() << "Voxel memory usage:" << usage / (1024 * 1024) << "MB of" 
                            << maxUsage / (1024 * 1024) << "MB";
                });
    }
}