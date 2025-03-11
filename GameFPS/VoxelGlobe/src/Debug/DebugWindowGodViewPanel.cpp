// ./src/Debug/DebugWindowGodViewPanel.cpp
#include "Debug/DebugWindow.hpp"
#include "Debug/Logger.hpp"
#include "Debug/DebugWindowUtility.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "Utils/SphereUtils.hpp"

void DebugWindow::renderGodViewPanel() {
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Enhanced God View Debug Tool");
    ImGui::TextWrapped("This tool provides a comprehensive globe visualization for debugging terrain generation and block placement.");
    ImGui::Separator();
    
    if (!godViewTool) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: God View Debug Tool not initialized!");
        return;
    }
    
    // Add persistent window toggle with more prominent UI
    ImGui::BeginGroup();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.9f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 1.0f, 0.7f));
    
    if (ImGui::Button("Open Separate Window", ImVec2(ImGui::GetContentRegionAvail().x * 0.48f, 30))) {
        showGodViewWindow = true;
        if (godViewWindow) {
            godViewWindow->visible = true;
            
            // Copy current settings to the window when enabling
            godViewWindow->manualRotation = godViewRotation;
            godViewWindow->autoRotate = godViewAutoRotate;
            godViewWindow->rotationSpeed = godViewRotationSpeed;
            godViewWindow->zoom = godViewZoom;
            godViewWindow->wireframeMode = godViewWireframe;
            godViewWindow->visualizationType = godViewVisualizationType;
            godViewWindow->visualizationMode = 2; // Hybrid mode by default
            
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
            
            LOG_INFO(LogCategory::UI, "God View window opened");
        }
    }
    
    ImGui::SameLine();
    
    // Toggle god view in this panel
    bool isActive = godViewTool->isActive();
    if (ImGui::Button(isActive ? "Hide in Panel" : "Show in Panel", ImVec2(ImGui::GetContentRegionAvail().x, 30))) {
        isActive = !isActive;
        godViewTool->setActive(isActive);
        showGodView = isActive;
        // Log when the God View is activated or deactivated
        LOG_INFO(LogCategory::UI, std::string("God View ") + (isActive ? "activated" : "deactivated"));
    }
    
    ImGui::PopStyleColor(2);
    ImGui::EndGroup();
    
    // If neither in-panel nor window is active, show message
    if (!isActive && !showGodViewWindow) {
        ImGui::Text("God View is disabled. Enable either panel or window display.");
        ImGui::TextWrapped("When enabled, a visualization will appear showing the globe with enhanced height visualization and block data.");
        
        // Add quick options for common views
        ImGui::Separator();
        ImGui::Text("Quick View Options:");
        
        if (ImGui::Button("North Pole View", ImVec2(150, 0))) {
            // Set camera to look at north pole
            godViewCameraPos[0] = 0.0f;
            godViewCameraPos[1] = 30.0f;
            godViewCameraPos[2] = 0.0f;
            
            godViewCameraTarget[0] = 0.0f;
            godViewCameraTarget[1] = 0.0f;
            godViewCameraTarget[2] = 0.0f;
            
            godViewRotation = 0.0f;
            godViewZoom = 1.0f;
            
            // Enable god view
            showGodView = true;
            godViewTool->setActive(true);
            
            LOG_INFO(LogCategory::UI, "God View set to North Pole view");
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Equatorial View", ImVec2(150, 0))) {
            // Set camera to look at equator
            godViewCameraPos[0] = 20.0f;
            godViewCameraPos[1] = 0.0f;
            godViewCameraPos[2] = -20.0f;
            
            godViewCameraTarget[0] = 0.0f;
            godViewCameraTarget[1] = 0.0f;
            godViewCameraTarget[2] = 0.0f;
            
            godViewRotation = 90.0f;
            godViewZoom = 1.0f;
            
            // Enable god view
            showGodView = true;
            godViewTool->setActive(true);
            
            LOG_INFO(LogCategory::UI, "God View set to Equatorial view");
        }
        
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
    if (ImGui::CollapsingHeader("Visualization Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Visualization type with more options
        const char* vizTypes[] = {"Terrain Height", "Biomes", "Block Density"};
        if (ImGui::Combo("Display Mode", &godViewVisualizationType, vizTypes, IM_ARRAYSIZE(vizTypes))) {
            godViewTool->setVisualizationType(godViewVisualizationType);
            
            // Sync to window if active
            if (godViewWindow && showGodViewWindow) {
                godViewWindow->visualizationType = godViewVisualizationType;
            }
            
            LOG_INFO(LogCategory::UI, "God View display mode changed to: " + std::string(vizTypes[godViewVisualizationType]));
        }
        
        // Add visualization mode selection
        int visualizationMode = 2; // Default to hybrid
        const char* vizModes[] = {"Procedural Only", "Actual Blocks Only", "Hybrid View"};
        if (ImGui::Combo("Data Source", &visualizationMode, vizModes, IM_ARRAYSIZE(vizModes))) {
            godViewTool->setVisualizationMode(static_cast<GodViewDebugTool::VisualizationMode>(visualizationMode));
            
            // Sync to window if active
            if (godViewWindow && showGodViewWindow) {
                godViewWindow->visualizationMode = visualizationMode;
            }
            
            LOG_INFO(LogCategory::UI, "God View data source changed to: " + std::string(vizModes[visualizationMode]));
        }
        
        // Wireframe toggle
        if (ImGui::Checkbox("Wireframe Mode", &godViewWireframe)) {
            godViewTool->setWireframeMode(godViewWireframe);
            
            // Sync to window if active
            if (godViewWindow && showGodViewWindow) {
                godViewWindow->wireframeMode = godViewWireframe;
            }
            
            LOG_DEBUG(LogCategory::UI, "God View wireframe mode " + std::string(godViewWireframe ? "enabled" : "disabled"));
        }
        
        // Adaptive resolution
        bool useAdaptiveResolution = true;
        if (ImGui::Checkbox("Adaptive Resolution", &useAdaptiveResolution)) {
            godViewTool->setAdaptiveResolution(useAdaptiveResolution);
            
            // Sync to window if active
            if (godViewWindow && showGodViewWindow) {
                godViewWindow->useAdaptiveResolution = useAdaptiveResolution;
            }
            
            LOG_INFO(LogCategory::UI, "God View adaptive resolution " + 
                     std::string(useAdaptiveResolution ? "enabled" : "disabled"));
        }
        
        // Detail factor
        float adaptiveDetailFactor = 1.0f;
        if (ImGui::SliderFloat("Detail Factor", &adaptiveDetailFactor, 0.5f, 2.0f, "%.2f")) {
            godViewTool->setAdaptiveDetailFactor(adaptiveDetailFactor);
            
            // Sync to window if active
            if (godViewWindow && showGodViewWindow) {
                godViewWindow->adaptiveDetailFactor = adaptiveDetailFactor;
            }
            
            LOG_DEBUG(LogCategory::UI, "God View detail factor set to: " + std::to_string(adaptiveDetailFactor));
        }
        
        // Add a button to refresh the visualization
        if (ImGui::Button("Refresh Visualization")) {
            godViewTool->clearHeightCache();
            LOG_INFO(LogCategory::UI, "God View visualization manually refreshed");
        }
    }
    
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
        if (ImGui::SliderFloat("Zoom Factor", &godViewZoom, 0.1f, 3.0f)) {
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
        
        // Add preset view buttons
        ImGui::Separator();
        ImGui::Text("Preset Views:");
        
        if (ImGui::Button("North Pole View", ImVec2(120, 0))) {
            godViewCameraPos[0] = 0.0f;
            godViewCameraPos[1] = 30.0f;
            godViewCameraPos[2] = 0.0f;
            
            godViewCameraTarget[0] = 0.0f;
            godViewCameraTarget[1] = 0.0f;
            godViewCameraTarget[2] = 0.0f;
            
            godViewRotation = 0.0f;
            godViewZoom = 1.0f;
            
            // Update the tool with new settings
            godViewTool->setCameraPosition(glm::vec3(0.0f, 30000.0f, 0.0f));
            godViewTool->setCameraTarget(glm::vec3(0.0f, 0.0f, 0.0f));
            godViewTool->rotateView(0.0f);
            godViewTool->setZoom(1.0f);
            
            // Sync to window if active
            if (godViewWindow && showGodViewWindow) {
                godViewWindow->cameraPosition = glm::vec3(0.0f, 30000.0f, 0.0f);
                godViewWindow->cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
                godViewWindow->manualRotation = 0.0f;
                godViewWindow->zoom = 1.0f;
            }
            
            LOG_INFO(LogCategory::UI, "God View set to North Pole view");
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Equatorial View", ImVec2(120, 0))) {
            godViewCameraPos[0] = 20.0f;
            godViewCameraPos[1] = 0.0f;
            godViewCameraPos[2] = -20.0f;
            
            godViewCameraTarget[0] = 0.0f;
            godViewCameraTarget[1] = 0.0f;
            godViewCameraTarget[2] = 0.0f;
            
            godViewRotation = 90.0f;
            godViewZoom = 1.0f;
            
            // Update the tool with new settings
            godViewTool->setCameraPosition(glm::vec3(20000.0f, 0.0f, -20000.0f));
            godViewTool->setCameraTarget(glm::vec3(0.0f, 0.0f, 0.0f));
            godViewTool->rotateView(90.0f);
            godViewTool->setZoom(1.0f);
            
            // Sync to window if active
            if (godViewWindow && showGodViewWindow) {
                godViewWindow->cameraPosition = glm::vec3(20000.0f, 0.0f, -20000.0f);
                godViewWindow->cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
                godViewWindow->manualRotation = 90.0f;
                godViewWindow->zoom = 1.0f;
            }
            
            LOG_INFO(LogCategory::UI, "God View set to Equatorial view");
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Overview", ImVec2(120, 0))) {
            godViewCameraPos[0] = 0.0f;
            godViewCameraPos[1] = 50.0f;
            godViewCameraPos[2] = -50.0f;
            
            godViewCameraTarget[0] = 0.0f;
            godViewCameraTarget[1] = 0.0f;
            godViewCameraTarget[2] = 0.0f;
            
            godViewRotation = 0.0f;
            godViewZoom = 0.5f;
            
            // Update the tool with new settings
            godViewTool->setCameraPosition(glm::vec3(0.0f, 50000.0f, -50000.0f));
            godViewTool->setCameraTarget(glm::vec3(0.0f, 0.0f, 0.0f));
            godViewTool->rotateView(0.0f);
            godViewTool->setZoom(0.5f);
            
            // Sync to window if active
            if (godViewWindow && showGodViewWindow) {
                godViewWindow->cameraPosition = glm::vec3(0.0f, 50000.0f, -50000.0f);
                godViewWindow->cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
                godViewWindow->manualRotation = 0.0f;
                godViewWindow->zoom = 0.5f;
            }
            
            LOG_INFO(LogCategory::UI, "God View set to Overview position");
        }
    }
    
    // Planet information section with more details
    if (ImGui::CollapsingHeader("Planet Information", ImGuiTreeNodeFlags_DefaultOpen)) {
        double planetRadius = player.getWorld().getRadius() / 1000.0f;
        double surfaceRadius = SphereUtils::getSurfaceRadiusMeters() / 1000.0f;
        
        ImGui::Text("Planet Radius: %.2f km", planetRadius);
        ImGui::Text("Surface Radius: %.2f km", surfaceRadius);
        ImGui::Text("Surface Area: %.2f million km²", 4.0 * 3.14159 * surfaceRadius * surfaceRadius);
        
        // Additional planet metrics
        float buildableHeight = PlanetConfig::MAX_BUILD_HEIGHT_METERS / 1000.0f;
        float buildableDepth = PlanetConfig::TERRAIN_DEPTH_METERS / 1000.0f;
        
        ImGui::Text("Buildable Height: %.2f km (above surface)", buildableHeight);
        ImGui::Text("Buildable Depth: %.2f km (below surface)", buildableDepth);
        ImGui::Text("Total Buildable Volume: %.2f trillion km³", 
                   4.0 * 3.14159 * (
                     pow(surfaceRadius + buildableHeight, 3) - 
                     pow(surfaceRadius - buildableDepth, 3)
                   ) / 3.0 / 1000.0f);
    }
    
    ImGui::Separator();
    ImGui::TextWrapped("Controls: Use the sliders above to adjust the view. The enhanced visualization shows terrain height, biomes, and actual block modifications with detailed color coding.");
}