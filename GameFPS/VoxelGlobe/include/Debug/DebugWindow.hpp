// ./include/Debug/DebugWindow.hpp
#ifndef DEBUG_WINDOW_HPP
#define DEBUG_WINDOW_HPP

#include "Debug/DebugManager.hpp"
#include "Player/Player.hpp"
#include "imgui.h"

class DebugWindow {
public:
    DebugWindow(DebugManager& debugMgr, Player& player);
    void render();
    bool isVisible() const { return visible; }
    void toggleVisibility() { visible = !visible; }

private:
    DebugManager& debugManager;
    Player& player;
    bool visible;
    bool showMeshDebug; // Flag to toggle mesh debugging tools
    float teleportCoords[3] = {0.0f, 1510.0f, 0.0f}; // Default teleport coordinates
    
    // Helper function to set a block at world coordinates
    void setBlockHelper(int x, int y, int z, BlockType type);
};

#endif