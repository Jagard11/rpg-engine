#include "world/VoxelManipulator.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include "renderer/Renderer.hpp"
#include "debug/VoxelDebug.hpp"

VoxelManipulator::VoxelManipulator()
    : m_leftMousePressed(false)
    , m_rightMousePressed(false)
{
}

void VoxelManipulator::initialize(World* world)
{
    // Reset state
    m_leftMousePressed = false;
    m_rightMousePressed = false;
}

void VoxelManipulator::processInput(World* world, Player* player, int mouseButton, bool isPressed, Renderer* renderer)
{
    // Skip if renderer is not available
    if (!renderer) {
        return;
    }
    
    // Get the mouse button state
    if (mouseButton == GLFW_MOUSE_BUTTON_LEFT) {
        // Filter out duplicate events
        if (m_leftMousePressed == isPressed) {
            return;
        }
        m_leftMousePressed = isPressed;
        
        // Only act on press, not release
        if (!isPressed) {
            return;
        }
        
        // Get the highlighted block and face directly from the renderer
        glm::ivec3 blockPos = renderer->getHighlightedBlock();
        glm::ivec3 faceNormal = renderer->getHighlightedFace();
        
        // Only proceed if highlighting is active (renderer has valid highlight)
        if (renderer->isHighlightEnabled() && (faceNormal.x != 0 || faceNormal.y != 0 || faceNormal.z != 0)) {
            // Create a raycast result using the renderer's highlighted information
            World::RaycastResult raycastResult;
            raycastResult.hit = true;
            raycastResult.blockPos = blockPos;
            raycastResult.faceNormal = faceNormal;
            raycastResult.distance = glm::length(glm::vec3(blockPos) - player->getPosition());
            
            // Add a voxel on the highlighted face
            addVoxel(world, player, raycastResult);
        }
    }
    else if (mouseButton == GLFW_MOUSE_BUTTON_RIGHT) {
        // Filter out duplicate events
        if (m_rightMousePressed == isPressed) {
            return;
        }
        m_rightMousePressed = isPressed;
        
        // Only act on press, not release
        if (!isPressed) {
            return;
        }
        
        // Get the highlighted block and face directly from the renderer
        glm::ivec3 blockPos = renderer->getHighlightedBlock();
        glm::ivec3 faceNormal = renderer->getHighlightedFace();
        
        // Only proceed if highlighting is active (renderer has valid highlight)
        if (renderer->isHighlightEnabled() && (faceNormal.x != 0 || faceNormal.y != 0 || faceNormal.z != 0)) {
            // Create a raycast result using the renderer's highlighted information
            World::RaycastResult raycastResult;
            raycastResult.hit = true;
            raycastResult.blockPos = blockPos;
            raycastResult.faceNormal = faceNormal;
            raycastResult.distance = glm::length(glm::vec3(blockPos) - player->getPosition());
            
            // Remove the highlighted voxel
            removeVoxel(world, raycastResult);
        }
    }
}

