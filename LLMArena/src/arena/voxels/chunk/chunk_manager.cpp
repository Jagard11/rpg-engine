// src/arena/voxels/chunk/chunk_manager.cpp
#include "../../../../include/arena/voxels/chunk/chunk_manager.h"
#include <QDebug>
#include <algorithm>
#include <chrono>

ChunkManager::ChunkManager(QObject* parent, size_t maxMemory, int viewDistance)
    : QObject(parent), 
      m_maxMemoryUsage(maxMemory),
      m_currentMemoryUsage(0),
      m_viewDistance(viewDistance),
      m_chunkGenerator(nullptr) {
    
    // Connect timers
    connect(&m_memoryCheckTimer, &QTimer::timeout, this, &ChunkManager::checkMemoryUsage);
    connect(&m_queueProcessTimer, &QTimer::timeout, this, &ChunkManager::processLoadQueue);
    
    // Start timers
    m_memoryCheckTimer.start(5000);  // Check memory every 5 seconds
    m_queueProcessTimer.start(50);   // Process queue every 50ms
}

ChunkManager::~ChunkManager() {
    // Save all modified chunks before shutting down
    saveAllChunks();
    
    // Unload all chunks
    unloadAllChunks();
}

std::shared_ptr<Chunk> ChunkManager::getChunk(const ChunkCoordinate& coordinate) {
    // Lock the chunks map
    std::lock_guard<std::mutex> lock(m_chunksMutex);
    
    // Check if the chunk is already loaded
    auto it = m_chunks.find(coordinate);
    if (it != m_chunks.end()) {
        // Update last access time
        it->second->updateAccessTime();
        return it->second;
    }
    
    // Try to force load the chunk
    if (forceLoadChunk(coordinate)) {
        auto it = m_chunks.find(coordinate);
        if (it != m_chunks.end()) {
            return it->second;
        }
    }
    
    // Chunk couldn't be loaded
    return nullptr;
}

bool ChunkManager::isChunkLoaded(const ChunkCoordinate& coordinate) const {
    // Lock the chunks map
    std::lock_guard<std::mutex> lock(m_chunksMutex);
    
    // Check if the chunk is in the map
    return m_chunks.find(coordinate) != m_chunks.end();
}

Voxel ChunkManager::getVoxel(float worldX, float worldY, float worldZ) {
    // Convert world coordinates to chunk coordinates
    ChunkCoordinate chunkCoord = ChunkCoordinate::fromWorldPosition(QVector3D(worldX, worldY, worldZ));
    
    // Calculate local coordinates within the chunk
    int localX = static_cast<int>(worldX) % ChunkCoordinate::CHUNK_SIZE;
    int localY = static_cast<int>(worldY) % ChunkCoordinate::CHUNK_SIZE;
    int localZ = static_cast<int>(worldZ) % ChunkCoordinate::CHUNK_SIZE;
    
    // Handle negative coordinates
    if (localX < 0) localX += ChunkCoordinate::CHUNK_SIZE;
    if (localY < 0) localY += ChunkCoordinate::CHUNK_SIZE;
    if (localZ < 0) localZ += ChunkCoordinate::CHUNK_SIZE;
    
    // Get the chunk
    std::shared_ptr<Chunk> chunk = getChunk(chunkCoord);
    if (chunk) {
        // Get the voxel
        return chunk->getVoxel(localX, localY, localZ);
    }
    
    // Chunk not loaded, return air
    return Voxel();
}

bool ChunkManager::setVoxel(float worldX, float worldY, float worldZ, const Voxel& voxel) {
    // Convert world coordinates to chunk coordinates
    ChunkCoordinate chunkCoord = ChunkCoordinate::fromWorldPosition(QVector3D(worldX, worldY, worldZ));
    
    // Calculate local coordinates within the chunk
    int localX = static_cast<int>(worldX) % ChunkCoordinate::CHUNK_SIZE;
    int localY = static_cast<int>(worldY) % ChunkCoordinate::CHUNK_SIZE;
    int localZ = static_cast<int>(worldZ) % ChunkCoordinate::CHUNK_SIZE;
    
    // Handle negative coordinates
    if (localX < 0) localX += ChunkCoordinate::CHUNK_SIZE;
    if (localY < 0) localY += ChunkCoordinate::CHUNK_SIZE;
    if (localZ < 0) localZ += ChunkCoordinate::CHUNK_SIZE;
    
    // Get the chunk
    std::shared_ptr<Chunk> chunk = getChunk(chunkCoord);
    if (chunk) {
        // Set the voxel
        bool changed = chunk->setVoxel(localX, localY, localZ, voxel);
        
        // If the voxel was changed, emit the signal
        if (changed) {
            emit chunkModified(chunkCoord);
        }
        
        return changed;
    }
    
    // Chunk not loaded, can't set voxel
    return false;
}

