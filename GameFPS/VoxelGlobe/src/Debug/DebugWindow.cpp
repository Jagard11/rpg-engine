// ./src/Debug/DebugWindow.cpp
#include "Debug/DebugWindow.hpp"
#include <iostream>
#include <sstream>
#include "World/Chunk.hpp"
#include "Debug/Logger.hpp"
#include "Debug/DebugWindowUtility.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <fstream>
#include "../../third_party/nlohmann/json.hpp"

using json = nlohmann::json;

DebugWindow::DebugWindow(DebugManager& debugMgr, Player& p) 
    : debugManager(debugMgr), 
      player(p), 
      visible(false), 
      showMeshDebug(false), 
      showLoggingConfig(false),
      showPerformance(true),
      showGodView(false) {
    
    // Load window state (visibility, panel states)
    loadWindowState();
    
    // Sync UI state with actual debug settings
    syncWithDebugManager();
    
    // Create the god view debug tool
    godViewTool = new GodViewDebugTool(player.getWorld());

    // Log initialization
    LOG_INFO(LogCategory::UI, "Debug Window initialized");
}

DebugWindow::~DebugWindow() {
    if (godViewTool) {
        delete godViewTool;
        godViewTool = nullptr;
    }
    LOG_INFO(LogCategory::UI, "Debug Window destroyed");
}

