// Visibility management methods
void updateVisibleChunks(const glm::vec3& playerPos, const glm::vec3& playerForward);
void markChunkVisible(const glm::ivec3& chunkPos);
void propagateVisibilityDownward(const glm::ivec3& chunkPos);
bool isVisibleFromAbove(const glm::ivec3& chunkPos, const glm::ivec3& playerChunkPos) const;
bool isChunkVisible(const glm::ivec3& chunkPos, const glm::vec3& playerPos, const glm::vec3& playerForward) const;

// New method to completely reset the visible chunks list
void resetVisibleChunks() { 
    size_t oldSize = m_visibleChunks.size();
    m_visibleChunks.clear(); 
    std::cout << "Reset visible chunks list. Removed " << oldSize << " entries." << std::endl;
}

// Flag to control greedy meshing 

// Update all dirty chunk meshes
void updateDirtyChunkMeshes(int maxUpdatesPerFrame);

// Reset all chunk visibility and dirty states periodically
void resetChunkStates();

// Get the number of dirty chunks for diagnostics
int getDirtyChunkCount() const;

private:
    uint64_t m_seed;
    int m_viewDistance;
    bool m_disableGreedyMeshing;
    std::unique_ptr<WorldGenerator> m_worldGenerator;
    std::unique_ptr<ChunkVisibilityManager> m_visibilityManager;
    std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, Vec3Hash> m_chunks;
    std::unordered_set<glm::ivec3, Vec3Hash> m_visibleChunks;
    std::deque<glm::ivec3> m_chunksToLoadQueue;
    std::deque<glm::ivec3> m_chunksToUnloadQueue;
    std::deque<ModifiedBlock> m_recentlyModifiedBlocks;
    int m_pendingChunkOperations;
    int m_maxSimultaneousChunksLoaded;
    bool m_initialized;
    
    // Maximum number of visible chunks to render
    size_t m_maxVisibleChunks; 