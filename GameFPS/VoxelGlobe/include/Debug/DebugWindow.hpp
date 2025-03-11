// ./include/Debug/DebugWindow.hpp
#ifndef DEBUG_WINDOW_HPP
#define DEBUG_WINDOW_HPP

#include "Debug/DebugManager.hpp"
#include "Debug/DebugSystem.hpp"
#include "Debug/GodViewDebugTool.hpp"
#include "Debug/GodViewWindow.hpp"
#include "Player/Player.hpp"
#include "Graphics/GraphicsSettings.hpp"
#include "imgui.h"

class DebugWindow {
public:
    DebugWindow(DebugManager& debugMgr, Player& player);
    ~DebugWindow();
    void render(const GraphicsSettings& settings);
    bool isVisible() const { return visible; }
    void toggleVisibility();
    void saveWindowState();
    void loadWindowState();
    
    // Accessor for GodViewDebugTool
    GodViewDebugTool* getGodViewTool() { return godViewTool; }
    
    // Accessor for GodViewWindow
    GodViewWindow* getGodViewWindow() { return godViewWindow; }
    
    // Render the god view if active
    void renderGodView(const GraphicsSettings& settings);
    
    // Render the god view panel (part of the debug window)
    void renderGodViewPanel();

private:
    DebugManager& debugManager;
    Player& player;
    bool visible;
    
    // God view tools
    GodViewDebugTool* godViewTool;
    GodViewWindow* godViewWindow;
    
    // UI State for each debug panel
    bool showMeshDebug; // Flag to toggle mesh debugging tools
    bool showLoggingConfig; // Flag to toggle logging configuration panel
    bool showPerformance; // Flag to toggle performance metrics panel
    bool showGodView; // Flag to toggle god view panel
    bool showGodViewWindow; // Flag to toggle persistent god view window
    
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
    
    // Sync UI state with DebugManager
    void syncWithDebugManager();
};

#endif