std::vector<ChunkCoordinate> ChunkManager::getLoadedChunks() const {
    // Lock the chunks map
    std::lock_guard<std::mutex> lock(m_chunksMutex);
    
    // Collect all loaded chunk coordinates
    std::vector<ChunkCoordinate> loadedChunks;
    loadedChunks.reserve(m_chunks.size());
    
    for (const auto& entry : m_chunks) {
        loadedChunks.push_back(entry.first);
    }
    
    return loadedChunks;
}

void ChunkManager::updateChunksAroundPoint(const QVector3D& position) {
    // Store the position for memory usage checks
    m_lastUpdatePosition = position;
    
    // Get the chunk coordinate of the point
    ChunkCoordinate centerCoord = ChunkCoordinate::fromWorldPosition(position);
    
    // Lock the queue mutex
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    // Clear the existing queue
    std::priority_queue<ChunkLoadEntry> emptyQueue;
    std::swap(m_loadQueue, emptyQueue);
    
    // Find all chunks within view distance
    std::vector<ChunkCoordinate> chunksToLoad;
    
    // Add chunks in a cube around the center
    // Add an extra layer to ensure overlap for chunk boundaries
    int loadDistance = m_viewDistance + 1;
    
    for (int x = -loadDistance; x <= loadDistance; x++) {
        for (int y = -loadDistance; y <= loadDistance; y++) {
            for (int z = -loadDistance; z <= loadDistance; z++) {
                ChunkCoordinate coord = centerCoord.offset(x, y, z);
                
                // Skip if already loaded
                if (isChunkLoaded(coord)) {
                    continue;
                }
                
                // Calculate priority based on distance
                float priority = calculateChunkPriority(coord, position);
                
                // Give higher priority to chunks adjacent to loaded chunks
                // to ensure proper seamless terrain rendering
                bool isAdjacent = false;
                auto neighbors = coord.getFaceNeighbors();
                for (const auto& neighbor : neighbors) {
                    if (isChunkLoaded(neighbor)) {
                        isAdjacent = true;
                        priority *= 1.5f; // Boost priority for adjacent chunks
                        break;
                    }
                }
                
                // Add to queue
                m_loadQueue.push({coord, priority});
                
                // Add debug output for chunk loading
                if (isAdjacent) {
                    qDebug() << "Prioritized loading of adjacent chunk:" << coord.getX() << coord.getY() << coord.getZ();
                }
            }
        }
    }
}

void ChunkManager::unloadAllChunks() {
    // Lock the chunks map
    std::lock_guard<std::mutex> lock(m_chunksMutex);
    
    // Save all modified chunks
    for (const auto& entry : m_chunks) {
        if (entry.second->isModified()) {
            saveChunkToStorage(entry.first);
        }
        
        // Emit signal
        emit chunkUnloaded(entry.first);
    }
    
    // Clear the map
    m_chunks.clear();
    
    // Reset memory usage
    m_currentMemoryUsage = 0;
}

bool ChunkManager::forceLoadChunk(const ChunkCoordinate& coordinate) {
    // Check if the chunk is already loaded
    if (isChunkLoaded(coordinate)) {
        return true;
    }
    
    // First try to load from storage
    if (loadChunkFromStorage(coordinate)) {
        return true;
    }
    
    // If that fails, try to generate the chunk
    std::shared_ptr<Chunk> chunk = generateChunk(coordinate);
    if (chunk) {
        // Lock the chunks map
        std::lock_guard<std::mutex> lock(m_chunksMutex);
        
        // Add the chunk to the map
        m_chunks[coordinate] = chunk;
        
        // Update memory usage
        m_currentMemoryUsage += chunk->calculateMemoryUsage();
        
        // Emit signal
        emit chunkLoaded(coordinate);
        
        return true;
    }
    
    return false;
}

bool ChunkManager::forceUnloadChunk(const ChunkCoordinate& coordinate) {
    // Lock the chunks map
    std::lock_guard<std::mutex> lock(m_chunksMutex);
    
    // Check if the chunk is loaded
    auto it = m_chunks.find(coordinate);
    if (it == m_chunks.end()) {
        return false;
    }
    
    // Save the chunk if modified
    if (it->second->isModified()) {
        saveChunkToStorage(coordinate);
    }
    
    // Update memory usage
    m_currentMemoryUsage -= it->second->calculateMemoryUsage();
    
    // Remove the chunk
    m_chunks.erase(it);
    
    // Emit signal
    emit chunkUnloaded(coordinate);
    
    return true;
}

