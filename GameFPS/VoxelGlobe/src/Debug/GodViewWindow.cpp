// ./src/Debug/GodViewWindow.cpp
#include "Debug/GodViewWindow.hpp"
#include "Debug/Logger.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/random.hpp>
#include <GLFW/glfw3.h>
#include "Utils/SphereUtils.hpp"
#include <ctime>
#include <random>
#include <fstream>
#include "../../third_party/nlohmann/json.hpp"

using json = nlohmann::json;

GodViewWindow::GodViewWindow(const World& w, GodViewDebugTool* tool)
    : world(w), 
      godViewTool(tool),
      lastFrameTime(glfwGetTime()),
      visible(false)  // Start hidden
{
    LOG_INFO(LogCategory::UI, "Enhanced God View Window initialized");
    
    // Create some default camera presets
    savedPresets["north_pole"] = {
        glm::vec3(0.0f, 20000.0f, -5000.0f),  // Position above north pole
        glm::vec3(0.0f, 0.0f, 0.0f),          // Looking at center
        0.0f,                                  // No rotation
        1.0f                                   // Normal zoom
    };
    
    savedPresets["equator"] = {
        glm::vec3(20000.0f, 0.0f, -20000.0f), // Position at equator
        glm::vec3(0.0f, 0.0f, 0.0f),          // Looking at center
        90.0f,                                 // 90 degree rotation
        1.0f                                   // Normal zoom
    };
    
    savedPresets["overview"] = {
        glm::vec3(0.0f, 50000.0f, -50000.0f), // High distant view
        glm::vec3(0.0f, 0.0f, 0.0f),          // Looking at center
        0.0f,                                  // No rotation
        0.5f                                   // Zoomed out
    };
}

GodViewWindow::~GodViewWindow() {
    // No need to delete godViewTool as it's now managed by DebugWindow
    LOG_INFO(LogCategory::UI, "Enhanced God View Window destroyed");
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
    
    // Create the window with a more sophisticated layout
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Planet Visualization", &visible, ImGuiWindowFlags_MenuBar)) {
        // Get current window position and size
        windowPos = ImGui::GetWindowPos();
        windowSize = ImGui::GetWindowSize();
        
        // Menu bar with various options
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save Current View", "Ctrl+S")) {
                    saveViewState("last_view");
                }
                if (ImGui::MenuItem("Load Last View", "Ctrl+L")) {
                    loadViewState("last_view");
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Close Window", "Esc")) {
                    visible = false;
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Top Down")) {
                    setTopDownView();
                }
                if (ImGui::MenuItem("Front View")) {
                    setFrontView();
                }
                if (ImGui::MenuItem("Follow Player")) {
                    setPlayerView();
                }
                if (ImGui::MenuItem("Random View")) {
                    setRandomView();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Reset View")) {
                    manualRotation = 0.0f;
                    zoom = 1.0f;
                    cameraPosition = glm::vec3(0.0f, 0.0f, -30000.0f);
                    cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Debug")) {
                if (ImGui::MenuItem("Show Chunk Boundaries", nullptr, showChunkBoundaries)) {
                    showChunkBoundaries = !showChunkBoundaries;
                }
                if (ImGui::MenuItem("Clear Height Cache")) {
                    godViewTool->clearHeightCache();
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMenuBar();
        }
        
        // Split the window into two panels using columns
        ImGui::Columns(2, "GodViewColumns", true);
        
        // Left panel - controls
        {
            ImGui::BeginChild("ControlsPanel", ImVec2(0, 0), true);
            
            renderControlPanel();
            
            ImGui::EndChild();
        }
        
        ImGui::NextColumn();
        
        // Right panel - visualization info
        {
            ImGui::BeginChild("VisualizationInfo", ImVec2(0, 0), true);
            
            renderDebugInfo();
            
            ImGui::EndChild();
        }
        
        ImGui::Columns(1);
        
        // Update the god view tool with current settings
        godViewTool->setCameraPosition(cameraPosition);
        godViewTool->setCameraTarget(cameraTarget);
        godViewTool->setZoom(zoom);
        godViewTool->setWireframeMode(wireframeMode);
        godViewTool->setVisualizationType(visualizationType);
        godViewTool->setVisualizationMode(static_cast<GodViewDebugTool::VisualizationMode>(visualizationMode));
        godViewTool->setAdaptiveResolution(useAdaptiveResolution);
        godViewTool->setAdaptiveDetailFactor(adaptiveDetailFactor);
        
        // Apply rotation
        if (autoRotate) {
            manualRotation += rotationSpeed * deltaTime * 60.0f;
            if (manualRotation >= 360.0f) manualRotation -= 360.0f;
        }
        godViewTool->rotateView(manualRotation);
        
        // Activate the globe view in world mode (renders as a small globe in the 3D world)
        godViewTool->setActive(true);
    }
    ImGui::End();
}

