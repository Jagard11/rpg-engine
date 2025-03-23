// src/arena/voxels/voxel_system_integration.cpp
#include "../../../include/arena/voxels/voxel_system_integration.h"
#include "../../../include/arena/voxels/voxel_world.h"
#include "../../../include/arena/voxels/voxel_renderer.h"
#include "../../../include/arena/voxels/culling/view_frustum.h"
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

void VoxelSystemIntegration::createSphericalPlanet(float radius, float terrainHeight, unsigned int seed) {
    if (!m_worldSystem) {
        qCritical() << "Cannot create spherical planet: world system not initialized";
        return;
    }
    
    try {
        // Initialize the world system with spherical planet settings
        m_worldSystem->initialize(VoxelWorldSystem::WorldType::Spherical, seed);
        
        // Set the planet parameters on the generator if needed
        // (This would need additional implementation if not handled in initialize)
        
        // Update the game scene to match the voxel world
        QTimer::singleShot(0, this, [this]() {
            updateGameScene();
        });
    } catch (const std::exception& e) {
        qCritical() << "Failed to create spherical planet:" << e.what();
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

// The missing function implementation
void VoxelSystemIntegration::streamChunksAroundPlayer(const QVector3D& playerPosition) {
    if (!m_worldSystem) {
        qWarning() << "Cannot stream chunks: World system not initialized";
        return;
    }
    
    try {
        // Update chunks around the player's position
        m_worldSystem->updateAroundViewer(playerPosition);
    } catch (const std::exception& e) {
        qCritical() << "Exception in streamChunksAroundPlayer:" << e.what();
    }
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

// Raycast from point in direction
bool VoxelSystemIntegration::raycast(const QVector3D& origin, const QVector3D& direction, float maxDistance,
                QVector3D& outHitPos, QVector3D& outHitNormal, Voxel& outVoxel) {
    if (m_worldSystem) {
        // Use world system for raycasting if available
        ChunkCoordinate hitChunk;
        return m_worldSystem->raycast(origin, direction, maxDistance, 
                                    outHitPos, outHitNormal, outVoxel, hitChunk);
    }
    
    // Fallback to simple raycasting if world system isn't available
    // This is a simplified version and won't work for large worlds
    float distance = 0.0f;
    
    // Voxel position we're examining
    int x = std::floor(origin.x());
    int y = std::floor(origin.y());
    int z = std::floor(origin.z());
    
    // Direction of movement for each axis
    int stepX = (direction.x() >= 0) ? 1 : -1;
    int stepY = (direction.y() >= 0) ? 1 : -1;
    int stepZ = (direction.z() >= 0) ? 1 : -1;
    
    // Distance to next boundary
    float nextBoundaryX = (stepX > 0) ? (x + 1) : x;
    float nextBoundaryY = (stepY > 0) ? (y + 1) : y;
    float nextBoundaryZ = (stepZ > 0) ? (z + 1) : z;
    
    // Parameter values for next boundaries
    float tMaxX = (direction.x() != 0) ? (nextBoundaryX - origin.x()) / direction.x() : std::numeric_limits<float>::max();
    float tMaxY = (direction.y() != 0) ? (nextBoundaryY - origin.y()) / direction.y() : std::numeric_limits<float>::max();
    float tMaxZ = (direction.z() != 0) ? (nextBoundaryZ - origin.z()) / direction.z() : std::numeric_limits<float>::max();
    
    // Parameter increments
    float tDeltaX = (direction.x() != 0) ? stepX / direction.x() : std::numeric_limits<float>::max();
    float tDeltaY = (direction.y() != 0) ? stepY / direction.y() : std::numeric_limits<float>::max();
    float tDeltaZ = (direction.z() != 0) ? stepZ / direction.z() : std::numeric_limits<float>::max();
    
    // Face index for collision
    int face = -1;
    
    // Loop until we hit something or exceed max distance
    while (distance < maxDistance) {
        // Check current voxel
        outVoxel = m_world->getVoxel(x, y, z);
        
        // If not air, we hit something
        if (outVoxel.type != VoxelType::Air) {
            // Calculate exact hit point
            outHitPos = origin + direction * distance;
            
            // Set normal based on face
            switch (face) {
                case 0: outHitNormal = QVector3D(1, 0, 0); break;  // +X
                case 1: outHitNormal = QVector3D(-1, 0, 0); break; // -X
                case 2: outHitNormal = QVector3D(0, 1, 0); break;  // +Y
                case 3: outHitNormal = QVector3D(0, -1, 0); break; // -Y
                case 4: outHitNormal = QVector3D(0, 0, 1); break;  // +Z
                case 5: outHitNormal = QVector3D(0, 0, -1); break; // -Z
            }
            
            return true;
        }
        
        // Move to next voxel
        if (tMaxX < tMaxY && tMaxX < tMaxZ) {
            // X axis crossing
            distance = tMaxX;
            tMaxX += tDeltaX;
            x += stepX;
            face = (stepX > 0) ? 1 : 0; // -X or +X face
        } else if (tMaxY < tMaxZ) {
            // Y axis crossing
            distance = tMaxY;
            tMaxY += tDeltaY;
            y += stepY;
            face = (stepY > 0) ? 3 : 2; // -Y or +Y face
        } else {
            // Z axis crossing
            distance = tMaxZ;
            tMaxZ += tDeltaZ;
            z += stepZ;
            face = (stepZ > 0) ? 5 : 4; // -Z or +Z face
        }
    }
    
    // No hit
    return false;
}

// Place voxel at hit position offset by normal
bool VoxelSystemIntegration::placeVoxel(const QVector3D& hitPos, const QVector3D& normal, const Voxel& voxel) {
    // Calculate position for new voxel
    QVector3D newPos = hitPos + normal;
    
    // Place voxel (void return type, so we just call it and return true)
    m_world->setVoxel(newPos.x(), newPos.y(), newPos.z(), voxel);
    return true;
}

// Remove voxel at hit position
bool VoxelSystemIntegration::removeVoxel(const QVector3D& hitPos) {
    // Remove voxel by setting it to air
    m_world->setVoxel(hitPos.x(), hitPos.y(), hitPos.z(), Voxel());
    return true;
}