// Helper function to set a block at world coordinates
void DebugWindow::setBlockHelper(int x, int y, int z, BlockType type) {
    // Use const_cast to get a non-const reference to the world
    // This is safe because we know the world object is not actually const
    // and we're only using this for debugging purposes
    World& worldRef = const_cast<World&>(player.getWorld());
    
    // Call setBlock on the non-const reference
    worldRef.setBlock(x, y, z, type);
    
    // Log the action using the new logging system
    std::stringstream ss;
    ss << (type == BlockType::AIR ? "Removed" : "Placed")
       << " block at position (" << x << ", " << y << ", " << z << ")";
    LOG_DEBUG(LogCategory::WORLD, ss.str());
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

void DebugWindow::render() {
    if (!visible) return;

    ImGui::Begin("Debug Tools", &visible);
    
    if (ImGui::BeginTabBar("DebugTabs")) {
        // Visualization tab
        if (ImGui::BeginTabItem("Visualization")) {
            renderVisualizationPanel();
            ImGui::EndTabItem();
        }
        
        // Logging tab
        if (ImGui::BeginTabItem("Logging")) {
            renderLoggingPanel();
            ImGui::EndTabItem();
        }
        
        // World Debug tab
        if (ImGui::BeginTabItem("World Debug")) {
            renderWorldDebugPanel();
            ImGui::EndTabItem();
        }
        
        // Performance tab
        if (ImGui::BeginTabItem("Performance")) {
            renderPerformancePanel();
            ImGui::EndTabItem();
        }
        
        // Player Info tab
        if (ImGui::BeginTabItem("Player Info")) {
            renderPlayerInfoPanel();
            ImGui::EndTabItem();
        }
        
        // God View tab
        if (ImGui::BeginTabItem("God View")) {
            showGodView = true;
            renderGodViewPanel();
            ImGui::EndTabItem();
        } else {
            // Only disable god view when not in this tab
            if (showGodView) {
                showGodView = false;
                if (godViewTool) godViewTool->setActive(false);
            }
        }
        
        ImGui::EndTabBar();
    }
    
    // Add save & restore buttons
    ImGui::Separator();
    if (ImGui::Button("Save Settings")) {
        debugManager.saveSettings();
        saveWindowState();
        LOG_INFO(LogCategory::GENERAL, "Debug settings saved manually");
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Reset to Defaults")) {
        // Reset all debug settings to defaults
        debugManager.setCullingEnabled(true);
        debugManager.setShowVoxelEdges(false);
        debugManager.setUseFaceColors(false);
        debugManager.setDebugVertexScaling(false);
        debugManager.setLogPlayerInfo(false);
        debugManager.setLogRaycast(false);
        debugManager.setLogChunkUpdates(false);
        debugManager.setLogBlockPlacement(false);
        debugManager.setLogCollision(false);
        debugManager.setLogInventory(false);
        debugManager.setLogLevel(LogLevel::INFO);
        
        // Resync UI state
        syncWithDebugManager();
        
        LOG_INFO(LogCategory::GENERAL, "Debug settings reset to defaults");
    }
    
    ImGui::End();
    
    // Save window state when closing
    if (!visible) {
        saveWindowState();
    }
    
    // Update God View if auto-rotate is enabled
    if (godViewTool && godViewTool->isActive() && godViewAutoRotate) {
        godViewTool->rotateView(godViewRotationSpeed);
        godViewRotation = fmod(godViewRotation + godViewRotationSpeed, 360.0f);
    }
}

void DebugWindow::renderVisualizationPanel() {
    ImGui::Text("Visualization Options");
    
    // General debug toggles
    bool showEdges = debugManager.showVoxelEdges();
    if (ImGui::Checkbox("Show Voxel Edges", &showEdges)) {
        debugManager.setShowVoxelEdges(showEdges);
    }

    bool culling = debugManager.isCullingEnabled();
    if (ImGui::Checkbox("Enable Culling", &culling)) {
        debugManager.setCullingEnabled(culling);
    }

    bool faceColors = debugManager.useFaceColors();
    if (ImGui::Checkbox("Use Face Colors", &faceColors)) {
        debugManager.setUseFaceColors(faceColors);
    }

    // Vertex scaling option - for debugging
    bool vertexScaling = debugManager.debugVertexScaling();
    if (ImGui::Checkbox("Debug Vertex Scaling", &vertexScaling)) {
        debugManager.setDebugVertexScaling(vertexScaling);
    }
    
    // Show mesh debug tools
    if (ImGui::Checkbox("Show Mesh Debugging", &showMeshDebug)) {
        saveWindowState(); // Save UI state when toggling panels
    }
    
    if (showMeshDebug) {
        ImGui::Separator();
        ImGui::Text("Mesh Debugging Tools");
        
        if (ImGui::Button("Regenerate All Meshes")) {
            // Access the world via const_cast (for debug purposes only)
            World& worldRef = const_cast<World&>(player.getWorld());
            
            // Force regenerate all chunks
            int chunksRegenerated = 0;
            for (auto& [key, chunk] : worldRef.getChunks()) {
                chunk->markMeshDirty();
                chunk->regenerateMesh();
                
                // Update buffers
                if (chunk->isBuffersInitialized()) {
                    chunk->updateBuffers();
                } else {
                    chunk->initializeBuffers();
                }
                chunksRegenerated++;
            }
            
            std::stringstream ss;
            ss << "Forcibly regenerated " << chunksRegenerated << " chunk meshes";
            LOG_INFO(LogCategory::RENDERING, ss.str());
        }

        // Add a button to check specific mesh data
        if (ImGui::Button("Print Active Chunk Data")) {
            // Get the player's current chunk
            int px = static_cast<int>(floor(player.position.x / static_cast<float>(Chunk::SIZE)));
            int py = static_cast<int>(floor(player.position.y / static_cast<float>(Chunk::SIZE)));
            int pz = static_cast<int>(floor(player.position.z / static_cast<float>(Chunk::SIZE)));
            
            // Access the world via const_cast
            World& worldRef = const_cast<World&>(player.getWorld());
            
            // Find and print chunk info
            auto it = worldRef.getChunks().find(std::make_tuple(px, py, pz, 1));
            if (it != worldRef.getChunks().end()) {
                auto& chunk = it->second;
                const std::vector<float>& mesh = chunk->getMesh();
                
                std::stringstream ss;
                ss << "Current chunk (" << px << ", " << py << ", " << pz << ") contains "
                   << mesh.size() / 5 << " vertices";
                LOG_DEBUG(LogCategory::RENDERING, ss.str());
                
                // Print first few vertices for inspection
                if (mesh.size() >= 20) {
                    std::stringstream vertexInfo;
                    vertexInfo << "Sample vertices:"
                              << "\n  First: " << mesh[0] << ", " << mesh[1] << ", " << mesh[2]
                              << "\n  Second: " << mesh[5] << ", " << mesh[6] << ", " << mesh[7]
                              << "\n  Third: " << mesh[10] << ", " << mesh[11] << ", " << mesh[12]
                              << "\n  Fourth: " << mesh[15] << ", " << mesh[16] << ", " << mesh[17];
                    LOG_DEBUG(LogCategory::RENDERING, vertexInfo.str());
                }
            } else {
                std::stringstream ss;
                ss << "Current chunk (" << px << ", " << py << ", " << pz << ") not found!";
                LOG_WARNING(LogCategory::RENDERING, ss.str());
            }
        }
    }
}

void DebugWindow::renderLoggingPanel() {
    ImGui::Text("Logging Configuration");
    
    // Log level selection
    if (ImGui::Combo("Log Level", &currentLogLevel, logLevelNames, IM_ARRAYSIZE(logLevelNames))) {
        LogLevel level = static_cast<LogLevel>(currentLogLevel);
        debugManager.setLogLevel(level);
    }
    
    ImGui::Separator();
    ImGui::Text("Log Categories");
    
    // Update category states from Logger
    for (int i = 0; i < 7; i++) {
        categoryEnabled[i] = Logger::getInstance().isCategoryEnabled(static_cast<LogCategory>(i));
    }
    
    // Category toggles
    for (int i = 0; i < 7; i++) {
        if (ImGui::Checkbox(categoryNames[i], &categoryEnabled[i])) {
            Logger::getInstance().setCategoryEnabled(
                static_cast<LogCategory>(i), categoryEnabled[i]);
                
            // For backward compatibility, set the legacy flags too
            switch (static_cast<LogCategory>(i)) {
                case LogCategory::PLAYER:
                    debugManager.setLogPlayerInfo(categoryEnabled[i]);
                    break;
                case LogCategory::WORLD:
                    debugManager.setLogChunkUpdates(categoryEnabled[i]);
                    debugManager.setLogBlockPlacement(categoryEnabled[i]);
                    break;
                case LogCategory::PHYSICS:
                    debugManager.setLogCollision(categoryEnabled[i]);
                    debugManager.setLogRaycast(categoryEnabled[i]);
                    break;
                case LogCategory::UI:
                    debugManager.setLogInventory(categoryEnabled[i]);
                    break;
                default:
                    break;
            }
        }
    }
    
    ImGui::Separator();
    
    // Legacy log toggles (for backward compatibility)
    ImGui::Text("Legacy Log Toggles");
    
    bool logPlayer = debugManager.logPlayerInfo();
    if (ImGui::Checkbox("Log Player Info", &logPlayer)) {
        debugManager.setLogPlayerInfo(logPlayer);
    }

    bool logRaycast = debugManager.logRaycast();
    if (ImGui::Checkbox("Log Raycast", &logRaycast)) {
        debugManager.setLogRaycast(logRaycast);
    }

    bool logChunks = debugManager.logChunkUpdates();
    if (ImGui::Checkbox("Log Chunk Updates", &logChunks)) {
        debugManager.setLogChunkUpdates(logChunks);
    }

    bool logBlocks = debugManager.logBlockPlacement();
    if (ImGui::Checkbox("Log Block Placement", &logBlocks)) {
        debugManager.setLogBlockPlacement(logBlocks);
    }

    bool logCollision = debugManager.logCollision();
    if (ImGui::Checkbox("Log Collision", &logCollision)) {
        debugManager.setLogCollision(logCollision);
    }

    bool logInventory = debugManager.logInventory();
    if (ImGui::Checkbox("Log Inventory", &logInventory)) {
        debugManager.setLogInventory(logInventory);
    }
}

void DebugWindow::renderWorldDebugPanel() {
    // Add a section for terrain debug
    ImGui::Text("Terrain Debug");
    
    // Show sphere projection parameters
    static float debugRadius = 6371.0f + 8.0f; // Surface radius
    ImGui::SliderFloat("Surface Radius", &debugRadius, 6371.0f, 6400.0f);
    
    // Button to force regenerate chunks around player
    if (ImGui::Button("Regenerate All Chunks")) {
        // Store the player's current chunk coordinates
        int px = static_cast<int>(floor(player.position.x / static_cast<float>(Chunk::SIZE)));
        int py = static_cast<int>(floor(player.position.y / static_cast<float>(Chunk::SIZE)));
        int pz = static_cast<int>(floor(player.position.z / static_cast<float>(Chunk::SIZE)));
        
        // Log the regeneration attempt
        std::stringstream ss;
        ss << "Regenerating all chunks around player at chunk coords: (" 
           << px << ", " << py << ", " << pz << ")";
        LOG_INFO(LogCategory::WORLD, ss.str());
        
        // Access the world via const_cast (for debug purposes only)
        World& worldRef = const_cast<World&>(player.getWorld());
        
        // Force regenerate all chunks in a 3x3x3 grid around player
        for (int x = px - 1; x <= px + 1; x++) {
            for (int y = py - 1; y <= py + 1; y++) {
                for (int z = pz - 1; z <= pz + 1; z++) {
                    auto key = std::make_tuple(x, y, z, 1);
                    auto& chunks = worldRef.getChunks();
                    auto it = chunks.find(key);
                    if (it != chunks.end()) {
                        std::stringstream chunkLog;
                        chunkLog << "Regenerating chunk at (" << x << ", " << y << ", " << z << ")";
                        LOG_DEBUG(LogCategory::WORLD, chunkLog.str());
                        
                        it->second->markMeshDirty();
                        it->second->regenerateMesh();
                        
                        // Make sure the buffers get updated
                        if (it->second->isBuffersInitialized()) {
                            it->second->updateBuffers();
                        } else {
                            it->second->initializeBuffers();
                        }
                    }
                }
            }
        }
    }
    
    // Add debug toggle for sphere projection algorithm
    static bool useAdvancedProjection = true;
    if (ImGui::Checkbox("Use Advanced Sphere Projection", &useAdvancedProjection)) {
        LOG_INFO(LogCategory::RENDERING, std::string("Advanced sphere projection ") + 
                 (useAdvancedProjection ? "enabled" : "disabled"));
    }
    
    // Teleport tool
    ImGui::Separator();
    ImGui::Text("Teleport Tool");
    ImGui::InputFloat3("Coordinates (X, Y, Z)", teleportCoords);
    if (ImGui::Button("Teleport")) {
        player.position = glm::vec3(teleportCoords[0], teleportCoords[1], teleportCoords[2]);
        
        std::stringstream ss;
        ss << "Teleported to (" << player.position.x << ", " 
           << player.position.y << ", " << player.position.z << ")";
        LOG_INFO(LogCategory::PLAYER, ss.str());
        
        // Save teleport coordinates for next session
        saveWindowState();
    }
    
    ImGui::Separator();
    ImGui::Text("Block Manipulation");
    
    // Add button to add a test block directly in front of player
    if (ImGui::Button("Place Test Block (GRASS)")) {
        // Calculate position 3 units in front of player
        glm::vec3 dirNormalized = glm::normalize(player.cameraDirection);
        glm::vec3 blockPos = player.position + dirNormalized * 3.0f;
        
        // Round to integer coordinates
        int blockX = static_cast<int>(floor(blockPos.x));
        int blockY = static_cast<int>(floor(blockPos.y));
        int blockZ = static_cast<int>(floor(blockPos.z));
        
        // Place the block and force update
        std::stringstream ss;
        ss << "Placing test GRASS block at: " << blockX << ", " << blockY << ", " << blockZ;
        LOG_INFO(LogCategory::WORLD, ss.str());
        setBlockHelper(blockX, blockY, blockZ, BlockType::GRASS);
    }
    
    // Button to remove a test block
    if (ImGui::Button("Remove Test Block")) {
        // Calculate position 3 units in front of player
        glm::vec3 dirNormalized = glm::normalize(player.cameraDirection);
        glm::vec3 blockPos = player.position + dirNormalized * 3.0f;
        
        // Round to integer coordinates
        int blockX = static_cast<int>(floor(blockPos.x));
        int blockY = static_cast<int>(floor(blockPos.y));
        int blockZ = static_cast<int>(floor(blockPos.z));
        
        // Remove the block and force update
        std::stringstream ss;
        ss << "Removing block at: " << blockX << ", " << blockY << ", " << blockZ;
        LOG_INFO(LogCategory::WORLD, ss.str());
        setBlockHelper(blockX, blockY, blockZ, BlockType::AIR);
    }
}

void DebugWindow::renderPerformancePanel() {
    ImGui::Text("Performance Metrics");
    
    // Calculate current frame time
    static float lastTime = ImGui::GetTime();
    float currentTime = ImGui::GetTime();
    float frameTime = currentTime - lastTime;
    lastTime = currentTime;
    
    // Update frame time history
    frameTimeHistory[frameTimeIndex] = frameTime;
    frameTimeIndex = (frameTimeIndex + 1) % IM_ARRAYSIZE(frameTimeHistory);
    
    // Calculate statistics
    minFrameTime = frameTime;
    maxFrameTime = frameTime;
    avgFrameTime = 0.0f;
    int numSamples = 0;
    
    for (int i = 0; i < IM_ARRAYSIZE(frameTimeHistory); i++) {
        if (frameTimeHistory[i] <= 0.0f) continue; // Skip uninitialized values
        
        minFrameTime = std::min(minFrameTime, frameTimeHistory[i]);
        maxFrameTime = std::max(maxFrameTime, frameTimeHistory[i]);
        avgFrameTime += frameTimeHistory[i];
        numSamples++;
    }
    
    if (numSamples > 0) avgFrameTime /= numSamples;
    
    // Display statistics
    ImGui::Text("Frame Time: %.2f ms (%.1f FPS)", frameTime * 1000.0f, 1.0f / frameTime);
    ImGui::Text("Min: %.2f ms, Max: %.2f ms, Avg: %.2f ms", 
                minFrameTime * 1000.0f, 
                maxFrameTime * 1000.0f, 
                avgFrameTime * 1000.0f);
    
    // Plot frame times
    ImGui::PlotLines("Frame Times", 
                     frameTimeHistory, 
                     IM_ARRAYSIZE(frameTimeHistory), 
                     frameTimeIndex, 
                     "Frame Time (ms)", 
                     0.0f, 
                     maxFrameTime * 1.2f, 
                     ImVec2(0, 80.0f));
    
    // Memory usage (placeholder - would need actual memory tracking)
    ImGui::Text("Memory Usage: Unknown");
}

void DebugWindow::renderPlayerInfoPanel() {
    ImGui::Text("Player Information");
    
    // Position
    ImGui::Text("Position: %.2f, %.2f, %.2f", 
                player.position.x, player.position.y, player.position.z);
    
    // Direction
    ImGui::Text("Camera Direction: %.2f, %.2f, %.2f", 
                player.cameraDirection.x, player.cameraDirection.y, player.cameraDirection.z);
    
    // Up vector
    ImGui::Text("Up Vector: %.2f, %.2f, %.2f", 
                player.up.x, player.up.y, player.up.z);
    
    // Calculate distance from center
    float distFromCenter = glm::length(player.position);
    float surfaceR = static_cast<float>(player.getWorld().getSurfaceRadius());
    float heightAboveSurface = distFromCenter - surfaceR;
    
    ImGui::Text("Distance from center: %.2f", distFromCenter);
    ImGui::Text("Height above surface: %.2f meters", heightAboveSurface);
    
    // Get world chunk coordinates
    int chunkX = static_cast<int>(floor(player.position.x / static_cast<float>(Chunk::SIZE)));
    int chunkY = static_cast<int>(floor(player.position.y / static_cast<float>(Chunk::SIZE)));
    int chunkZ = static_cast<int>(floor(player.position.z / static_cast<float>(Chunk::SIZE)));
    
    ImGui::Text("Chunk coordinates: %d, %d, %d", chunkX, chunkY, chunkZ);
    
    // Local position within chunk
    float localX = player.position.x - chunkX * Chunk::SIZE;
    float localY = player.position.y - chunkY * Chunk::SIZE;
    float localZ = player.position.z - chunkZ * Chunk::SIZE;
    
    ImGui::Text("Local position in chunk: %.2f, %.2f, %.2f", localX, localY, localZ);
    
    // Inventory
    ImGui::Separator();
    ImGui::Text("Inventory");
    ImGui::Text("Selected Slot: %d", player.inventory.selectedSlot);
    ImGui::Text("Selected Block: %d", static_cast<int>(player.inventory.slots[player.inventory.selectedSlot]));
}

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
        
        // Dump debugging info
        DebugWindowUtility::dumpGodViewState(godViewTool);
    }
    
    // Add a force-activate button for emergency debugging
    if (ImGui::Button("Force Activate (Debug)")) {
        DebugWindowUtility::forceActivateGodView(godViewTool);
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

void DebugWindow::saveWindowState() {
    try {
        json state;
        
        // Save window visibility
        state["visible"] = visible;
        
        // Save panel states
        state["panels"] = {
            {"showMeshDebug", showMeshDebug},
            {"showLoggingConfig", showLoggingConfig},
            {"showPerformance", showPerformance},
            {"showGodView", showGodView}
        };
        
        // Save teleport coordinates
        state["teleportCoords"] = {
            teleportCoords[0],
            teleportCoords[1],
            teleportCoords[2]
        };
        
        // Save god view state
        state["godView"] = {
            {"active", godViewTool ? godViewTool->isActive() : false},
            {"cameraPos", {godViewCameraPos[0], godViewCameraPos[1], godViewCameraPos[2]}},
            {"cameraTarget", {godViewCameraTarget[0], godViewCameraTarget[1], godViewCameraTarget[2]}},
            {"zoom", godViewZoom},
            {"rotation", godViewRotation},
            {"wireframe", godViewWireframe},
            {"visualizationType", godViewVisualizationType},
            {"autoRotate", godViewAutoRotate},
            {"rotationSpeed", godViewRotationSpeed}
        };
        
        // Write to file
        std::ofstream file("debug_window_state.json");
        if (file.is_open()) {
            file << state.dump(4);
        }
    } catch (const std::exception& e) {
        LOG_ERROR(LogCategory::UI, "Error saving debug window state: " + std::string(e.what()));
    }
}

void DebugWindow::loadWindowState() {
    try {
        std::ifstream file("debug_window_state.json");
        if (!file.is_open()) {
            return; // No state file exists yet
        }
        
        json state;
        file >> state;
        
        // Load window visibility
        if (state.contains("visible")) {
            visible = state["visible"].get<bool>();
        }
        
        // Load panel states
        if (state.contains("panels")) {
            auto& panels = state["panels"];
            if (panels.contains("showMeshDebug")) showMeshDebug = panels["showMeshDebug"].get<bool>();
            if (panels.contains("showLoggingConfig")) showLoggingConfig = panels["showLoggingConfig"].get<bool>();
            if (panels.contains("showPerformance")) showPerformance = panels["showPerformance"].get<bool>();
            if (panels.contains("showGodView")) showGodView = panels["showGodView"].get<bool>();
        }
        
        // Load teleport coordinates
        if (state.contains("teleportCoords")) {
            auto& coords = state["teleportCoords"];
            if (coords.is_array() && coords.size() == 3) {
                teleportCoords[0] = coords[0].get<float>();
                teleportCoords[1] = coords[1].get<float>();
                teleportCoords[2] = coords[2].get<float>();
            }
        }
        
        // Load god view state
        if (state.contains("godView")) {
            auto& godView = state["godView"];
            
            if (godView.contains("cameraPos")) {
                auto& camPos = godView["cameraPos"];
                if (camPos.is_array() && camPos.size() == 3) {
                    godViewCameraPos[0] = camPos[0].get<float>();
                    godViewCameraPos[1] = camPos[1].get<float>();
                    godViewCameraPos[2] = camPos[2].get<float>();
                }
            }
            
            if (godView.contains("cameraTarget")) {
                auto& camTarget = godView["cameraTarget"];
                if (camTarget.is_array() && camTarget.size() == 3) {
                    godViewCameraTarget[0] = camTarget[0].get<float>();
                    godViewCameraTarget[1] = camTarget[1].get<float>();
                    godViewCameraTarget[2] = camTarget[2].get<float>();
                }
            }
            
            if (godView.contains("zoom")) godViewZoom = godView["zoom"].get<float>();
            if (godView.contains("rotation")) godViewRotation = godView["rotation"].get<float>();
            if (godView.contains("wireframe")) godViewWireframe = godView["wireframe"].get<bool>();
            if (godView.contains("visualizationType")) godViewVisualizationType = godView["visualizationType"].get<int>();
            if (godView.contains("autoRotate")) godViewAutoRotate = godView["autoRotate"].get<bool>();
            if (godView.contains("rotationSpeed")) godViewRotationSpeed = godView["rotationSpeed"].get<float>();
            
            // Apply settings to the god view tool
            if (godViewTool) {
                godViewTool->setCameraPosition(glm::vec3(godViewCameraPos[0], godViewCameraPos[1], godViewCameraPos[2]));
                godViewTool->setCameraTarget(glm::vec3(godViewCameraTarget[0], godViewCameraTarget[1], godViewCameraTarget[2]));
                godViewTool->setZoom(godViewZoom);
                godViewTool->rotateView(godViewRotation);
                godViewTool->setWireframeMode(godViewWireframe);
                godViewTool->setVisualizationType(godViewVisualizationType);
                
                // Set active state last
                if (godView.contains("active")) {
                    godViewTool->setActive(godView["active"].get<bool>() && showGodView);
                }
            }
        }
        
        LOG_DEBUG(LogCategory::UI, "Debug window state loaded");
    } catch (const std::exception& e) {
        LOG_ERROR(LogCategory::UI, "Error loading debug window state: " + std::string(e.what()));
    }
}

void DebugWindow::syncWithDebugManager() {
    // Read values from DebugManager to update UI state
    
    // Log level
    currentLogLevel = static_cast<int>(Logger::getInstance().getMinLogLevel());
    
    // Category enabled states
    for (int i = 0; i < 7; i++) {
        categoryEnabled[i] = Logger::getInstance().isCategoryEnabled(static_cast<LogCategory>(i));
    }
}

void DebugWindow::toggleVisibility() {
    visible = !visible;
    LOG_INFO(LogCategory::UI, std::string("Debug window ") + (visible ? "shown" : "hidden"));
}