bool VoxelManipulator::addVoxel(World* world, Player* player, const World::RaycastResult& raycastResult, int blockType)
{
    if (!raycastResult.hit) {
        return false;
    }
    
    // Get block position in front of face (add face normal to block position)
    glm::ivec3 newBlockPos = raycastResult.blockPos + glm::ivec3(raycastResult.faceNormal);
    
    // Check if placement is valid (not inside player, etc.)
    if (!isValidPlacement(world, newBlockPos)) {
        Debug::VoxelDebug::recordVoxelOperation(world, newBlockPos, false, "ADD");
        return false;
    }
    
    // Check if the new block would collide with the player
    if (wouldCollideWithPlayer(newBlockPos, player)) {
        std::cout << "Cannot place block at (" 
                  << newBlockPos.x << ", " << newBlockPos.y << ", " << newBlockPos.z 
                  << ") - would collide with player" << std::endl;
        Debug::VoxelDebug::recordVoxelOperation(world, newBlockPos, false, "ADD");
        return false;
    }
    
    // Calculate chunk position and local position for debugging
    glm::ivec3 chunkPos = world->worldToChunkPos(glm::vec3(newBlockPos));
    glm::ivec3 localPos = world->worldToLocalPos(glm::vec3(newBlockPos));
    
    // CRITICAL FIX: Check if we're exactly at a chunk boundary (where modulo is 0)
    // These are special cases that need careful handling
    bool exactBoundary = false;
    if (newBlockPos.x % World::CHUNK_SIZE == 0 || 
        newBlockPos.z % World::CHUNK_SIZE == 0 ||
        newBlockPos.y % World::CHUNK_HEIGHT == 0) {
        exactBoundary = true;
        std::cout << "Block is at EXACT CHUNK BOUNDARY!" << std::endl;
    }
    
    // Check if we're at a chunk boundary
    bool isChunkBoundary = 
        localPos.x == 0 || localPos.x == (World::CHUNK_SIZE - 1) ||
        localPos.y == 0 || localPos.y == (World::CHUNK_HEIGHT - 1) ||
        localPos.z == 0 || localPos.z == (World::CHUNK_SIZE - 1) ||
        exactBoundary;
    
    // Set the block
    std::cout << "Adding " << blockType << " block at position: (" 
              << newBlockPos.x << ", " << newBlockPos.y << ", " << newBlockPos.z 
              << "), chunk: (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z
              << "), local: (" << localPos.x << ", " << localPos.y << ", " << localPos.z 
              << "), is boundary: " << (isChunkBoundary ? "YES" : "NO") 
              << (exactBoundary ? " [EXACT BOUNDARY]" : "") << std::endl;
    
    world->setBlock(newBlockPos, blockType);
    
    // CRITICAL FIX: For exact boundaries, ensure both chunks are updated
    if (exactBoundary) {
        // Determine which adjacent chunks need updates based on exact boundaries
        if (newBlockPos.x % World::CHUNK_SIZE == 0) {
            // Update both chunks on either side of X boundary
            world->updateChunkMeshes(chunkPos + glm::ivec3(-1, 0, 0));
            world->updateChunkMeshes(chunkPos + glm::ivec3(0, 0, 0));
        }
        if (newBlockPos.z % World::CHUNK_SIZE == 0) {
            // Update both chunks on either side of Z boundary
            world->updateChunkMeshes(chunkPos + glm::ivec3(0, 0, -1));
            world->updateChunkMeshes(chunkPos + glm::ivec3(0, 0, 0));
        }
        if (newBlockPos.y % World::CHUNK_HEIGHT == 0) {
            // Update both chunks on either side of Y boundary
            world->updateChunkMeshes(chunkPos + glm::ivec3(0, -1, 0));
            world->updateChunkMeshes(chunkPos + glm::ivec3(0, 0, 0));
        }
    } else {
        // Normal case - update surrounding chunks
        updateSurroundingChunks(world, newBlockPos);
    }
    
    // Record successful voxel addition
    Debug::VoxelDebug::recordVoxelOperation(world, newBlockPos, true, "ADD");
    
    return true;
}