void ChunkManager::saveAllChunks() {
    // Lock the chunks map
    std::lock_guard<std::mutex> lock(m_chunksMutex);
    
    // Save all modified chunks
    for (const auto& entry : m_chunks) {
        if (entry.second->isModified()) {
            saveChunkToStorage(entry.first);
        }
    }
}

void ChunkManager::checkMemoryUsage() {
    // Skip if we're below the limit
    if (m_currentMemoryUsage <= m_maxMemoryUsage) {
        return;
    }
    
    // Unload chunks until we're below the limit (minus some margin)
    size_t targetUsage = m_maxMemoryUsage * 0.9; // 90% of max
    
    while (m_currentMemoryUsage > targetUsage) {
        // Unload the least recently used chunk
        unloadLeastRecentlyUsedChunk();
        
        // If we have no more chunks, we're done
        if (m_chunks.empty()) {
            break;
        }
    }
    
    // Emit signal for memory usage change
    emit memoryUsageChanged(m_currentMemoryUsage, m_maxMemoryUsage);
}

void ChunkManager::processLoadQueue() {
    // Lock the queue mutex
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    // Process up to 3 chunks per call
    for (int i = 0; i < 3 && !m_loadQueue.empty(); i++) {
        // Check if we're over the memory limit
        if (m_currentMemoryUsage > m_maxMemoryUsage) {
            break;
        }
        
        // Get the highest priority chunk
        ChunkLoadEntry entry = m_loadQueue.top();
        m_loadQueue.pop();
        
        // Skip if already loaded
        if (isChunkLoaded(entry.coordinate)) {
            continue;
        }
        
        // Try to load the chunk
        forceLoadChunk(entry.coordinate);
    }
}

void ChunkManager::unloadLeastRecentlyUsedChunk() {
    // Lock the chunks map
    std::lock_guard<std::mutex> lock(m_chunksMutex);
    
    // Find the least recently used chunk
    ChunkCoordinate lruCoord;
    QDateTime oldestTime = QDateTime::currentDateTime();
    
    for (const auto& entry : m_chunks) {
        if (entry.second->getLastAccessTime() < oldestTime) {
            oldestTime = entry.second->getLastAccessTime();
            lruCoord = entry.first;
        }
    }
    
    // Unload the chunk if found
    if (oldestTime < QDateTime::currentDateTime()) {
        forceUnloadChunk(lruCoord);
    }
}

void ChunkManager::updateMemoryUsage() {
    // Lock the chunks map
    std::lock_guard<std::mutex> lock(m_chunksMutex);
    
    // Recalculate memory usage
    size_t usage = 0;
    for (const auto& entry : m_chunks) {
        usage += entry.second->calculateMemoryUsage();
    }
    
    // Update the member variable
    m_currentMemoryUsage = usage;
    
    // Emit signal if there's a significant change
    static size_t lastReportedUsage = 0;
    if (std::abs(static_cast<long long>(usage) - static_cast<long long>(lastReportedUsage)) > m_maxMemoryUsage / 20) {
        lastReportedUsage = usage;
        emit memoryUsageChanged(usage, m_maxMemoryUsage);
    }
}

float ChunkManager::calculateChunkPriority(const ChunkCoordinate& chunkCoord, const QVector3D& viewerPos) const {
    // Calculate distance from viewer to chunk center
    QVector3D chunkCenter = chunkCoord.getCenter();
    float distance = (chunkCenter - viewerPos).length();
    
    // Priority is inversely proportional to distance
    float priority = 1000.0f / (1.0f + distance);
    
    // Add random jitter to break ties (0.0 - 0.1)
    float jitter = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 0.1f;
    priority += jitter;
    
    return priority;
}

bool ChunkManager::loadChunkFromStorage(const ChunkCoordinate& coordinate) {
    // TODO: Implement chunk loading from disk
    // For now, return false to indicate loading failed
    return false;
}

bool ChunkManager::saveChunkToStorage(const ChunkCoordinate& coordinate) {
    // TODO: Implement chunk saving to disk
    // For now, just mark the chunk as unmodified
    auto it = m_chunks.find(coordinate);
    if (it != m_chunks.end()) {
        it->second->setModified(false);
        return true;
    }
    
    return false;
}

std::shared_ptr<Chunk> ChunkManager::generateChunk(const ChunkCoordinate& coordinate) {
    // Check if we have a generator
    if (!m_chunkGenerator) {
        qWarning() << "No chunk generator configured";
        
        // Create a basic empty chunk
        auto chunk = std::make_shared<Chunk>(coordinate);
        return chunk;
    }
    
    // Use the generator to create the chunk
    return m_chunkGenerator->generateChunk(coordinate);
}