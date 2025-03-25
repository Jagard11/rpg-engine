// src/arena/voxels/world/voxel_world_system.cpp
#include "../../../../include/arena/voxels/world/voxel_world_system.h"
#include <QDebug>

VoxelWorldSystem::VoxelWorldSystem(QObject* parent)
    : QObject(parent),
      m_worldType(WorldType::Flat),
      m_worldSeed(0),
      m_planetRadius(1000.0f) {
    
    // Create the chunk manager
    m_chunkManager = std::make_unique<ChunkManager>(this);
    
    // Connect signals
    setupSignalConnections();
}

VoxelWorldSystem::~VoxelWorldSystem() {
    // Save all modified chunks before shutdown
    saveAll();
}

bool VoxelWorldSystem::initialize(WorldType worldType, unsigned int seed) {
    // Store world parameters
    m_worldType = worldType;
    m_worldSeed = seed;
    
    // Create the appropriate generator based on world type
    switch (worldType) {
        case WorldType::Flat: {
            auto generator = std::make_shared<FlatTerrainGenerator>();
            generator->setSeed(seed);
            m_chunkGenerator = generator;
            break;
        }
        case WorldType::Hills: {
            auto generator = std::make_shared<NoiseTerrainGenerator>();
            generator->setSeed(seed);
            m_chunkGenerator = generator;
            break;
        }
        case WorldType::Spherical: {
            auto generator = std::make_shared<SphericalPlanetGenerator>();
            generator->setSeed(seed);
            generator->setRadius(m_planetRadius);
            generator->setTerrainHeight(50.0f);
            m_chunkGenerator = generator;
            break;
        }
        case WorldType::Improved: {
            auto generator = std::make_shared<ImprovedTerrainGenerator>();
            generator->setSeed(seed);
            m_chunkGenerator = generator;
            break;
        }
        default:
            qWarning() << "Unknown world type:" << static_cast<int>(worldType);
            return false;
    }
    
    // Set the generator in the chunk manager
    m_chunkManager->setChunkGenerator(m_chunkGenerator);
    
    // Initialize system with origin chunks
    QVector3D origin(0, 0, 0);
    updateAroundViewer(origin);
    
    return true;
}

Voxel VoxelWorldSystem::getVoxel(float x, float y, float z) const {
    // Pass through to chunk manager
    return m_chunkManager->getVoxel(x, y, z);
}

bool VoxelWorldSystem::setVoxel(float x, float y, float z, const Voxel& voxel) {
    // Pass through to chunk manager
    return m_chunkManager->setVoxel(x, y, z, voxel);
}

void VoxelWorldSystem::updateAroundViewer(const QVector3D& viewerPosition) {
    // Spherical worlds need special handling
    if (m_worldType == WorldType::Spherical) {
        // For spherical worlds, we need to update chunks based on the player's position
        // relative to the planet's surface
        
        // Calculate distance from center
        float distanceFromCenter = viewerPosition.length();
        
        // Calculate direction from center (normalized)
        QVector3D directionFromCenter = viewerPosition.normalized();
        
        // Adjust viewer position to be on the surface if far away
        QVector3D adjustedPosition = viewerPosition;
        
        if (distanceFromCenter > m_planetRadius * 1.5f || distanceFromCenter < m_planetRadius * 0.8f) {
            // Player is far from surface, adjust position to be on surface
            adjustedPosition = directionFromCenter * m_planetRadius;
            qDebug() << "Adjusting viewer position for spherical world" << viewerPosition << "->" << adjustedPosition;
        }
        
        // Update chunks around the adjusted position
        m_chunkManager->updateChunksAroundPoint(adjustedPosition);
    }
    else {
        // For flat or hilly worlds, just update chunks around the viewer
        m_chunkManager->updateChunksAroundPoint(viewerPosition);
    }
}