bool VoxelManipulator::removeVoxel(World* world, const World::RaycastResult& raycastResult)
{
    if (!raycastResult.hit) {
        return false;
    }
    
    // Get the block position from raycast
    glm::ivec3 blockPos = raycastResult.blockPos;
    
    // Check if the block exists
    int blockType = world->getBlock(blockPos);
    if (blockType == 0) {
        // Block is already air
        Debug::VoxelDebug::recordVoxelOperation(world, blockPos, false, "REMOVE");
        return false;
    }
    
    // Calculate chunk position and local position for debugging
    glm::ivec3 chunkPos = world->worldToChunkPos(glm::vec3(blockPos));
    glm::ivec3 localPos = world->worldToLocalPos(glm::vec3(blockPos));
    
    // CRITICAL FIX: Check if we're exactly at a chunk boundary (where modulo is 0)
    // These are special cases that need careful handling
    bool exactBoundary = false;
    if (blockPos.x % World::CHUNK_SIZE == 0 || 
        blockPos.z % World::CHUNK_SIZE == 0 ||
        blockPos.y % World::CHUNK_HEIGHT == 0) {
        exactBoundary = true;
        std::cout << "Block is at EXACT CHUNK BOUNDARY!" << std::endl;
    }
    
    // Check if we're at a chunk boundary
    bool isChunkBoundary = 
        localPos.x == 0 || localPos.x == (World::CHUNK_SIZE - 1) ||
        localPos.y == 0 || localPos.y == (World::CHUNK_HEIGHT - 1) ||
        localPos.z == 0 || localPos.z == (World::CHUNK_SIZE - 1) ||
        exactBoundary;
        
    std::cout << "Removing block at position: (" 
              << blockPos.x << ", " << blockPos.y << ", " << blockPos.z 
              << "), chunk: (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z
              << "), local: (" << localPos.x << ", " << localPos.y << ", " << localPos.z 
              << "), is boundary: " << (isChunkBoundary ? "YES" : "NO")
              << (exactBoundary ? " [EXACT BOUNDARY]" : "") << std::endl;
    
    // Set block to air (0)
    world->setBlock(blockPos, 0);
    
    // CRITICAL FIX: For exact boundaries, ensure both chunks are updated
    if (exactBoundary) {
        // Determine which adjacent chunks need updates based on exact boundaries
        if (blockPos.x % World::CHUNK_SIZE == 0) {
            // Update both chunks on either side of X boundary
            world->updateChunkMeshes(chunkPos + glm::ivec3(-1, 0, 0));
            world->updateChunkMeshes(chunkPos + glm::ivec3(0, 0, 0));
        }
        if (blockPos.z % World::CHUNK_SIZE == 0) {
            // Update both chunks on either side of Z boundary
            world->updateChunkMeshes(chunkPos + glm::ivec3(0, 0, -1));
            world->updateChunkMeshes(chunkPos + glm::ivec3(0, 0, 0));
        }
        if (blockPos.y % World::CHUNK_HEIGHT == 0) {
            // Update both chunks on either side of Y boundary
            world->updateChunkMeshes(chunkPos + glm::ivec3(0, -1, 0));
            world->updateChunkMeshes(chunkPos + glm::ivec3(0, 0, 0));
        }
    } else {
        // Normal case - update surrounding chunks
        updateSurroundingChunks(world, blockPos);
    }
    
    // Record successful voxel removal
    Debug::VoxelDebug::recordVoxelOperation(world, blockPos, true, "REMOVE");
    
    return true;
}

bool VoxelManipulator::isValidPlacement(World* world, const glm::ivec3& pos) const
{
    // Don't place blocks below y=0
    if (pos.y < 0) {
        return false;
    }
    
    // Don't replace existing blocks
    if (world->getBlock(pos) != 0) {
        return false;
    }
    
    return true;
}

bool VoxelManipulator::wouldCollideWithPlayer(const glm::ivec3& blockPos, Player* player) const
{
    // Get player bounds
    glm::vec3 playerMin = player->getMinBounds();
    glm::vec3 playerMax = player->getMaxBounds();
    
    // Block bounds
    glm::vec3 blockMin = glm::vec3(blockPos);
    glm::vec3 blockMax = glm::vec3(blockPos) + glm::vec3(1.0f);
    
    // Check if the block's bounds overlap with the player's bounds
    // This uses AABB collision detection (Axis-Aligned Bounding Box)
    if (blockMin.x <= playerMax.x && blockMax.x >= playerMin.x &&
        blockMin.y <= playerMax.y && blockMax.y >= playerMin.y &&
        blockMin.z <= playerMax.z && blockMax.z >= playerMin.z) {
        return true; // Collision detected
    }
    
    return false; // No collision
}

