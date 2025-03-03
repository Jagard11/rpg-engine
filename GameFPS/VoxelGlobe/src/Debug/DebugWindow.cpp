// ./src/Debug/DebugWindow.cpp
#include "Debug/DebugWindow.hpp"
#include <iostream>

DebugWindow::DebugWindow(DebugManager& debugMgr) : debugManager(debugMgr), visible(false) {}

void DebugWindow::render(const Player& player) {
    if (!visible) return;

    ImGui::Begin("Debug Tools", &visible, ImGuiWindowFlags_AlwaysAutoResize);

    // General debug toggles
    bool showEdges = debugManager.showVoxelEdges();
    if (ImGui::Checkbox("Show Voxel Edges", &showEdges)) {
        debugManager.setShowVoxelEdges(showEdges);
        if (debugManager.logChunkUpdates()) std::cout << "Voxel Edges toggled: " << (showEdges ? "ON" : "OFF") << std::endl;
    }

    bool culling = debugManager.isCullingEnabled();
    if (ImGui::Checkbox("Enable Culling", &culling)) {
        debugManager.setCullingEnabled(culling);
        if (debugManager.logChunkUpdates()) std::cout << "Culling toggled: " << (culling ? "ON" : "OFF") << std::endl;
    }

    bool faceColors = debugManager.useFaceColors();
    if (ImGui::Checkbox("Use Face Colors", &faceColors)) {
        debugManager.setUseFaceColors(faceColors);
        if (debugManager.logChunkUpdates()) std::cout << "Face Colors toggled: " << (faceColors ? "ON" : "OFF") << std::endl;
    }

    // Log toggles
    ImGui::Separator();
    ImGui::Text("Debug Logs");
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

    bool logInventory = debugManager.logInventory(); // Added new toggle
    if (ImGui::Checkbox("Log Inventory", &logInventory)) {
        debugManager.setLogInventory(logInventory);
    }

    // Display player info if enabled
    if (debugManager.logPlayerInfo()) {
        ImGui::Separator();
        ImGui::Text("Player Pos: %.2f, %.2f, %.2f", player.position.x, player.position.y, player.position.z);
        ImGui::Text("Camera Dir: %.2f, %.2f, %.2f", player.cameraDirection.x, player.cameraDirection.y, player.cameraDirection.z);
    }

    ImGui::End();
}