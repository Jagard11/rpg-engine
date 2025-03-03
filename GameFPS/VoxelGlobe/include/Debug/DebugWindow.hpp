// ./include/Debug/DebugWindow.hpp
#ifndef DEBUG_WINDOW_HPP
#define DEBUG_WINDOW_HPP

#include "Debug/DebugManager.hpp"
#include "Player/Player.hpp"
#include "imgui.h"

class DebugWindow {
public:
    DebugWindow(DebugManager& debugMgr, Player& player); // Updated to take Player reference
    void render();
    bool isVisible() const { return visible; }
    void toggleVisibility() { visible = !visible; }

private:
    DebugManager& debugManager;
    Player& player; // Added Player reference
    bool visible;
    float teleportCoords[3] = {0.0f, 1510.0f, 0.0f}; // Default teleport coordinates
};

#endif