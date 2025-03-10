// ./include/Debug/DebugWindow.hpp
#ifndef DEBUG_WINDOW_HPP
#define DEBUG_WINDOW_HPP

#include "Debug/DebugManager.hpp"
#include "Debug/DebugSystem.hpp"
#include "Player/Player.hpp"
#include "imgui.h"

class DebugWindow {
public:
    DebugWindow(DebugManager& debugMgr, Player& player);
    void render();
    bool isVisible() const { return visible; }
    void toggleVisibility() { visible = !visible; }
    void saveWindowState();
    void loadWindowState();

private:
    DebugManager& debugManager;
    Player& player;
    bool visible;
    
    // UI State for each debug panel
    bool showMeshDebug; // Flag to toggle mesh debugging tools
    bool showLoggingConfig; // Flag to toggle logging configuration panel
    bool showPerformance; // Flag to toggle performance metrics panel
    
    // Teleport coordinates
    float teleportCoords[3] = {0.0f, 1510.0f, 0.0f}; // Default teleport coordinates
    
    // Log level selection and display
    int currentLogLevel = static_cast<int>(LogLevel::DEBUG);
    const char* logLevelNames[6] = {"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};
    
    // Category toggles
    bool categoryEnabled[7] = {true, true, true, true, true, true, true};
    const char* categoryNames[7] = {"GENERAL", "WORLD", "PLAYER", "PHYSICS", "RENDERING", "INPUT", "UI"};
    
    // Performance metrics tracking
    float frameTimeHistory[100] = {0};
    int frameTimeIndex = 0;
    float minFrameTime = 0.0f;
    float maxFrameTime = 0.0f;
    float avgFrameTime = 0.0f;
    
    // Helper function to set a block at world coordinates
    void setBlockHelper(int x, int y, int z, BlockType type);
    
    // Render different debug panels
    void renderLoggingPanel();
    void renderVisualizationPanel();
    void renderWorldDebugPanel();
    void renderPerformancePanel();
    void renderPlayerInfoPanel();
    
    // Sync UI state with DebugManager
    void syncWithDebugManager();
};

#endif