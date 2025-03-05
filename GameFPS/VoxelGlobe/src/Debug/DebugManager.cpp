// ./src/Debug/DebugManager.cpp
#include "Debug/DebugManager.hpp"

DebugManager::DebugManager()
    : showVoxelEdges_(false),
      enableCulling(true),  // Changed default to true for better performance
      useFaceColors_(false),
      logPlayerInfo_(false),
      logRaycast_(false),
      logChunkUpdates_(false),
      logBlockPlacement_(false),
      logCollision_(false),
      logInventory_(false),
      debugVertexScaling_(false) {  // Changed default to false since this is fixed now
}

DebugManager& DebugManager::getInstance() {
    static DebugManager instance;
    return instance;
}

// General debug toggles
bool DebugManager::showVoxelEdges() const { 
    return showVoxelEdges_; 
}

bool DebugManager::isCullingEnabled() const { 
    return enableCulling; 
}

bool DebugManager::useFaceColors() const { 
    return useFaceColors_; 
}

// Logging toggles
bool DebugManager::logPlayerInfo() const { 
    return logPlayerInfo_; 
}

bool DebugManager::logRaycast() const { 
    return logRaycast_; 
}

bool DebugManager::logChunkUpdates() const { 
    return logChunkUpdates_; 
}

bool DebugManager::logBlockPlacement() const { 
    return logBlockPlacement_; 
}

bool DebugManager::logCollision() const { 
    return logCollision_; 
}

bool DebugManager::logInventory() const { 
    return logInventory_; 
}

// Mesh debugging flag
bool DebugManager::debugVertexScaling() const { 
    return debugVertexScaling_; 
}

// Setters
void DebugManager::setShowVoxelEdges(bool enabled) { 
    showVoxelEdges_ = enabled; 
}

void DebugManager::setCullingEnabled(bool enabled) { 
    enableCulling = enabled; 
}

void DebugManager::setUseFaceColors(bool enabled) { 
    useFaceColors_ = enabled; 
}

void DebugManager::setLogPlayerInfo(bool enabled) { 
    logPlayerInfo_ = enabled; 
}

void DebugManager::setLogRaycast(bool enabled) { 
    logRaycast_ = enabled; 
}

void DebugManager::setLogChunkUpdates(bool enabled) { 
    logChunkUpdates_ = enabled; 
}

void DebugManager::setLogBlockPlacement(bool enabled) { 
    logBlockPlacement_ = enabled; 
}

void DebugManager::setLogCollision(bool enabled) { 
    logCollision_ = enabled; 
}

void DebugManager::setLogInventory(bool enabled) { 
    logInventory_ = enabled; 
}

void DebugManager::setDebugVertexScaling(bool enabled) { 
    debugVertexScaling_ = enabled; 
}