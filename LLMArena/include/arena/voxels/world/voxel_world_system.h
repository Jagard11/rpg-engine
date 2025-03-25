// include/arena/voxels/world/voxel_world_system.h
#ifndef VOXEL_WORLD_SYSTEM_H
#define VOXEL_WORLD_SYSTEM_H

#include <QObject>
#include <memory>

#include "../chunk/chunk_manager.h"
#include "../chunk/chunk_generator.h"

/**
 * @brief Main class for managing a voxel-based world.
 * 
 * This class is the high-level interface for interacting with the voxel world.
 * It manages the chunk system, terrain generation, and world configuration.
 */
class VoxelWorldSystem : public QObject {
    Q_OBJECT
    
public:
    /**
     * @brief World generation type
     */
    enum class WorldType {
        Flat,       // Flat world with simple terrain
        Hills,      // Rolling hills with noise-based terrain
        Spherical,  // Globe-shaped world (planet)
        Improved    // Enhanced procedural terrain generation
    };
    
    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit VoxelWorldSystem(QObject* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~VoxelWorldSystem();
    
    /**
     * @brief Initialize the world
     * @param worldType Type of world to generate
     * @param seed Random seed for generation
     * @return True if successful
     */
    bool initialize(WorldType worldType, unsigned int seed);
    
    /**
     * @brief Get a voxel at a specific world position
     * @param x World X coordinate
     * @param y World Y coordinate
     * @param z World Z coordinate
     * @return The voxel at the specified position
     */
    Voxel getVoxel(float x, float y, float z) const;
    
    /**
     * @brief Set a voxel at a specific world position
     * @param x World X coordinate
     * @param y World Y coordinate
     * @param z World Z coordinate
     * @param voxel The voxel to set
     * @return True if the voxel was changed
     */
    bool setVoxel(float x, float y, float z, const Voxel& voxel);
    
    /**
     * @brief Update the world around a viewer position (typically the player)
     * @param viewerPosition World position of the viewer
     */
    void updateAroundViewer(const QVector3D& viewerPosition);
    
    /**
     * @brief Get all visible voxel positions in a chunk
     * @param chunkCoord Chunk coordinate
     * @return Vector of visible voxel positions in world coordinates
     */
    std::vector<QVector3D> getVisibleVoxelsInChunk(const ChunkCoordinate& chunkCoord) const;
    
    /**
     * @brief Get all loaded chunk coordinates
     * @return Vector of chunk coordinates
     */
    std::vector<ChunkCoordinate> getLoadedChunks() const;
    
    /**
     * @brief Check if a chunk is loaded
     * @param coord Chunk coordinate
     * @return True if the chunk is loaded
     */
    bool isChunkLoaded(const ChunkCoordinate& coord) const;
    
    /**
     * @brief Get the world type
     * @return The world type
     */
    WorldType getWorldType() const { return m_worldType; }
    
    /**
     * @brief Get the world seed
     * @return The world seed
     */
    unsigned int getWorldSeed() const { return m_worldSeed; }
    
    /**
     * @brief Force a specific chunk to be loaded
     * @param coord Chunk coordinate
     * @return True if successful
     */
    bool forceLoadChunk(const ChunkCoordinate& coord);
    
    /**
     * @brief Cast a ray through the world and find the first voxel hit
     * @param origin Ray origin
     * @param direction Ray direction (normalized)
     * @param maxDistance Maximum ray distance
     * @param outHitPos Output parameter for hit position
     * @param outHitNormal Output parameter for hit normal
     * @param outVoxel Output parameter for hit voxel
     * @param outHitChunk Output parameter for hit chunk coordinate
     * @return True if the ray hit something
     */
    bool raycast(const QVector3D& origin, const QVector3D& direction, float maxDistance,
                QVector3D& outHitPos, QVector3D& outHitNormal, Voxel& outVoxel,
                ChunkCoordinate& outHitChunk) const;
    
    /**
     * @brief Save all modified chunks
     */
    void saveAll();
    
    /**
     * @brief Get the planet radius (for spherical worlds)
     * @return The planet radius in blocks
     */
    float getPlanetRadius() const { return m_planetRadius; }
    
    /**
     * @brief Get the surface height at a specific X,Z coordinate
     * @param x X coordinate in world space
     * @param z Z coordinate in world space
     * @return The Y coordinate of the surface at the specified X,Z position
     */
    float getSurfaceHeightAt(float x, float z) const;
    
    /**
     * @brief Get the chunk manager
     * @return The chunk manager
     */
    ChunkManager* getChunkManager() const { return m_chunkManager.get(); }
    
signals:
    /**
     * @brief Signal emitted when a chunk is loaded
     * @param coord Chunk coordinate
     */
    void chunkLoaded(const ChunkCoordinate& coord);
    
    /**
     * @brief Signal emitted when a chunk is unloaded
     * @param coord Chunk coordinate
     */
    void chunkUnloaded(const ChunkCoordinate& coord);
    
    /**
     * @brief Signal emitted when a chunk is modified
     * @param coord Chunk coordinate
     */
    void chunkModified(const ChunkCoordinate& coord);
    
    /**
     * @brief Signal emitted when memory usage changes
     * @param usage Current memory usage in bytes
     * @param maxUsage Maximum memory usage in bytes
     */
    void memoryUsageChanged(size_t usage, size_t maxUsage);
    
private:
    // The chunk manager
    std::unique_ptr<ChunkManager> m_chunkManager;
    
    // The current world generator
    std::shared_ptr<ChunkGenerator> m_chunkGenerator;
    
    // World parameters
    WorldType m_worldType;
    unsigned int m_worldSeed;
    
    // For spherical worlds
    float m_planetRadius;
    
    // Connection methods
    void setupSignalConnections();
};

#endif // VOXEL_WORLD_SYSTEM_Hwww