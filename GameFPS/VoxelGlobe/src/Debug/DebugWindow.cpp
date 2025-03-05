// ./src/Debug/DebugWindow.cpp
#include "Debug/DebugWindow.hpp"
#include <iostream>
#include "World/Chunk.hpp"

DebugWindow::DebugWindow(DebugManager& debugMgr, Player& p) 
    : debugManager(debugMgr), player(p), visible(false), showMeshDebug(false) {}

// Helper function to set a block at world coordinates
void DebugWindow::setBlockHelper(int x, int y, int z, BlockType type) {
    // Use const_cast to get a non-const reference to the world
    // This is safe because we know the world object is not actually const
    // and we're only using this for debugging purposes
    World& worldRef = const_cast<World&>(player.getWorld());
    
    // Call setBlock on the non-const reference
    worldRef.setBlock(x, y, z, type);
    
    // Log the action
    std::cout << "Debug: " << (type == BlockType::AIR ? "Removed" : "Placed") 
              << " block at position (" << x << ", " << y << ", " << z << ")" << std::endl;
}

void DebugWindow::render() {
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

    bool logInventory = debugManager.logInventory();
    if (ImGui::Checkbox("Log Inventory", &logInventory)) {
        debugManager.setLogInventory(logInventory);
    }

    // Teleport tool
    ImGui::Separator();
    ImGui::Text("Teleport Tool");
    ImGui::InputFloat3("Coordinates (X, Y, Z)", teleportCoords);
    if (ImGui::Button("Teleport")) {
        player.position = glm::vec3(teleportCoords[0], teleportCoords[1], teleportCoords[2]);
        if (debugManager.logPlayerInfo()) {
            std::cout << "Teleported to (" << player.position.x << ", " << player.position.y << ", " << player.position.z << ")" << std::endl;
        }
    }

    // Display player info if enabled
    if (debugManager.logPlayerInfo()) {
        ImGui::Separator();
        ImGui::Text("Player Pos: %.2f, %.2f, %.2f", player.position.x, player.position.y, player.position.z);
        ImGui::Text("Camera Dir: %.2f, %.2f, %.2f", player.cameraDirection.x, player.cameraDirection.y, player.cameraDirection.z);
    }

    // Add a new section for terrain debug
    ImGui::Separator();
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
        std::cout << "Regenerating all chunks around player at chunk coords: (" 
                  << px << ", " << py << ", " << pz << ")" << std::endl;
        
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
                        std::cout << "Regenerating chunk at (" << x << ", " << y << ", " << z << ")" << std::endl;
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
        // Set a global flag to control which projection algorithm to use
        // Note: implementation would need a global flag to actually toggle between algorithms
    }
    
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
        std::cout << "Placing test GRASS block at: " << blockX << ", " << blockY << ", " << blockZ << std::endl;
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
        std::cout << "Removing block at: " << blockX << ", " << blockY << ", " << blockZ << std::endl;
        setBlockHelper(blockX, blockY, blockZ, BlockType::AIR);
    }

    // Add a new section for mesh debugging
    ImGui::Separator();
    ImGui::Text("Mesh Debugging");

    // Vertex scaling option - for debugging
    bool vertexScaling = debugManager.debugVertexScaling();
    if (ImGui::Checkbox("Debug Vertex Scaling", &vertexScaling)) {
        debugManager.setDebugVertexScaling(vertexScaling);
        std::cout << "Mesh scaling debugging " << (vertexScaling ? "enabled" : "disabled") << std::endl;
    }

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
        
        std::cout << "Forcibly regenerated " << chunksRegenerated << " chunk meshes" << std::endl;
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
            
            std::cout << "Current chunk (" << px << ", " << py << ", " << pz << ") contains:" << std::endl;
            std::cout << "  - " << mesh.size() / 5 << " vertices" << std::endl;
            
            // Print first few vertices for inspection
            if (mesh.size() >= 20) {
                std::cout << "  - First vertex: " << mesh[0] << ", " << mesh[1] << ", " << mesh[2] << std::endl;
                std::cout << "  - Second vertex: " << mesh[5] << ", " << mesh[6] << ", " << mesh[7] << std::endl;
                std::cout << "  - Third vertex: " << mesh[10] << ", " << mesh[11] << ", " << mesh[12] << std::endl;
                std::cout << "  - Fourth vertex: " << mesh[15] << ", " << mesh[16] << ", " << mesh[17] << std::endl;
            }
        } else {
            std::cout << "Current chunk (" << px << ", " << py << ", " << pz << ") not found!" << std::endl;
        }
    }

    ImGui::End();
}