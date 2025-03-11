// ./src/Debug/DebugWindowGodViewPanel.cpp
#include "Debug/DebugWindow.hpp"
#include "Debug/Logger.hpp"
#include "Debug/DebugWindowUtility.hpp"
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
    
    // Add persistent window toggle
    if (ImGui::Checkbox("Show in Separate Window", &showGodViewWindow)) {
        if (godViewWindow) {
            godViewWindow->visible = showGodViewWindow;
            
            // Copy current settings to the window when enabling
            if (showGodViewWindow) {
                godViewWindow->manualRotation = godViewRotation;
                godViewWindow->autoRotate = godViewAutoRotate;
                godViewWindow->rotationSpeed = godViewRotationSpeed;
                godViewWindow->zoom = godViewZoom;
                godViewWindow->wireframeMode = godViewWireframe;
                godViewWindow->visualizationType = godViewVisualizationType;
                
                // Set camera position and target
                godViewWindow->cameraPosition = glm::vec3(
                    godViewCameraPos[0] * 1000.0f,
                    godViewCameraPos[1] * 1000.0f,
                    godViewCameraPos[2] * 1000.0f
                );
                
                godViewWindow->cameraTarget = glm::vec3(
                    godViewCameraTarget[0] * 1000.0f,
                    godViewCameraTarget[1] * 1000.0f,
                    godViewCameraTarget[2] * 1000.0f
                );
            }
        }
    }
    
    ImGui::SameLine();
    
    // Toggle god view in this panel
    bool isActive = godViewTool->isActive();
    if (ImGui::Checkbox("Show in Panel", &isActive)) {
        godViewTool->setActive(isActive);
        // Log when the God View is activated or deactivated
        LOG_INFO(LogCategory::UI, std::string("God View ") + (isActive ? "activated" : "deactivated"));
    }
    
    // Add a force-activate button for emergency debugging
    if (ImGui::Button("Force Activate (Debug)")) {
        DebugWindowUtility::forceActivateGodView(godViewTool);
    }
    
    // If neither in-panel nor window is active, show message
    if (!isActive && !showGodViewWindow) {
        ImGui::Text("God View is disabled. Enable either panel or window display.");
        ImGui::TextWrapped("When enabled, a visualization will appear showing the globe view.");
        return;
    }
    
    if (isActive) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "God View is active in panel!");
    }
    
    if (showGodViewWindow) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "God View window is active!");
    }
    
    ImGui::Separator();
    
    // Camera position controls
    if (ImGui::CollapsingHeader("Camera Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Camera position controls with wider range
        if (ImGui::SliderFloat3("Camera Position (km)", godViewCameraPos, -100.0f, 100.0f)) {
            // Convert km to meters for internal representation
            glm::vec3 posMeter = glm::vec3(
                godViewCameraPos[0] * 1000.0f,
                godViewCameraPos[1] * 1000.0f,
                godViewCameraPos[2] * 1000.0f
            );
            godViewTool->setCameraPosition(posMeter);
            
            // Sync to window if active
            if (godViewWindow && showGodViewWindow) {
                godViewWindow->cameraPosition = posMeter;
            }
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
            
            // Sync to window if active
            if (godViewWindow && showGodViewWindow) {
                godViewWindow->cameraTarget = targetMeter;
            }
        }
        
        // Zoom control
        if (ImGui::SliderFloat("Zoom Factor", &godViewZoom, 0.1f, 10.0f)) {
            godViewTool->setZoom(godViewZoom);
            
            // Sync to window if active
            if (godViewWindow && showGodViewWindow) {
                godViewWindow->zoom = godViewZoom;
            }
        }
        
        // Auto-rotation settings
        if (ImGui::Checkbox("Auto-Rotate", &godViewAutoRotate)) {
            // Sync to window if active
            if (godViewWindow && showGodViewWindow) {
                godViewWindow->autoRotate = godViewAutoRotate;
            }
        }
        
        if (godViewAutoRotate) {
            ImGui::SameLine();
            if (ImGui::SliderFloat("Rotation Speed", &godViewRotationSpeed, 0.05f, 1.0f)) {
                // Sync to window if active
                if (godViewWindow && showGodViewWindow) {
                    godViewWindow->rotationSpeed = godViewRotationSpeed;
                }
            }
        }
        
        // Manual rotation control
        if (ImGui::SliderFloat("Rotation", &godViewRotation, 0.0f, 360.0f)) {
            godViewTool->rotateView(godViewRotation);
            
            // Sync to window if active
            if (godViewWindow && showGodViewWindow) {
                godViewWindow->manualRotation = godViewRotation;
            }
        }
    }
    
    // Visual settings
    if (ImGui::CollapsingHeader("Visual Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Wireframe toggle
        if (ImGui::Checkbox("Wireframe Mode", &godViewWireframe)) {
            godViewTool->setWireframeMode(godViewWireframe);
            
            // Sync to window if active
            if (godViewWindow && showGodViewWindow) {
                godViewWindow->wireframeMode = godViewWireframe;
            }
        }
        
        // Visualization type
        const char* vizTypes[] = {"Terrain", "Biomes", "Block Density"};
        if (ImGui::Combo("Visualization", &godViewVisualizationType, vizTypes, IM_ARRAYSIZE(vizTypes))) {
            godViewTool->setVisualizationType(godViewVisualizationType);
            
            // Sync to window if active
            if (godViewWindow && showGodViewWindow) {
                godViewWindow->visualizationType = godViewVisualizationType;
            }
        }
    }
    
    ImGui::Separator();
    ImGui::TextWrapped("Controls: Use the sliders above to adjust the view. The globe representation shows the planet with a flat surface as it currently exists in the game.");
    
    // Real-time visualization info
    double planetRadius = player.getWorld().getRadius();
    ImGui::Text("Planet Radius: %.2f km", planetRadius / 1000.0);
    ImGui::Text("Surface Area: %.2f million km²", 4.0 * 3.14159 * pow(planetRadius / 1000.0, 2));
}