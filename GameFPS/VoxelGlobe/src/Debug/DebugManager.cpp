// ./src/Debug/DebugManager.cpp
#include "Debug/DebugManager.hpp"

DebugManager::DebugManager()
    : showVoxelEdges_(false),
      enableCulling(false),
      useFaceColors_(false),
      logPlayerInfo_(false),
      logRaycast_(false),
      logChunkUpdates_(false),
      logBlockPlacement_(false),
      logCollision_(false),
      logInventory_(false) {} // Initialized new member to false

DebugManager& DebugManager::getInstance() {
    static DebugManager instance;
    return instance;
}