std::vector<QVector3D> VoxelWorldSystem::getVisibleVoxelsInChunk(const ChunkCoordinate& chunkCoord) const {
    // Get the chunk
    std::shared_ptr<Chunk> chunk = m_chunkManager->getChunk(chunkCoord);
    if (!chunk) {
        return {}; // Empty vector if chunk not loaded
    }
    
    // Get visible voxels in local coordinates
    std::vector<VoxelPos> localPositions = chunk->getVisibleVoxels();
    
    // Convert to world coordinates
    std::vector<QVector3D> worldPositions;
    worldPositions.reserve(localPositions.size());
    
    for (const VoxelPos& pos : localPositions) {
        worldPositions.push_back(chunkCoord.toWorldPosition(pos.x, pos.y, pos.z));
    }
    
    return worldPositions;
}

std::vector<ChunkCoordinate> VoxelWorldSystem::getLoadedChunks() const {
    return m_chunkManager->getLoadedChunks();
}

bool VoxelWorldSystem::isChunkLoaded(const ChunkCoordinate& coord) const {
    return m_chunkManager->isChunkLoaded(coord);
}

bool VoxelWorldSystem::forceLoadChunk(const ChunkCoordinate& coord) {
    return m_chunkManager->forceLoadChunk(coord);
}

bool VoxelWorldSystem::raycast(const QVector3D& origin, const QVector3D& direction, float maxDistance,
                             QVector3D& outHitPos, QVector3D& outHitNormal, Voxel& outVoxel,
                             ChunkCoordinate& outHitChunk) const {
    // Initialize variables
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
    
    // Special handling for spherical worlds
    if (m_worldType == WorldType::Spherical) {
        // For spherical worlds, we need to raytrace through a sphere
        float a = direction.lengthSquared();
        float b = 2.0f * QVector3D::dotProduct(origin, direction);
        float c = origin.lengthSquared() - m_planetRadius * m_planetRadius;
        
        // Calculate discriminant
        float discriminant = b * b - 4 * a * c;
        
        if (discriminant < 0) {
            // No intersection with planet
            return false;
        }
        
        // Calculate intersection points
        float t1 = (-b - sqrt(discriminant)) / (2 * a);
        float t2 = (-b + sqrt(discriminant)) / (2 * a);
        
        // Check if intersections are within range
        if (t1 > maxDistance && t2 > maxDistance) {
            return false;
        }
        
        // Use the closest positive intersection
        float t = (t1 > 0) ? t1 : t2;
        
        if (t < 0) {
            return false; // Both intersections behind ray
        }
        
        // Calculate hit position
        QVector3D hitPos = origin + direction * t;
        
        // Get the containing chunk
        ChunkCoordinate hitChunk = ChunkCoordinate::fromWorldPosition(hitPos);
        
        // Check if chunk is loaded
        if (!isChunkLoaded(hitChunk)) {
            return false;
        }
        
        // Calculate local coordinates
        int localX = static_cast<int>(hitPos.x()) % ChunkCoordinate::CHUNK_SIZE;
        int localY = static_cast<int>(hitPos.y()) % ChunkCoordinate::CHUNK_SIZE;
        int localZ = static_cast<int>(hitPos.z()) % ChunkCoordinate::CHUNK_SIZE;
        
        // Handle negative coordinates
        if (localX < 0) localX += ChunkCoordinate::CHUNK_SIZE;
        if (localY < 0) localY += ChunkCoordinate::CHUNK_SIZE;
        if (localZ < 0) localZ += ChunkCoordinate::CHUNK_SIZE;
        
        // Get the voxel
        std::shared_ptr<Chunk> chunk = m_chunkManager->getChunk(hitChunk);
        if (!chunk) {
            return false;
        }
        
        Voxel voxel = chunk->getVoxel(localX, localY, localZ);
        
        // Skip air voxels
        if (voxel.type == VoxelType::Air) {
            return false;
        }
        
        // Fill out results
        outHitPos = hitPos;
        outHitNormal = hitPos.normalized(); // Normal points outward from center
        outVoxel = voxel;
        outHitChunk = hitChunk;
        
        return true;
    }
    
    // Standard ray marching for flat and hilly worlds
    while (distance < maxDistance) {
        // Check current voxel
        Voxel voxel = getVoxel(x, y, z);
        
        // If not air, we hit something
        if (voxel.type != VoxelType::Air) {
            // Calculate exact hit point
            QVector3D hitPoint = origin + direction * distance;
            
            // Calculate which chunk contains this voxel
            ChunkCoordinate hitChunk = ChunkCoordinate::fromWorldPosition(QVector3D(x, y, z));
            
            // Fill out results
            outHitPos = hitPoint;
            outVoxel = voxel;
            outHitChunk = hitChunk;
            
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

void VoxelWorldSystem::saveAll() {
    // Pass through to chunk manager
    m_chunkManager->saveAllChunks();
}

void VoxelWorldSystem::setupSignalConnections() {
    // Connect ChunkManager signals to our signals
    connect(m_chunkManager.get(), &ChunkManager::chunkLoaded, 
            this, &VoxelWorldSystem::chunkLoaded);
    
    connect(m_chunkManager.get(), &ChunkManager::chunkUnloaded, 
            this, &VoxelWorldSystem::chunkUnloaded);
    
    connect(m_chunkManager.get(), &ChunkManager::chunkModified, 
            this, &VoxelWorldSystem::chunkModified);
    
    connect(m_chunkManager.get(), &ChunkManager::memoryUsageChanged, 
            this, &VoxelWorldSystem::memoryUsageChanged);
}

float VoxelWorldSystem::getSurfaceHeightAt(float x, float z) const {
    try {
        // Check if we have a valid chunk generator
        if (!m_chunkGenerator) {
            qWarning() << "Cannot get surface height: No chunk generator available";
            return -1.0f;
        }
        
        // If using improved generator, use its direct surface height function
        if (m_worldType == WorldType::Improved) {
            try {
                auto* improvedGenerator = dynamic_cast<ImprovedTerrainGenerator*>(m_chunkGenerator.get());
                if (improvedGenerator) {
                    return improvedGenerator->getSurfaceHeightAt(x, z);
                }
                else {
                    qWarning() << "Cannot get surface height: Failed to cast to ImprovedTerrainGenerator";
                }
            }
            catch (const std::exception& e) {
                qWarning() << "Exception in ImprovedTerrainGenerator::getSurfaceHeightAt:" << e.what();
            }
            catch (...) {
                qWarning() << "Unknown exception in ImprovedTerrainGenerator::getSurfaceHeightAt";
            }
        }
        
        // For other world types, use raycasting from above
        try {
            QVector3D origin(x, 100.0f, z);  // Start high up
            QVector3D direction(0, -1, 0);   // Look straight down
            float maxDistance = 200.0f;      // Look up to 200 units down
            
            QVector3D hitPos;
            QVector3D hitNormal;
            Voxel hitVoxel;
            ChunkCoordinate hitChunk;
            
            // Perform the raycast
            if (raycast(origin, direction, maxDistance, hitPos, hitNormal, hitVoxel, hitChunk)) {
                return hitPos.y();
            }
            else {
                qDebug() << "No surface found at position (" << x << ", " << z << ") with raycast";
            }
        }
        catch (const std::exception& e) {
            qWarning() << "Exception in raycast for surface height:" << e.what();
        }
        catch (...) {
            qWarning() << "Unknown exception in raycast for surface height";
        }
        
        // Failed to determine surface height
        return -1.0f;
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in VoxelWorldSystem::getSurfaceHeightAt:" << e.what();
        return -1.0f;
    }
    catch (...) {
        qWarning() << "Unknown exception in VoxelWorldSystem::getSurfaceHeightAt";
        return -1.0f;
    }
}