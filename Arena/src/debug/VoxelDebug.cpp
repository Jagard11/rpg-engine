#include "debug/VoxelDebug.hpp"
#include "core/StackTrace.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace Debug {

// Static member initialization
bool VoxelDebug::s_initialized = false;
std::chrono::steady_clock::time_point VoxelDebug::s_startTime;

void VoxelDebug::initialize() {
    // Create the debug directory if it doesn't exist
    std::filesystem::create_directories(DEBUG_DIR);
    
    // Record the start time
    s_startTime = std::chrono::steady_clock::now();
    
    // Clear any existing traces
    Core::StackTrace::clearTraces();
    
    s_initialized = true;
    std::cout << "VoxelDebug system initialized. Press F12 to dump debug info." << std::endl;
}

void VoxelDebug::dumpDebugInfo(World* world, Player* player) {
    if (!s_initialized) {
        initialize();
    }
    
    // Generate a filename with the current timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream filename;
    filename << DEBUG_DIR << "/voxel_debug_" 
             << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".log";
    
    // Open the debug file
    std::ofstream debugFile(filename.str());
    if (!debugFile.is_open()) {
        std::cerr << "Failed to open debug file: " << filename.str() << std::endl;
        return;
    }
    
    // Write basic information
    debugFile << "===== Voxel Engine Debug Dump =====" << std::endl;
    debugFile << "Timestamp: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << std::endl;
    debugFile << "Uptime: " << std::chrono::duration_cast<std::chrono::seconds>(
                                std::chrono::steady_clock::now() - s_startTime).count() << " seconds" << std::endl;
    debugFile << std::endl;
    
    // Player information
    if (player) {
        glm::vec3 playerPos = player->getPosition();
        glm::vec3 playerMin = player->getMinBounds();
        glm::vec3 playerMax = player->getMaxBounds();
        
        debugFile << "----- Player Information -----" << std::endl;
        debugFile << "Position: (" << playerPos.x << ", " << playerPos.y << ", " << playerPos.z << ")" << std::endl;
        debugFile << "Bounds: Min(" << playerMin.x << ", " << playerMin.y << ", " << playerMin.z << ") "
                  << "Max(" << playerMax.x << ", " << playerMax.y << ", " << playerMax.z << ")" << std::endl;
        
        // Calculate which chunk the player is in
        if (world) {
            glm::ivec3 chunkPos = world->worldToChunkPos(playerPos);
            glm::ivec3 localPos = world->worldToLocalPos(playerPos);
            
            debugFile << "Chunk: (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ")" << std::endl;
            debugFile << "Local position in chunk: (" << localPos.x << ", " << localPos.y << ", " << localPos.z << ")" << std::endl;
            
            // Check if player is at a chunk boundary
            bool isAtChunkBoundary = 
                (int)playerPos.x % World::CHUNK_SIZE == 0 || 
                (int)playerPos.x % World::CHUNK_SIZE == World::CHUNK_SIZE - 1 ||
                (int)playerPos.y % World::CHUNK_HEIGHT == 0 || 
                (int)playerPos.y % World::CHUNK_HEIGHT == World::CHUNK_HEIGHT - 1 ||
                (int)playerPos.z % World::CHUNK_SIZE == 0 || 
                (int)playerPos.z % World::CHUNK_SIZE == World::CHUNK_SIZE - 1;
                
            debugFile << "At chunk boundary: " << (isAtChunkBoundary ? "YES" : "NO") << std::endl;
            
            // Check surrounding blocks - log blocks around the player
            debugFile << "Surrounding blocks:" << std::endl;
            for (int y = -1; y <= 2; y++) {
                for (int z = -1; z <= 1; z++) {
                    for (int x = -1; x <= 1; x++) {
                        glm::ivec3 blockPos = glm::ivec3(
                            std::floor(playerPos.x) + x,
                            std::floor(playerPos.y) + y,
                            std::floor(playerPos.z) + z
                        );
                        
                        int blockType = world->getBlock(blockPos);
                        
                        // Is this block at a chunk boundary?
                        glm::ivec3 blockChunkPos = world->worldToChunkPos(glm::vec3(blockPos));
                        glm::ivec3 blockLocalPos = world->worldToLocalPos(glm::vec3(blockPos));
                        bool isBlockAtBoundary = 
                            blockLocalPos.x == 0 || blockLocalPos.x == World::CHUNK_SIZE - 1 ||
                            blockLocalPos.y == 0 || blockLocalPos.y == World::CHUNK_HEIGHT - 1 ||
                            blockLocalPos.z == 0 || blockLocalPos.z == World::CHUNK_SIZE - 1;
                        
                        debugFile << "  [" << x << "," << y << "," << z << "] "
                                  << "World(" << blockPos.x << "," << blockPos.y << "," << blockPos.z << ") "
                                  << "Chunk(" << blockChunkPos.x << "," << blockChunkPos.y << "," << blockChunkPos.z << ") "
                                  << "Local(" << blockLocalPos.x << "," << blockLocalPos.y << "," << blockLocalPos.z << ") "
                                  << "Type: " << blockType;
                                  
                        if (isBlockAtBoundary) {
                            debugFile << " [BOUNDARY]";
                        }
                        debugFile << std::endl;
                    }
                }
            }
        }
        
        debugFile << std::endl;
    }
    
    // World information
    if (world) {
        debugFile << "----- World Information -----" << std::endl;
        
        // Count loaded chunks
        const auto& chunks = world->getChunks();
        debugFile << "Number of loaded chunks: " << chunks.size() << std::endl;
        
        // Sample a few chunks near the player if available
        if (player) {
            glm::ivec3 playerChunkPos = world->worldToChunkPos(player->getPosition());
            
            debugFile << "Chunks around player:" << std::endl;
            for (int y = -1; y <= 1; y++) {
                for (int z = -1; z <= 1; z++) {
                    for (int x = -1; x <= 1; x++) {
                        glm::ivec3 chunkPos = playerChunkPos + glm::ivec3(x, y, z);
                        auto it = chunks.find(chunkPos);
                        if (it != chunks.end()) {
                            const auto& chunk = it->second;
                            debugFile << "  Chunk(" << chunkPos.x << "," << chunkPos.y << "," << chunkPos.z << ") - "
                                      << "Vertices: " << chunk->getMeshVertices().size() / 5 << ", "
                                      << "Indices: " << chunk->getMeshIndices().size() << ", "
                                      << "Modified: " << (chunk->isModified() ? "YES" : "NO") << ", "
                                      << "Dirty: " << (chunk->isDirty() ? "YES" : "NO") << std::endl;
                        } else {
                            debugFile << "  Chunk(" << chunkPos.x << "," << chunkPos.y << "," << chunkPos.z << ") - Not loaded" << std::endl;
                        }
                    }
                }
            }
        }
        
        debugFile << std::endl;
    }
    
    // Close the main debug file
    debugFile.close();
    
    // Dump the stack traces to a separate file
    std::string traceFilename = filename.str() + ".traces";
    Core::StackTrace::dumpTracesToFile(traceFilename);
    
    std::cout << "Debug information dumped to " << filename.str() << std::endl;
    std::cout << "Stack traces dumped to " << traceFilename << std::endl;
}

