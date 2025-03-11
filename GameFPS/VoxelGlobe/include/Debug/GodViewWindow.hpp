// ./include/Debug/GodViewWindow.hpp
#ifndef GOD_VIEW_WINDOW_HPP
#define GOD_VIEW_WINDOW_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include "World/World.hpp"
#include "Graphics/GraphicsSettings.hpp"
#include "Debug/GodViewDebugTool.hpp"
#include "imgui.h"

/**
 * Dedicated window for displaying the God View of the globe
 * Can be resized, moved, and persists separately from the debug window
 */
class GodViewWindow {
public:
    GodViewWindow(const World& world, GodViewDebugTool* tool);
    ~GodViewWindow();

    // Render the God View window
    void render(const GraphicsSettings& settings);
    
    // UI state
    bool visible = false;
    
    // Window properties
    ImVec2 windowSize = ImVec2(600, 500);
    ImVec2 windowPos = ImVec2(50, 50);
    
    // Control state
    bool autoRotate = false;
    float rotationSpeed = 0.2f;
    float manualRotation = 0.0f;
    float zoom = 1.0f;
    
    // Camera settings
    glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, -30000.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    
    // Appearance
    bool wireframeMode = false;
    int visualizationType = 0;        // 0=terrain, 1=biomes, 2=block density
    int visualizationMode = 2;        // 0=procedural, 1=actual, 2=hybrid (maps to enum)
    bool useAdaptiveResolution = true;
    float adaptiveDetailFactor = 1.0f;
    bool showChunkBoundaries = false;
    
    // Preset views
    void setTopDownView();
    void setFrontView();
    void setPlayerView();
    void setRandomView();
    
    // Focus on specific areas
    void focusOnLocation(const glm::vec3& worldPos);
    
    // Save/restore camera state
    void saveViewState(const std::string& name);
    bool loadViewState(const std::string& name);
    
    // Access to the underlying tool
    GodViewDebugTool* getGodViewTool() { return godViewTool; }
    
private:
    GodViewDebugTool* godViewTool;
    const World& world;
    
    // Track last frame time for auto-rotation
    double lastFrameTime = 0.0;
    
    // Saved camera presets
    struct CameraPreset {
        glm::vec3 position;
        glm::vec3 target;
        float rotation;
        float zoom;
    };
    std::unordered_map<std::string, CameraPreset> savedPresets;
    
    // UI component rendering helpers
    void renderControlPanel();
    void renderVisualizationControls();
    void renderCameraControls();
    void renderPresetControls();
    void renderDebugInfo();
};

#endif // GOD_VIEW_WINDOW_HPP