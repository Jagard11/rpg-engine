// ./include/Debug/DebugWindow.hpp
#ifndef DEBUG_WINDOW_HPP
#define DEBUG_WINDOW_HPP

#include "Debug/DebugManager.hpp"
#include "Player/Player.hpp"
#include "imgui.h"

class DebugWindow {
public:
    DebugWindow(DebugManager& debugMgr);
    void render(const Player& player);
    bool isVisible() const { return visible; }
    void toggleVisibility() { visible = !visible; }

private:
    DebugManager& debugManager;
    bool visible;
};

#endif