void VoxelDebug::recordVoxelOperation(World* world, const glm::ivec3& blockPos, bool success, const std::string& action) {
    if (!s_initialized) {
        initialize();
    }
    
    // Create a context string for this operation
    std::stringstream context;
    context << action << " voxel at (" << blockPos.x << "," << blockPos.y << "," << blockPos.z << ") - " 
            << (success ? "SUCCESS" : "FAILED");
    
    // If world is available, add chunk information
    if (world) {
        glm::ivec3 chunkPos = world->worldToChunkPos(glm::vec3(blockPos));
        glm::ivec3 localPos = world->worldToLocalPos(glm::vec3(blockPos));
        
        // Is this block at a chunk boundary?
        bool isBlockAtBoundary = 
            localPos.x == 0 || localPos.x == World::CHUNK_SIZE - 1 ||
            localPos.y == 0 || localPos.y == World::CHUNK_HEIGHT - 1 ||
            localPos.z == 0 || localPos.z == World::CHUNK_SIZE - 1;
            
        context << " | Chunk:(" << chunkPos.x << "," << chunkPos.y << "," << chunkPos.z 
                << ") Local:(" << localPos.x << "," << localPos.y << "," << localPos.z 
                << ")" << (isBlockAtBoundary ? " [BOUNDARY]" : "");
                
        int currentBlock = world->getBlock(blockPos);
        context << " | Block type: " << currentBlock;
    }
    
    // Record the stack trace with the context
    Core::StackTrace::recordTrace(context.str());
}

void VoxelDebug::recordPlayerStuck(Player* player, const glm::vec3& position) {
    if (!s_initialized) {
        initialize();
    }
    
    std::stringstream context;
    context << "Player stuck at (" << position.x << "," << position.y << "," << position.z << ")";
    
    // Record the stack trace
    Core::StackTrace::recordTrace(context.str());
}

void VoxelDebug::recordStackTrace(const std::string& context) {
    if (!s_initialized) {
        initialize();
    }
    
    // Record the stack trace with the given context
    Core::StackTrace::recordTrace(context);
}

} // namespace Debug 