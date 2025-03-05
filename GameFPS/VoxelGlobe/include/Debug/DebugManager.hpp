// ./include/Debug/DebugManager.hpp
#ifndef DEBUG_MANAGER_HPP
#define DEBUG_MANAGER_HPP

class DebugManager {
public:
    DebugManager();
    static DebugManager& getInstance();

    // General debug toggles
    bool showVoxelEdges() const;
    bool isCullingEnabled() const;
    bool useFaceColors() const;

    // Specific log toggles
    bool logPlayerInfo() const;
    bool logRaycast() const;
    bool logChunkUpdates() const;
    bool logBlockPlacement() const;
    bool logCollision() const;
    bool logInventory() const;
    
    // Mesh debugging options
    bool debugVertexScaling() const;

    // Setters
    void setShowVoxelEdges(bool enabled);
    void setCullingEnabled(bool enabled);
    void setUseFaceColors(bool enabled);
    void setLogPlayerInfo(bool enabled);
    void setLogRaycast(bool enabled);
    void setLogChunkUpdates(bool enabled);
    void setLogBlockPlacement(bool enabled);
    void setLogCollision(bool enabled);
    void setLogInventory(bool enabled);
    void setDebugVertexScaling(bool enabled);

private:
    bool showVoxelEdges_;
    bool enableCulling;
    bool useFaceColors_;
    bool logPlayerInfo_;
    bool logRaycast_;
    bool logChunkUpdates_;
    bool logBlockPlacement_;
    bool logCollision_;
    bool logInventory_;
    bool debugVertexScaling_; // Flag for controlling visual vertex scaling debugging
};

#endif