void GodViewWindow::renderControlPanel() {
    if (ImGui::CollapsingHeader("Visualization", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderVisualizationControls();
    }
    
    if (ImGui::CollapsingHeader("Camera Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderCameraControls();
    }
    
    if (ImGui::CollapsingHeader("Saved Views", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderPresetControls();
    }
}

void GodViewWindow::renderVisualizationControls() {
    // Visualization type
    const char* vizTypes[] = {"Terrain Height", "Biomes", "Block Density"};
    if (ImGui::Combo("Display Mode", &visualizationType, vizTypes, IM_ARRAYSIZE(vizTypes))) {
        // Log the visualization type change
        LOG_INFO(LogCategory::UI, "God View visualization type changed to: " + std::string(vizTypes[visualizationType]));
    }
    
    // Visualization mode (procedural vs actual)
    const char* vizModes[] = {"Procedural Only", "Actual Blocks Only", "Hybrid View"};
    if (ImGui::Combo("Data Source", &visualizationMode, vizModes, IM_ARRAYSIZE(vizModes))) {
        // Log the visualization mode change
        LOG_INFO(LogCategory::UI, "God View data source changed to: " + std::string(vizModes[visualizationMode]));
    }
    
    // Wireframe toggle
    if (ImGui::Checkbox("Wireframe Mode", &wireframeMode)) {
        // The wireframe setting will be applied in the render method
        LOG_DEBUG(LogCategory::UI, "God View wireframe mode " + std::string(wireframeMode ? "enabled" : "disabled"));
    }
    
    // Chunk boundaries toggle
    if (ImGui::Checkbox("Show Chunk Boundaries", &showChunkBoundaries)) {
        LOG_DEBUG(LogCategory::UI, "God View chunk boundaries " + 
                  std::string(showChunkBoundaries ? "enabled" : "disabled"));
    }
    
    // Resolution controls
    ImGui::Separator();
    ImGui::Text("Resolution Controls:");
    
    if (ImGui::Checkbox("Adaptive Resolution", &useAdaptiveResolution)) {
        LOG_INFO(LogCategory::UI, "God View adaptive resolution " + 
                 std::string(useAdaptiveResolution ? "enabled" : "disabled"));
    }
    
    ImGui::BeginDisabled(!useAdaptiveResolution);
    
    if (ImGui::SliderFloat("Detail Factor", &adaptiveDetailFactor, 0.5f, 2.0f, "%.2f")) {
        // The detail factor will be applied in the render method
        LOG_DEBUG(LogCategory::UI, "God View detail factor set to: " + std::to_string(adaptiveDetailFactor));
    }
    
    ImGui::EndDisabled();
    
    // Manual refresh button
    if (ImGui::Button("Refresh Visualization")) {
        godViewTool->clearHeightCache();
        LOG_INFO(LogCategory::UI, "God View visualization manually refreshed");
    }
}

void GodViewWindow::renderCameraControls() {
    // Rotation controls
    ImGui::Text("Rotation:");
    
    if (ImGui::Checkbox("Auto-Rotate", &autoRotate)) {
        LOG_DEBUG(LogCategory::UI, "God View auto-rotate " + std::string(autoRotate ? "enabled" : "disabled"));
    }
    
    ImGui::BeginDisabled(!autoRotate);
    if (ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 0.05f, 1.0f, "%.2f")) {
        LOG_DEBUG(LogCategory::UI, "God View rotation speed set to: " + std::to_string(rotationSpeed));
    }
    ImGui::EndDisabled();
    
    if (ImGui::SliderFloat("Manual Rotation", &manualRotation, 0.0f, 360.0f, "%.1f°")) {
        if (autoRotate) {
            // Disable auto-rotation when manually rotating
            autoRotate = false;
            LOG_DEBUG(LogCategory::UI, "God View auto-rotate disabled due to manual rotation");
        }
    }
    
    // Zoom control
    ImGui::Separator();
    ImGui::Text("Zoom:");
    
    if (ImGui::SliderFloat("Zoom Factor", &zoom, 0.1f, 3.0f, "%.2f")) {
        LOG_DEBUG(LogCategory::UI, "God View zoom factor set to: " + std::to_string(zoom));
    }
    
    // Camera position and target controls
    ImGui::Separator();
    ImGui::Text("Camera Position (km):");
    
    // Position controls
    if (ImGui::InputFloat3("Camera Pos", glm::value_ptr(cameraPosition), "%.1f")) {
        LOG_DEBUG(LogCategory::UI, "God View camera position updated");
    }
    
    ImGui::Text("Look At Point (km):");
    if (ImGui::InputFloat3("Target Pos", glm::value_ptr(cameraTarget), "%.1f")) {
        LOG_DEBUG(LogCategory::UI, "God View camera target updated");
    }
}

void GodViewWindow::renderPresetControls() {
    ImGui::Text("Saved Views:");
    
    // Add quick preset buttons
    if (ImGui::Button("North Pole")) {
        loadViewState("north_pole");
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Equator")) {
        loadViewState("equator");
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Overview")) {
        loadViewState("overview");
    }
    
    // Add custom preset controls
    ImGui::Separator();
    static char presetName[32] = "custom_view";
    ImGui::InputText("Preset Name", presetName, IM_ARRAYSIZE(presetName));
    
    if (ImGui::Button("Save Current View")) {
        saveViewState(presetName);
        LOG_INFO(LogCategory::UI, "Saved current view as: " + std::string(presetName));
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Load View")) {
        if (loadViewState(presetName)) {
            LOG_INFO(LogCategory::UI, "Loaded view: " + std::string(presetName));
        } else {
            LOG_WARNING(LogCategory::UI, "Failed to load view: " + std::string(presetName));
        }
    }
}

void GodViewWindow::renderDebugInfo() {
    // Planet statistics
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Planet Information");
    
    // Calculate planet metrics
    double planetRadius = world.getRadius() / 1000.0; // Convert to km
    double surfaceRadius = SphereUtils::getSurfaceRadiusMeters() / 1000.0; // km
    double surfaceArea = 4.0 * 3.14159 * surfaceRadius * surfaceRadius;
    
    ImGui::Text("Planet Radius: %.2f km", planetRadius);
    ImGui::Text("Surface Radius: %.2f km", surfaceRadius);
    ImGui::Text("Surface Area: %.2f million km²", surfaceArea);
    
    // Chunk statistics
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Chunk Statistics");
    
    // Calculate chunk metrics
    size_t totalChunks = world.getChunks().size();
    
    // Calculate loaded area radius
    int chunkSize = Chunk::SIZE;
    float loadedAreaRadius = std::sqrt(totalChunks) * chunkSize / 2.0f / 1000.0f; // km
    
    ImGui::Text("Total Loaded Chunks: %zu", totalChunks);
    ImGui::Text("Loaded Area Radius: ~%.2f km", loadedAreaRadius);
    ImGui::Text("Chunk Size: %d blocks", chunkSize);
    
    // Camera information
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Camera Information");
    
    // Calculate distance to surface
    double cameraDist = glm::length(glm::dvec3(cameraPosition));
    double distToSurface = cameraDist - surfaceRadius * 1000.0;
    
    ImGui::Text("Camera Distance: %.2f km", cameraDist / 1000.0);
    ImGui::Text("Height Above Surface: %.2f km", distToSurface / 1000.0);
    ImGui::Text("Current Rotation: %.1f°", manualRotation);
    ImGui::Text("Zoom Factor: %.2fx", zoom);
    
    // Visualization info
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Visualization Info");
    
    const char* vizTypeStr = "Unknown";
    switch (visualizationType) {
        case 0: vizTypeStr = "Terrain Height"; break;
        case 1: vizTypeStr = "Biomes"; break;
        case 2: vizTypeStr = "Block Density"; break;
    }
    
    const char* vizModeStr = "Unknown";
    switch (visualizationMode) {
        case 0: vizModeStr = "Procedural Only"; break;
        case 1: vizModeStr = "Actual Blocks Only"; break;
        case 2: vizModeStr = "Hybrid View"; break;
    }
    
    ImGui::Text("Display Mode: %s", vizTypeStr);
    ImGui::Text("Data Source: %s", vizModeStr);
    ImGui::Text("Wireframe: %s", wireframeMode ? "Enabled" : "Disabled");
    ImGui::Text("Resolution: %s", useAdaptiveResolution ? "Adaptive" : "Static");
    ImGui::Text("Detail Factor: %.2fx", adaptiveDetailFactor);
}

void GodViewWindow::setTopDownView() {
    cameraPosition = glm::vec3(0.0f, 30000.0f, 0.0f);
    cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    manualRotation = 0.0f;
    zoom = 1.0f;
    LOG_INFO(LogCategory::UI, "God View set to top-down view");
}

void GodViewWindow::setFrontView() {
    cameraPosition = glm::vec3(0.0f, 0.0f, -30000.0f);
    cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    manualRotation = 0.0f;
    zoom = 1.0f;
    LOG_INFO(LogCategory::UI, "God View set to front view");
}

void GodViewWindow::setPlayerView() {
    // This is just a placeholder - the actual implementation would need
    // to access the player's position, which is not currently available here
    // We would need player position from the World or Game object
    
    // As a fallback, set a view that looks at the origin
    cameraPosition = glm::vec3(10000.0f, 10000.0f, -10000.0f);
    cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    manualRotation = 45.0f;
    zoom = 1.0f;
    LOG_INFO(LogCategory::UI, "God View set to simulated player view");
}

void GodViewWindow::setRandomView() {
    // Seed with current time
    static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_real_distribution<float> angleDist(0.0f, 360.0f);
    std::uniform_real_distribution<float> heightDist(5000.0f, 50000.0f);
    std::uniform_real_distribution<float> zoomDist(0.5f, 1.5f);
    
    // Generate random spherical coordinates
    float azimuth = angleDist(rng);
    float elevation = angleDist(rng) / 2.0f - 90.0f; // -90 to +90 degrees
    float distance = heightDist(rng);
    
    // Convert to Cartesian coordinates
    float radElevation = glm::radians(elevation);
    float radAzimuth = glm::radians(azimuth);
    
    float x = distance * std::cos(radElevation) * std::cos(radAzimuth);
    float y = distance * std::sin(radElevation);
    float z = distance * std::cos(radElevation) * std::sin(radAzimuth);
    
    // Set random view
    cameraPosition = glm::vec3(x, y, z);
    cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    manualRotation = angleDist(rng);
    zoom = zoomDist(rng);
    
    LOG_INFO(LogCategory::UI, "God View set to random perspective");
}

void GodViewWindow::focusOnLocation(const glm::vec3& worldPos) {
    // Calculate direction from center to position
    glm::vec3 dirFromCenter = glm::normalize(worldPos);
    
    // Set camera target to the specified position
    cameraTarget = worldPos;
    
    // Position camera at a reasonable distance from the target, looking at it
    float viewDistance = 10000.0f; // 10km away from the target
    cameraPosition = worldPos + dirFromCenter * (-viewDistance);
    
    // Adjust rotation to match the orientation
    float azimuth = std::atan2(dirFromCenter.z, dirFromCenter.x);
    manualRotation = glm::degrees(azimuth);
    
    // Set zoom to emphasize the target
    zoom = 1.0f;
    
    LOG_INFO(LogCategory::UI, "God View focused on location: " + 
             std::to_string(worldPos.x) + ", " + 
             std::to_string(worldPos.y) + ", " + 
             std::to_string(worldPos.z));
}

void GodViewWindow::saveViewState(const std::string& name) {
    // Save current camera settings
    CameraPreset preset = {
        cameraPosition,
        cameraTarget,
        manualRotation,
        zoom
    };
    
    // Store in memory
    savedPresets[name] = preset;
    
    // Optionally, save to disk for persistence
    try {
        json presetData;
        
        // Convert preset data to JSON
        json presetJson;
        presetJson["position"] = {preset.position.x, preset.position.y, preset.position.z};
        presetJson["target"] = {preset.target.x, preset.target.y, preset.target.z};
        presetJson["rotation"] = preset.rotation;
        presetJson["zoom"] = preset.zoom;
        
        // Add to saved presets
        presetData["views"][name] = presetJson;
        
        // Also save current visualization settings
        presetData["settings"]["visualizationType"] = visualizationType;
        presetData["settings"]["visualizationMode"] = visualizationMode;
        presetData["settings"]["wireframeMode"] = wireframeMode;
        presetData["settings"]["useAdaptiveResolution"] = useAdaptiveResolution;
        presetData["settings"]["adaptiveDetailFactor"] = adaptiveDetailFactor;
        
        // Save to file
        std::ofstream file("godview_presets.json");
        if (file.is_open()) {
            file << presetData.dump(4);
            LOG_INFO(LogCategory::UI, "Saved God View preset to disk: " + name);
        } else {
            LOG_ERROR(LogCategory::UI, "Failed to save God View preset to disk: " + name);
        }
    } catch (...) {
        LOG_ERROR(LogCategory::UI, "Exception while saving God View preset: " + name);
    }
}

bool GodViewWindow::loadViewState(const std::string& name) {
    // Check if we have this preset in memory
    auto it = savedPresets.find(name);
    if (it != savedPresets.end()) {
        // Apply the preset
        cameraPosition = it->second.position;
        cameraTarget = it->second.target;
        manualRotation = it->second.rotation;
        zoom = it->second.zoom;
        
        LOG_INFO(LogCategory::UI, "Loaded God View preset from memory: " + name);
        return true;
    }
    
    // If not in memory, try to load from disk
    try {
        std::ifstream file("godview_presets.json");
        if (file.is_open()) {
            json presetData;
            file >> presetData;
            
            // Check if the named preset exists
            if (presetData.contains("views") && presetData["views"].contains(name)) {
                auto& presetJson = presetData["views"][name];
                
                // Parse position
                if (presetJson.contains("position") && presetJson["position"].is_array() && 
                    presetJson["position"].size() == 3) {
                    cameraPosition.x = presetJson["position"][0];
                    cameraPosition.y = presetJson["position"][1];
                    cameraPosition.z = presetJson["position"][2];
                }
                
                // Parse target
                if (presetJson.contains("target") && presetJson["target"].is_array() && 
                    presetJson["target"].size() == 3) {
                    cameraTarget.x = presetJson["target"][0];
                    cameraTarget.y = presetJson["target"][1];
                    cameraTarget.z = presetJson["target"][2];
                }
                
                // Parse rotation and zoom
                if (presetJson.contains("rotation")) manualRotation = presetJson["rotation"];
                if (presetJson.contains("zoom")) zoom = presetJson["zoom"];
                
                // Save to memory for future use
                savedPresets[name] = {cameraPosition, cameraTarget, manualRotation, zoom};
                
                LOG_INFO(LogCategory::UI, "Loaded God View preset from disk: " + name);
                return true;
            }
        }
    } catch (...) {
        LOG_ERROR(LogCategory::UI, "Exception while loading God View preset: " + name);
    }
    
    LOG_WARNING(LogCategory::UI, "God View preset not found: " + name);
    return false;
}