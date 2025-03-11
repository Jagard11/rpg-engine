// ./src/Debug/DebugWindowGodViewPanel.cpp
#include "Debug/DebugWindow.hpp"
#include "Debug/Logger.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

void DebugWindow::renderGodViewPanel() {
    ImGui::Text("God View Debug Tool");
    ImGui::TextWrapped("This tool provides a global view of the planet for debugging terrain generation algorithms.");
    ImGui::Separator();
    
    if (!godViewTool) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: God View Debug Tool not initialized!");
        return;
    }
    
    // Toggle god view
    bool isActive = godViewTool->isActive();
    if (ImGui::Checkbox("Enable God View", &isActive)) {
        godViewTool->setActive(isActive);
        // Log when the God View is activated or deactivated
        LOG_INFO(LogCategory::UI, std::string("God View ") + (isActive ? "activated" : "deactivated"));
    }
    
    if (!isActive) {
        ImGui::Text("God View is disabled. Enable it to see the globe visualization.");
        ImGui::TextWrapped("When enabled, a visualization will appear showing the globe view.");
        return;
    }
    
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "God View is active!");
    ImGui::TextWrapped("The globe visualization is now active. Use arrow keys to rotate the globe.");
    ImGui::Separator();
    
    // Camera position controls
    if (ImGui::CollapsingHeader("Camera Settings")) {
        // Camera position controls with wider range
        if (ImGui::SliderFloat3("Camera Position (km)", godViewCameraPos, -100.0f, 100.0f)) {
            // Convert km to meters for internal representation
            glm::vec3 posMeter = glm::vec3(
                godViewCameraPos[0] * 1000.0f,
                godViewCameraPos[1] * 1000.0f,
                godViewCameraPos[2] * 1000.0f
            );
            godViewTool->setCameraPosition(posMeter);
        }
        
        // Look at target
        if (ImGui::SliderFloat3("Look At (km)", godViewCameraTarget, -20.0f, 20.0f)) {
            // Convert km to meters for internal representation
            glm::vec3 targetMeter = glm::vec3(
                godViewCameraTarget[0] * 1000.0f, 
                godViewCameraTarget[1] * 1000.0f, 
                godViewCameraTarget[2] * 1000.0f
            );
            godViewTool->setCameraTarget(targetMeter);
        }
        
        // Zoom control
        if (ImGui::SliderFloat("Zoom Factor", &godViewZoom, 0.1f, 10.0f)) {
            godViewTool->setZoom(godViewZoom);
        }
        
        // Auto-rotation settings
        if (ImGui::Checkbox("Auto-Rotate", &godViewAutoRotate)) {
            // This will be handled in the main render loop
        }
        
        if (godViewAutoRotate) {
            ImGui::SliderFloat("Rotation Speed", &godViewRotationSpeed, 0.05f, 1.0f);
        }
    }
    
    // Visual settings
    if (ImGui::CollapsingHeader("Visual Settings")) {
        // Wireframe toggle
        if (ImGui::Checkbox("Wireframe Mode", &godViewWireframe)) {
            godViewTool->setWireframeMode(godViewWireframe);
        }
        
        // Visualization type
        const char* vizTypes[] = {"Height Map", "Biomes", "Block Density"};
        if (ImGui::Combo("Visualization", &godViewVisualizationType, vizTypes, IM_ARRAYSIZE(vizTypes))) {
            godViewTool->setVisualizationType(godViewVisualizationType);
        }
        
        // Show the height range information
        ImGui::Separator();
        ImGui::Text("Height Visualization Range:");
        ImGui::TextColored(ImVec4(0.2f, 0.2f, 0.8f, 1.0f), "Blue: -5km (below sea level)");
        ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "Green: Sea level");
        ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "Red: +15km (above sea level)");
    }
    
    ImGui::Separator();
    ImGui::TextWrapped("Controls: Use arrow keys to rotate the globe. After 15 seconds of inactivity, auto-rotation will resume with north pole facing up.");
    
    // Real-time visualization info
    double planetRadius = player.getWorld().getRadius();
    ImGui::Text("Planet Radius: %.2f km", planetRadius / 1000.0);
    ImGui::Text("Surface Area: %.2f million km²", 4.0 * 3.14159 * pow(planetRadius / 1000.0, 2));
}

void DebugWindow::renderGodView(const GraphicsSettings& settings) {
    // Render the God View if it's initialized and active
    if (godViewTool && godViewTool->isActive()) {
        try {
            LOG_DEBUG(LogCategory::RENDERING, "Rendering God View");
            godViewTool->render(settings);
        } catch (const std::exception& e) {
            LOG_ERROR(LogCategory::RENDERING, "Error rendering God View: " + std::string(e.what()));
        }
    }
}