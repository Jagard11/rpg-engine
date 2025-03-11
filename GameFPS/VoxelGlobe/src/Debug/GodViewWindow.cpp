// ./src/Debug/GodViewWindow.cpp
#include "Debug/GodViewWindow.hpp"
#include "Debug/Logger.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>

GodViewWindow::GodViewWindow(const World& w, GodViewDebugTool* tool)
    : world(w), 
      godViewTool(tool),
      lastFrameTime(glfwGetTime()),
      visible(false)  // Start hidden
{
    LOG_INFO(LogCategory::UI, "God View Window initialized");
}

GodViewWindow::~GodViewWindow() {
    // No need to delete godViewTool as it's now managed by DebugWindow
    LOG_INFO(LogCategory::UI, "God View Window destroyed");
}


void GodViewWindow::render(const GraphicsSettings& settings) {
    // SAFETY CHECK: Only render if visible
    if (!visible || !godViewTool || !ImGui::GetCurrentContext()) {
        return;
    }
    
    // Calculate delta time for smooth auto-rotation
    double currentTime = glfwGetTime();
    double deltaTime = currentTime - lastFrameTime;
    lastFrameTime = currentTime;
    
    // Create a window for controls only, not for 3D rendering
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Globe Controls", &visible)) {
        // Get current window position and size
        windowPos = ImGui::GetWindowPos();
        windowSize = ImGui::GetWindowSize();
        
        // Update the god view tool with current settings
        godViewTool->setCameraPosition(cameraPosition);
        godViewTool->setCameraTarget(cameraTarget);
        godViewTool->setZoom(zoom);
        godViewTool->setWireframeMode(wireframeMode);
        godViewTool->setVisualizationType(visualizationType);
        
        // Apply rotation
        if (autoRotate) {
            manualRotation += rotationSpeed * deltaTime * 60.0f;
            if (manualRotation >= 360.0f) manualRotation -= 360.0f;
        }
        godViewTool->rotateView(manualRotation);
        
        // Controls
        if (ImGui::Button("Reset View")) {
            manualRotation = 0.0f;
            zoom = 1.0f;
            cameraPosition = glm::vec3(0.0f, 0.0f, -30000.0f);
            cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        }
        
        ImGui::SameLine();
        if (ImGui::Button("North Pole")) {
            manualRotation = 0.0f;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("South Pole")) {
            manualRotation = 180.0f;
        }
        
        ImGui::Checkbox("Auto-Rotate", &autoRotate);
        if (autoRotate) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.0f);
            ImGui::SliderFloat("Speed", &rotationSpeed, 0.05f, 1.0f);
        }
        
        ImGui::SliderFloat("Rotation", &manualRotation, 0.0f, 360.0f);
        if (ImGui::IsItemActive() && autoRotate) {
            autoRotate = false;
        }
        
        ImGui::SliderFloat("Zoom", &zoom, 0.1f, 3.0f);
        ImGui::Checkbox("Wireframe", &wireframeMode);
        
        const char* vizTypes[] = {"Terrain", "Biomes", "Block Density"};
        ImGui::Combo("Display", &visualizationType, vizTypes, IM_ARRAYSIZE(vizTypes));
        
        ImGui::Separator();
        
        // Information about the globe
        double planetRadius = world.getRadius() / 1000.0; // Convert to km
        ImGui::Text("Planet Radius: %.2f km", planetRadius);
        ImGui::Text("Surface Area: %.2f million km²", 4.0 * 3.14159 * planetRadius * planetRadius);
        
        // Activate the globe view in world mode (renders as a small globe in the 3D world)
        godViewTool->setActive(true);
    }
    ImGui::End();
}