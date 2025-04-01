class ChunkVisibilityManager {
public:
    ChunkVisibilityManager(World* world);
    ~ChunkVisibilityManager();
    
    void updateChunkVisibility(const Player& player);
    
    // Added this method declaration to fix the linter error
    bool shouldSkipChunk(const glm::ivec3& chunkPos);
    
private:
    World* m_world;
    
    bool checkForOpenVoxel(const glm::ivec3& chunkPos);
    bool isAdjacentToOpenVoxel(const glm::ivec3& chunkPos, int x, int y, int z);
    int getVoxel(const glm::ivec3& chunkPos, int x, int y, int z);
    void markChunkForRendering(const glm::ivec3& chunkPos);
    void loadChunkIntoMemory(const glm::ivec3& chunkPos);
}; 