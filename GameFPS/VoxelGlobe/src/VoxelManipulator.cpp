// ./VoxelGlobe/src/VoxelManipulator.cpp
#include "VoxelManipulator.hpp"
#include "Debug.hpp"
#include <iostream>

VoxelManipulator::VoxelManipulator(World& world) : worldRef(world) {}

bool VoxelManipulator::placeBlock(const Player& player, BlockType type) {
    glm::vec3 eyePos = player.position + player.up * player.height;
    glm::ivec3 hitPos;
    glm::vec3 hitNormal;

    if (raycast(eyePos, player.cameraDirection, 5.0f, hitPos, hitNormal)) {
        glm::ivec3 placePos = hitPos + glm::ivec3(hitNormal);
        int worldY = placePos.y + static_cast<int>(1591.55f + 8.0f); // Adjust to world coordinates
        if (placePos.y >= 0 && placePos.y < Chunk::SIZE) {
            worldRef.setBlock(placePos.x, worldY, placePos.z, type);
            if (g_showDebug) {
                std::cout << "Placed block at (" << placePos.x << ", " << worldY << ", " << placePos.z 
                          << ") Type: " << static_cast<int>(type) << std::endl;
            }
            return true;
        } else if (g_showDebug) {
            std::cout << "Placement out of bounds at y = " << placePos.y << std::endl;
        }
    } else if (g_showDebug) {
        std::cout << "Failed to place block: No valid hit detected" << std::endl;
    }
    return false;
}

bool VoxelManipulator::removeBlock(const Player& player) {
    glm::vec3 eyePos = player.position + player.up * player.height;
    glm::ivec3 hitPos;
    glm::vec3 hitNormal;

    if (raycast(eyePos, player.cameraDirection, 5.0f, hitPos, hitNormal)) {
        int worldY = hitPos.y + static_cast<int>(1591.55f + 8.0f); // Adjust to world coordinates
        worldRef.setBlock(hitPos.x, worldY, hitPos.z, BlockType::AIR);
        if (g_showDebug) {
            std::cout << "Removed block at (" << hitPos.x << ", " << worldY << ", " << hitPos.z << ")" << std::endl;
        }
        return true;
    } else if (g_showDebug) {
        std::cout << "Failed to remove block: No hit detected" << std::endl;
    }
    return false;
}

bool VoxelManipulator::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, 
                               glm::ivec3& hitPos, glm::vec3& hitNormal) const {
    glm::vec3 dir = glm::normalize(direction);
    float t = 0.0f;
    float step = 0.1f;
    const float radius = 1591.55f;
    const float chunkBaseY = radius + 8.0f;

    while (t <= maxDistance) {
        glm::vec3 pos = origin + dir * t;
        int worldX = static_cast<int>(floor(pos.x));
        int worldY = static_cast<int>(floor(pos.y));
        int localY = worldY - static_cast<int>(chunkBaseY);
        int worldZ = static_cast<int>(floor(pos.z));

        if (localY >= 0 && localY < Chunk::SIZE) {
            Block block = worldRef.getBlock(worldX, localY, worldZ);
            if (block.type != BlockType::AIR) {
                hitPos = glm::ivec3(worldX, localY, worldZ);

                glm::vec3 hitPoint = pos;
                glm::vec3 blockCenter = glm::vec3(worldX + 0.5f, localY + chunkBaseY + 0.5f, worldZ + 0.5f);
                glm::vec3 diff = hitPoint - blockCenter;
                hitNormal = glm::vec3(0.0f);
                float maxDiff = 0.0f;
                if (fabs(diff.x) > fabs(maxDiff)) { maxDiff = diff.x; hitNormal = glm::vec3(diff.x > 0 ? 1 : -1, 0, 0); }
                if (fabs(diff.y) > fabs(maxDiff)) { maxDiff = diff.y; hitNormal = glm::vec3(0, diff.y > 0 ? 1 : -1, 0); }
                if (fabs(diff.z) > fabs(maxDiff)) { hitNormal = glm::vec3(0, 0, diff.z > 0 ? 1 : -1); }

                if (dir.y < -0.3f && diff.y > 0) {
                    hitNormal = glm::vec3(0, 1, 0);
                }

                if (g_showDebug) {
                    std::cout << "Raycast hit at (" << worldX << ", " << localY << ", " << worldZ 
                              << ") World Y: " << (localY + chunkBaseY) 
                              << " Normal: (" << hitNormal.x << ", " << hitNormal.y << ", " << hitNormal.z 
                              << ") Pos: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
                }
                return true;
            }
        }
        t += step;
    }
    if (g_showDebug) {
        glm::vec3 endPos = origin + dir * t;
        std::cout << "Raycast ended at t = " << t << " Pos: (" << endPos.x << ", " << endPos.y << ", " << endPos.z << ")" << std::endl;
    }
    return false;
}