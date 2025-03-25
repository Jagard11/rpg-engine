// src/arena/voxels/voxel_system_integration.cpp
#include "../../../include/arena/voxels/voxel_system_integration.h"
#include "../../../include/arena/ui/voxel_highlight_renderer.h"
#include <QDebug>
#include <QDir>
#include <QDateTime>

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
        
        // Initialize world system if it's not already created
        if (!m_worldSystem) {
            qDebug() << "Creating VoxelWorldSystem...";
            m_worldSystem = std::make_unique<VoxelWorldSystem>(this);
        }
        
        // Initialize renderer with direct reference to world system
        qDebug() << "Initializing VoxelRenderer...";
        
        // Set the world reference in the renderer - this is critical
        if (m_renderer && m_world) {
            m_renderer->setWorld(m_world);
            qDebug() << "Set VoxelWorld in renderer";
        } else {
            qWarning() << "Failed to set world in renderer - renderer or world is null";
        }
        
        // Important: ensure the renderer is updated when chunks change
        if (m_worldSystem && m_renderer) {
            try {
                QObject::connect(m_worldSystem.get(), &VoxelWorldSystem::chunkModified, 
                        m_renderer, &VoxelRenderer::updateRenderData, Qt::UniqueConnection);
                qDebug() << "Connected world system chunk modified signal to renderer";
            } catch (const std::exception& e) {
                qWarning() << "Exception connecting world system signals:" << e.what();
            }
        }
        
        // Initialize the renderer
        if (m_renderer) {
            m_renderer->initialize();
            qDebug() << "Renderer initialized";
        }
        
        // Initialize highlight renderer
        qDebug() << "Initializing VoxelHighlightRenderer...";
        if (m_highlightRenderer) {
            m_highlightRenderer->initialize();
        }
        
        // Initialize sky system
        qDebug() << "Initializing SkySystem...";
        if (m_sky) {
            m_sky->initialize();
        }
        
        // Connect any remaining signals
        connectSignals();
        
        // Force renderer update - this is important to display initial terrain
        if (m_renderer) {
            m_renderer->updateRenderData();
            qDebug() << "Forced renderer update";
        }
        
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
        qWarning() << "Cannot render - missing renderer components";
        return;
    }
    
    try {
        // Render sky background first
        if (m_sky) {
            m_sky->render(viewMatrix, projectionMatrix);
        }

        // Debug output to track rendering
        static int frameCount = 0;
        if (frameCount++ % 100 == 0) {  // Log every 100 frames
            qDebug() << "Rendering voxel world with" 
                     << (m_world ? m_world->getVisibleVoxels().size() : 0) 
                     << "visible voxels";
        }
        
        // Force update render data occasionally to ensure voxels are visible
        static int updateCount = 0;
        if (updateCount++ % 300 == 0) {  // Update every ~5 seconds at 60fps
            if (m_renderer) {
                m_renderer->updateRenderData();
            }
        }
        
        // Render voxel world
        if (m_renderer) {
            m_renderer->render(viewMatrix, projectionMatrix);
        }
        
        // Render voxel highlight if needed
        if (m_highlightRenderer && m_highlightedVoxelFace >= 0) {
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
    try {
        qDebug() << "Creating simplified default world...";
        
        // Clear the existing world first to avoid duplicate voxels
        if (m_world) {
            // Since there's no direct clear method, we'll just create an empty room
            // with air voxels to replace any existing terrain
            qDebug() << "Creating empty room to clear previous terrain";
            m_world->createRoomWithWalls(2, 2, 2);  // Small temporary room
            
            // Then fill the area with air voxels to ensure it's empty
            int clearSize = 32;
            Voxel airVoxel;
            airVoxel.type = VoxelType::Air;
            
            for (int x = -clearSize/2; x < clearSize/2; x++) {
                for (int y = 0; y < clearSize; y++) {
                    for (int z = -clearSize/2; z < clearSize/2; z++) {
                        m_world->setVoxel(x, y, z, airVoxel);
                    }
                }
            }
            
            qDebug() << "Cleared existing world data";
        }
        
        // Create a smaller and simpler terrain for better performance
        int worldSize = 16; // Reduced size
        int worldHeight = 16; // Reduced height
        
        qDebug() << "Creating terrain of size " << worldSize << "x" << worldHeight;
        
        // Create a simple heightmap-based terrain
        for (int x = -worldSize/2; x < worldSize/2; x++) {
            for (int z = -worldSize/2; z < worldSize/2; z++) {
                // Simple height calculation for better performance
                // Using a basic sine wave pattern rather than complex noise
                float height = 5.0f + 3.0f * sin(x * 0.3f) * cos(z * 0.3f);
                int terrainHeight = static_cast<int>(height);
                
                // Only create surface blocks and a couple layers beneath
                // This significantly reduces the number of voxels
                for (int y = terrainHeight - 2; y <= terrainHeight; y++) {
                    // Skip if below minimum height
                    if (y < 0) continue;
                    
                    Voxel voxel;
                    if (y == terrainHeight) {
                        // Top layer is grass
                        voxel.type = VoxelType::Grass;
                        voxel.color = QColor(34, 139, 34); // Forest green
                    } else {
                        // Lower layers are dirt
                        voxel.type = VoxelType::Dirt;
                        voxel.color = QColor(139, 69, 19); // Saddle brown
                    }
                    
                    // Set the voxel
                    m_world->setVoxel(x, y, z, voxel);
                }
            }
        }
        
        // Place the player spawn point
        int spawnX = 0;
        int spawnZ = 0;
        float height = 5.0f + 3.0f * sin(spawnX * 0.3f) * cos(spawnZ * 0.3f);
        int spawnHeight = static_cast<int>(height);
        
        // Create a small platform at the spawn point for visibility
        for (int dx = -1; dx <= 1; dx++) {
            for (int dz = -1; dz <= 1; dz++) {
                Voxel voxel;
                voxel.type = VoxelType::Cobblestone;
                voxel.color = QColor(200, 200, 200); // Light gray
                m_world->setVoxel(spawnX + dx, spawnHeight, spawnZ + dz, voxel);
            }
        }
        
        // Force the renderer to update
        if (m_renderer) {
            m_renderer->updateRenderData();
        }
        
        // Update the game scene with the new voxels
        updateGameScene();
        
        emit worldChanged();
        qDebug() << "Default world created successfully";
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

float VoxelSystemIntegration::getSurfaceHeightAt(float x, float z) const
{
    try {
        // If world system exists and has surface height method, use it
        if (m_worldSystem) {
            try {
                return m_worldSystem->getSurfaceHeightAt(x, z);
            }
            catch (const std::exception& e) {
                qWarning() << "Exception in VoxelWorldSystem::getSurfaceHeightAt:" << e.what();
            }
            catch (...) {
                qWarning() << "Unknown exception in VoxelWorldSystem::getSurfaceHeightAt";
            }
        }
        
        // If we don't have a world system (or the method implementation),
        // try to find the surface using raycasting
        if (m_world) {
            try {
                // Start from a high point and raycast down
                QVector3D origin(x, 100.0f, z);  // Start high up
                QVector3D direction(0, -1, 0);   // Look straight down
                float maxDistance = 200.0f;      // Look up to 200 units down
                
                QVector3D hitPos;
                QVector3D hitNormal;
                Voxel hitVoxel;
                
                // Perform the raycast
                if (const_cast<VoxelSystemIntegration*>(this)->raycast(origin, direction, maxDistance, hitPos, hitNormal, hitVoxel)) {
                    return hitPos.y();
                }
            }
            catch (const std::exception& e) {
                qWarning() << "Exception in raycast for surface height:" << e.what();
            }
            catch (...) {
                qWarning() << "Unknown exception in raycast for surface height";
            }
        }
        
        // Failed to determine surface height
        return -1.0f;
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in VoxelSystemIntegration::getSurfaceHeightAt:" << e.what();
        return -1.0f;
    }
    catch (...) {
        qWarning() << "Unknown exception in VoxelSystemIntegration::getSurfaceHeightAt";
        return -1.0f;
    }
}

void VoxelSystemIntegration::updateGameScene()
{
    // Safety check
    if (!m_gameScene) {
        return;
    }

    // Use a static variable to track if we've already updated the scene
    // This prevents continuous updates that can cause performance issues
    static bool initialTerrainCreated = false;
    
    // Don't update if we've already created the terrain
    // This is a temporary fix to prevent the game from becoming unresponsive
    if (initialTerrainCreated) {
        // Only allow updates when explicitly triggered for voxel placement/removal
        static int updateCounter = 0;
        if (updateCounter++ % 60 != 0) { // Only update 1/60 of the time
            return;
        }
    }

    try {
        qDebug() << "Updating game scene with voxels...";
        
        // Flag to track if any voxel system provided voxels
        bool voxelsAdded = false;
        int entityCount = 0;
        
        // Fall back to basic voxel world if needed - simpler approach first
        if (m_world) {
            try {
                qDebug() << "Using basic voxel world for voxels";
                entityCount = 0;
                
                // Get all visible voxels
                QVector<VoxelPos> visibleVoxels = m_world->getVisibleVoxels();
                int maxVoxels = 500; // Limit to prevent performance issues
                
                qDebug() << "Found " << visibleVoxels.size() << " visible voxels in basic voxel world";
                qDebug() << "Will add up to " << maxVoxels << " voxels for performance";
                
                // Add voxel entities to the game scene (limited number)
                int processedVoxels = 0;
                for (const VoxelPos& pos : visibleVoxels) {
                    // Limit the number of voxels we process
                    if (processedVoxels++ >= maxVoxels) {
                        break;
                    }
                    
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
                    try {
                        m_gameScene->addEntity(entity);
                        entityCount++;
                    } catch (...) {
                        // Entity might already exist, ignore
                    }
                }
                
                if (entityCount > 0) {
                    qDebug() << "Added" << entityCount << "voxel entities to game scene from basic voxel world";
                    voxelsAdded = true;
                    initialTerrainCreated = true;
                }
            } catch (const std::exception& e) {
                qWarning() << "Exception using basic voxel world:" << e.what();
            } catch (...) {
                qWarning() << "Unknown exception using basic voxel world";
            }
        }
        
        // If no voxels were added, create a flat floor as fallback
        if (!voxelsAdded && !initialTerrainCreated) {
            qDebug() << "Creating basic floor as fallback...";
            entityCount = 0;
            
            // Create a simple floor - smaller size for performance
            int size = 8;
            for (int x = -size; x <= size; x++) {
                for (int z = -size; z <= size; z++) {
                    QString entityId = QString("voxel_%1_%2_%3").arg(x).arg(0).arg(z);
                    
                    GameEntity entity;
                    entity.id = entityId;
                    entity.type = "voxel";
                    entity.position = QVector3D(x + 0.5f, 0.5f, z + 0.5f);
                    entity.dimensions = QVector3D(1.0f, 1.0f, 1.0f);
                    entity.isStatic = true;
                    
                    try {
                        m_gameScene->addEntity(entity);
                        entityCount++;
                    } catch (...) {
                        // Entity might already exist, ignore
                    }
                }
            }
            
            qDebug() << "Added" << entityCount << "basic floor voxels to game scene";
            initialTerrainCreated = true;
        }
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in updateGameScene:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception in updateGameScene";
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
    qDebug() << "Connecting VoxelSystemIntegration signals...";
    
    // Connect world signals to renderer
    if (m_world && m_renderer) {
        try {
            // Connect with UniqueConnection to avoid duplicate connections
            QObject::connect(m_world, &VoxelWorld::worldChanged, 
                    m_renderer, &VoxelRenderer::updateRenderData, Qt::UniqueConnection);
            QObject::connect(m_world, &VoxelWorld::worldChanged, 
                    this, &VoxelSystemIntegration::updateGameScene, Qt::UniqueConnection);
            qDebug() << "Connected VoxelWorld signals to renderer";
        } catch (const std::exception& e) {
            qWarning() << "Exception connecting VoxelWorld signals:" << e.what();
        }
    } else {
        qWarning() << "Could not connect VoxelWorld signals - world or renderer is null";
    }
    
    // Connect world system signals if available
    if (m_worldSystem) {
        try {
            QObject::connect(m_worldSystem.get(), &VoxelWorldSystem::chunkLoaded, 
                    this, &VoxelSystemIntegration::chunkLoaded, Qt::UniqueConnection);
            QObject::connect(m_worldSystem.get(), &VoxelWorldSystem::chunkUnloaded, 
                    this, &VoxelSystemIntegration::chunkUnloaded, Qt::UniqueConnection);
            QObject::connect(m_worldSystem.get(), &VoxelWorldSystem::chunkModified, 
                    this, &VoxelSystemIntegration::updateGameScene, Qt::UniqueConnection);
            
            // Connect to renderer directly as well
            if (m_renderer) {
                QObject::connect(m_worldSystem.get(), &VoxelWorldSystem::chunkModified, 
                        m_renderer, &VoxelRenderer::updateRenderData, Qt::UniqueConnection);
            }
            
            // Connect memory usage signal
            QObject::connect(m_worldSystem.get(), &VoxelWorldSystem::memoryUsageChanged, 
                    this, // Context for the lambda
                    [this](size_t usage, size_t maxUsage) {
                        qDebug() << "Voxel memory usage:" << usage / (1024 * 1024) << "MB of" 
                                << maxUsage / (1024 * 1024) << "MB";
                    }, 
                    Qt::UniqueConnection);
            
            qDebug() << "Connected VoxelWorldSystem signals";
        } catch (const std::exception& e) {
            qWarning() << "Exception connecting VoxelWorldSystem signals:" << e.what();
        }
    } else {
        qDebug() << "No VoxelWorldSystem available to connect signals";
    }
    
    qDebug() << "Signal connections complete";
}