void VoxelManipulator::updateSurroundingChunks(World* world, const glm::ivec3& blockPos)
{
    // Convert to chunk coordinates
    glm::ivec3 chunkPos = world->worldToChunkPos(glm::vec3(blockPos));
    glm::ivec3 localPos = world->worldToLocalPos(glm::vec3(blockPos));
    
    // Update the chunk containing the block
    world->updateChunkMeshes(chunkPos);
    
    // Flags to track which adjacent faces need updates
    bool updateNegX = false, updatePosX = false;
    bool updateNegY = false, updatePosY = false;
    bool updateNegZ = false, updatePosZ = false;
    
    // Also update neighboring chunks if the block is at a chunk boundary
    const int CHUNK_SIZE = World::CHUNK_SIZE;
    const int CHUNK_HEIGHT = World::CHUNK_HEIGHT;
    
    // Check X boundaries
    if (localPos.x == 0) {
        // On the negative X boundary
        updateNegX = true;
        world->updateChunkMeshes(chunkPos + glm::ivec3(-1, 0, 0));
    } else if (localPos.x == CHUNK_SIZE - 1) {
        // On the positive X boundary
        updatePosX = true;
        world->updateChunkMeshes(chunkPos + glm::ivec3(1, 0, 0));
    }
    
    // Check Y boundaries
    if (localPos.y == 0) {
        // On the negative Y boundary
        updateNegY = true;
        world->updateChunkMeshes(chunkPos + glm::ivec3(0, -1, 0));
    } else if (localPos.y == CHUNK_HEIGHT - 1) {
        // On the positive Y boundary
        updatePosY = true;
        world->updateChunkMeshes(chunkPos + glm::ivec3(0, 1, 0));
    }
    
    // Check Z boundaries
    if (localPos.z == 0) {
        // On the negative Z boundary
        updateNegZ = true;
        world->updateChunkMeshes(chunkPos + glm::ivec3(0, 0, -1));
    } else if (localPos.z == CHUNK_SIZE - 1) {
        // On the positive Z boundary
        updatePosZ = true;
        world->updateChunkMeshes(chunkPos + glm::ivec3(0, 0, 1));
    }
    
    // CRITICAL FIX: Update diagonal chunks for corner blocks
    // If a block is at a corner, we need to update the diagonally adjacent chunks
    if (updateNegX && updateNegZ) {
        world->updateChunkMeshes(chunkPos + glm::ivec3(-1, 0, -1)); // NegX, NegZ corner
    }
    if (updateNegX && updatePosZ) {
        world->updateChunkMeshes(chunkPos + glm::ivec3(-1, 0, 1));  // NegX, PosZ corner
    }
    if (updatePosX && updateNegZ) {
        world->updateChunkMeshes(chunkPos + glm::ivec3(1, 0, -1));  // PosX, NegZ corner
    }
    if (updatePosX && updatePosZ) {
        world->updateChunkMeshes(chunkPos + glm::ivec3(1, 0, 1));   // PosX, PosZ corner
    }
    
    // Handle vertical corners as well if needed
    if (updateNegX && updateNegY) {
        world->updateChunkMeshes(chunkPos + glm::ivec3(-1, -1, 0)); // NegX, NegY
    }
    if (updateNegX && updatePosY) {
        world->updateChunkMeshes(chunkPos + glm::ivec3(-1, 1, 0));  // NegX, PosY
    }
    if (updatePosX && updateNegY) {
        world->updateChunkMeshes(chunkPos + glm::ivec3(1, -1, 0));  // PosX, NegY
    }
    if (updatePosX && updatePosY) {
        world->updateChunkMeshes(chunkPos + glm::ivec3(1, 1, 0));   // PosX, PosY
    }
    if (updateNegZ && updateNegY) {
        world->updateChunkMeshes(chunkPos + glm::ivec3(0, -1, -1)); // NegZ, NegY
    }
    if (updateNegZ && updatePosY) {
        world->updateChunkMeshes(chunkPos + glm::ivec3(0, 1, -1));  // NegZ, PosY
    }
    if (updatePosZ && updateNegY) {
        world->updateChunkMeshes(chunkPos + glm::ivec3(0, -1, 1));  // PosZ, NegY
    }
    if (updatePosZ && updatePosY) {
        world->updateChunkMeshes(chunkPos + glm::ivec3(0, 1, 1));   // PosZ, PosY
    }
} 