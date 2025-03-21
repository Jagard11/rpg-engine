// include/arena/voxels/chunk/chunk_manager.h
#ifndef CHUNK_MANAGER_H
#define CHUNK_MANAGER_H

#include <memory>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <vector>
#include <atomic>
#include <QObject>
#include <QThread>
#include <QTimer>
#include <QVector3D>

#include "chunk.h"
#include "chunk_generator.h"

// Forward declaration
class ChunkGenerator;

/**
 * @brief Manages loading, unloading, and access to chunks.
 * 
 * The ChunkManager is responsible for loading chunks into memory as needed,
 * unloading chunks that are no longer needed, and providing access to chunks.
 * It also handles chunk generation and serialization.
 */
class ChunkManager : public QObject {
    Q_OBJECT
    
public:
    /**
     * @brief Construct a chunk manager
     * @param parent Parent QObject
     * @param maxMemory Maximum memory usage in bytes (default is 256 MB)
     * @param viewDistance Maximum view distance in chunks (default is 8)
     */
    ChunkManager(QObject* parent = nullptr, 
                 size_t maxMemory = 256 * 1024 * 1024, 
                 int viewDistance = 8);
    
    /**
     * @brief Destructor
     */
    ~ChunkManager();
    
    /**
     * @brief Get a chunk by coordinate
     * @param coordinate Chunk coordinate
     * @return Shared pointer to the chunk, or nullptr if not loaded
     */
    std::shared_ptr<Chunk> getChunk(const ChunkCoordinate& coordinate);
    
    /**
     * @brief Check if a chunk is loaded
     * @param coordinate Chunk coordinate
     * @return True if loaded, false otherwise
     */
    bool isChunkLoaded(const ChunkCoordinate& coordinate) const;
    
    /**
     * @brief Get a voxel by world position
     * @param worldX World X coordinate
     * @param worldY World Y coordinate
     * @param worldZ World Z coordinate
     * @return The voxel at the specified position, or air if not loaded
     */
    Voxel getVoxel(float worldX, float worldY, float worldZ);
    
    /**
     * @brief Set a voxel by world position
     * @param worldX World X coordinate
     * @param worldY World Y coordinate
     * @param worldZ World Z coordinate
     * @param voxel The voxel to set
     * @return True if the voxel was changed, false otherwise
     */
    bool setVoxel(float worldX, float worldY, float worldZ, const Voxel& voxel);
    
    /**
     * @brief Get all loaded chunks
     * @return Vector of loaded chunk coordinates
     */
    std::vector<ChunkCoordinate> getLoadedChunks() const;
    
    /**
     * @brief Update chunks around a point (typically the player)
     * @param position World position
     */
    void updateChunksAroundPoint(const QVector3D& position);
    
    /**
     * @brief Unload all chunks (e.g. when changing worlds)
     */
    void unloadAllChunks();
    
    /**
     * @brief Forcibly load a specific chunk
     * @param coordinate Chunk coordinate
     * @return True if chunk was loaded or already loaded, false on error
     */
    bool forceLoadChunk(const ChunkCoordinate& coordinate);
    
    /**
     * @brief Forcibly unload a specific chunk
     * @param coordinate Chunk coordinate
     * @return True if chunk was unloaded, false if it wasn't loaded
     */
    bool forceUnloadChunk(const ChunkCoordinate& coordinate);
    
    /**
     * @brief Save all modified chunks
     */
    void saveAllChunks();
    
    /**
     * @brief Get current memory usage
     * @return Memory usage in bytes
     */
    size_t getMemoryUsage() const { return m_currentMemoryUsage; }
    
    /**
     * @brief Get maximum memory usage
     * @return Maximum memory usage in bytes
     */
    size_t getMaxMemoryUsage() const { return m_maxMemoryUsage; }
    
    /**
     * @brief Set maximum memory usage
     * @param maxMemory Maximum memory usage in bytes
     */
    void setMaxMemoryUsage(size_t maxMemory) { m_maxMemoryUsage = maxMemory; }
    
    /**
     * @brief Get view distance
     * @return View distance in chunks
     */
    int getViewDistance() const { return m_viewDistance; }
    
    /**
     * @brief Set view distance
     * @param viewDistance View distance in chunks
     */
    void setViewDistance(int viewDistance) { m_viewDistance = viewDistance; }
    
    /**
     * @brief Set the chunk generator
     * @param generator Chunk generator
     */
    void setChunkGenerator(std::shared_ptr<ChunkGenerator> generator) { m_chunkGenerator = generator; }
    
signals:
    /**
     * @brief Signal emitted when a chunk is loaded
     * @param coordinate Chunk coordinate
     */
    void chunkLoaded(const ChunkCoordinate& coordinate);
    
    /**
     * @brief Signal emitted when a chunk is unloaded
     * @param coordinate Chunk coordinate
     */
    void chunkUnloaded(const ChunkCoordinate& coordinate);
    
    /**
     * @brief Signal emitted when a chunk is modified
     * @param coordinate Chunk coordinate
     */
    void chunkModified(const ChunkCoordinate& coordinate);
    
    /**
     * @brief Signal emitted when memory usage changes significantly
     * @param currentUsage Current memory usage in bytes
     * @param maxUsage Maximum memory usage in bytes
     */
    void memoryUsageChanged(size_t currentUsage, size_t maxUsage);
    
private slots:
    /**
     * @brief Check memory usage and unload chunks if necessary
     */
    void checkMemoryUsage();
    
    /**
     * @brief Process the chunk loading queue
     */
    void processLoadQueue();
    
private:
    // Chunk loading priority queue entry
    struct ChunkLoadEntry {
        ChunkCoordinate coordinate;
        float priority;
        
        bool operator<(const ChunkLoadEntry& other) const {
            // Reverse comparison for max heap (higher priority first)
            return priority < other.priority;
        }
    };
    
    // Map of loaded chunks
    std::unordered_map<ChunkCoordinate, std::shared_ptr<Chunk>> m_chunks;
    
    // Chunk loading priority queue
    std::priority_queue<ChunkLoadEntry> m_loadQueue;
    
    // Most recent player/camera position
    QVector3D m_lastUpdatePosition;
    
    // Memory management
    size_t m_maxMemoryUsage;
    std::atomic<size_t> m_currentMemoryUsage;
    
    // View distance
    int m_viewDistance;
    
    // Chunk generator
    std::shared_ptr<ChunkGenerator> m_chunkGenerator;
    
    // Thread safety
    mutable std::mutex m_chunksMutex;
    mutable std::mutex m_queueMutex;
    
    // Timer for memory checks
    QTimer m_memoryCheckTimer;
    
    // Timer for queue processing
    QTimer m_queueProcessTimer;
    
    // Chunk loading thread
    QThread m_loadThread;
    
    // Helper methods
    void unloadLeastRecentlyUsedChunk();
    void updateMemoryUsage();
    float calculateChunkPriority(const ChunkCoordinate& chunkCoord, const QVector3D& viewerPos) const;
    bool loadChunkFromStorage(const ChunkCoordinate& coordinate);
    bool saveChunkToStorage(const ChunkCoordinate& coordinate);
    std::shared_ptr<Chunk> generateChunk(const ChunkCoordinate& coordinate);
};

#endif // CHUNK_MANAGER_H