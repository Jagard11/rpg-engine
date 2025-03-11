// ./include/Debug/DebugWindow.hpp
#ifndef DEBUG_WINDOW_HPP
#define DEBUG_WINDOW_HPP

#include "Debug/DebugManager.hpp"
#include "Debug/DebugSystem.hpp"
#include "Debug/GodViewDebugTool.hpp"
#include "Player/Player.hpp"
#include "imgui.h"

class DebugWindow {
public:
    DebugWindow(DebugManager& debugMgr, Player& player);
    ~DebugWindow();
    void render();
    bool isVisible() const { return visible; }
    void toggleVisibility();
    void saveWindowState();
    void loadWindowState();
    
    // Accessor for GodViewDebugTool
    GodViewDebugTool* getGodViewTool() { return godViewTool; }
    
    // Render the god view if active
    void renderGodView(const GraphicsSettings& settings);

private:
    DebugManager& debugManager;
    Player& player;
    bool visible;
    
    // God view tool
    GodViewDebugTool* godViewTool;
    
    // UI State for each debug panel
    bool showMeshDebug; // Flag to toggle mesh debugging tools
    bool showLoggingConfig; // Flag to toggle logging configuration panel
    bool showPerformance; // Flag to toggle performance metrics panel
    bool showGodView; // Flag to toggle god view panel
    
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
    
    // God view control state
    float godViewCameraPos[3] = {0.0f, 0.0f, -30.0f}; // Values in km
    float godViewCameraTarget[3] = {0.0f, 0.0f, 0.0f};
    float godViewZoom = 1.0f;
    float godViewRotation = 0.0f;
    bool godViewWireframe = false;
    int godViewVisualizationType = 0;
    bool godViewAutoRotate = false;
    float godViewRotationSpeed = 0.2f;
    
    // Helper function to set a block at world coordinates
    void setBlockHelper(int x, int y, int z, BlockType type);
    
    // Render different debug panels
    void renderLoggingPanel();
    void renderVisualizationPanel();
    void renderWorldDebugPanel();
    void renderPerformancePanel();
    void renderPlayerInfoPanel();
    void renderGodViewPanel();
    
    // Sync UI state with DebugManager
    void syncWithDebugManager();
};

#endif