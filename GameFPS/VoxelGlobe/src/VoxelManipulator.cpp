// ./src/VoxelManipulator.cpp
#include "VoxelManipulator.hpp"
#include "Core/Debug.hpp"
#include <iostream>

VoxelManipulator::VoxelManipulator(World& world) : worldRef(world) {}

bool VoxelManipulator::placeBlock(const Player& player, BlockType type) {
    if (type == BlockType::AIR) return false;

    glm::vec3 eyePos = player.position + player.up * player.getHeight();
    glm::ivec3 hitPos;
    glm::vec3 hitNormal;

    if (raycast(eyePos, player.cameraDirection, MAX_REACH, hitPos, hitNormal)) {
        glm::ivec3 placePos = hitPos + glm::ivec3(hitNormal);

        if (placePos.y < FLOOR_HEIGHT || placePos.y > CEILING_HEIGHT) {
            if (g_showDebug) {
                std::cout << "Placement out of height bounds: y = " << placePos.y << std::endl;
            }
            return false;
        }

        if (!isAdjacentToSolid(placePos)) {
            if (g_showDebug) {
                std::cout << "Placement failed: No adjacent solid block at (" 
                          << placePos.x << ", " << placePos.y << ", " << placePos.z << ")" << std::endl;
            }
            return false;
        }

        worldRef.setBlock(placePos.x, placePos.y, placePos.z, type);
        if (g_showDebug) {
            std::cout << "Placed block at (" << placePos.x << ", " << placePos.y << ", " << placePos.z 
                      << ") Type: " << static_cast<int>(type) << std::endl;
        }
        return true;
    }
    return false;
}

bool VoxelManipulator::removeBlock(const Player& player) {
    glm::vec3 eyePos = player.position + player.up * player.getHeight();
    glm::ivec3 hitPos;
    glm::vec3 hitNormal;

    if (raycast(eyePos, player.cameraDirection, MAX_REACH, hitPos, hitNormal)) {
        if (hitPos.y < FLOOR_HEIGHT || hitPos.y > CEILING_HEIGHT) {
            if (g_showDebug) {
                std::cout << "Removal out of height bounds: y = " << hitPos.y << std::endl;
            }
            return false;
        }

        Block block = worldRef.getBlock(hitPos.x, hitPos.y, hitPos.z);
        if (block.type == BlockType::AIR) {
            if (g_showDebug) {
                std::cout << "Cannot remove air at (" << hitPos.x << ", " << hitPos.y << ", " << hitPos.z << ")" << std::endl;
            }
            return false;
        }

        worldRef.setBlock(hitPos.x, hitPos.y, hitPos.z, BlockType::AIR);
        if (g_showDebug) {
            std::cout << "Removed block at (" << hitPos.x << ", " << hitPos.y << ", " << hitPos.z << ")" << std::endl;
        }
        return true;
    }
    return false;
}

bool VoxelManipulator::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, 
                               glm::ivec3& hitPos, glm::vec3& hitNormal, ToolType tool) const {
    glm::vec3 dir = glm::normalize(direction);
    glm::vec3 rayStart = origin;
    glm::ivec3 currentVoxel = glm::ivec3(floor(rayStart.x), floor(rayStart.y), floor(rayStart.z));
    glm::vec3 deltaDist = glm::vec3(
        dir.x == 0 ? FLT_MAX : fabs(1.0f / dir.x),
        dir.y == 0 ? FLT_MAX : fabs(1.0f / dir.y),
        dir.z == 0 ? FLT_MAX : fabs(1.0f / dir.z)
    );
    glm::ivec3 step = glm::ivec3(
        dir.x > 0 ? 1 : -1,
        dir.y > 0 ? 1 : -1,
        dir.z > 0 ? 1 : -1
    );
    glm::vec3 sideDist = glm::vec3(
        (dir.x > 0 ? currentVoxel.x + 1 - rayStart.x : rayStart.x - currentVoxel.x) * deltaDist.x,
        (dir.y > 0 ? currentVoxel.y + 1 - rayStart.y : rayStart.y - currentVoxel.y) * deltaDist.y,
        (dir.z > 0 ? currentVoxel.z + 1 - rayStart.z : rayStart.z - currentVoxel.z) * deltaDist.z
    );

    float t = 0.0f;
    while (t <= maxDistance) {
        if (currentVoxel.y < FLOOR_HEIGHT || currentVoxel.y > CEILING_HEIGHT) {
            if (g_showDebug) {
                std::cout << "Raycast out of bounds at (" << currentVoxel.x << ", " << currentVoxel.y << ", " << currentVoxel.z << ")" << std::endl;
            }
            return false;
        }

        Block block = worldRef.getBlock(currentVoxel.x, currentVoxel.y, currentVoxel.z);

        if (g_showDebug) {
            std::cout << "Checking block at (" << currentVoxel.x << ", " << currentVoxel.y << ", " << currentVoxel.z 
                      << ") Type: " << (int)block.type << std::endl;
        }

        // For now, with no tool (NONE), only solid blocks (DIRT, GRASS) are hittable
        if (tool == ToolType::NONE && (block.type == BlockType::DIRT || block.type == BlockType::GRASS)) {
            hitPos = currentVoxel;
            glm::vec3 hitPoint = origin + dir * t;
            glm::vec3 blockCenter = glm::vec3(currentVoxel.x + 0.5f, currentVoxel.y + 0.5f, currentVoxel.z + 0.5f);
            glm::vec3 diff = hitPoint - blockCenter;
            hitNormal = glm::vec3(0.0f);
            float maxDiff = 0.0f;
            if (fabs(diff.x) > fabs(maxDiff)) { maxDiff = diff.x; hitNormal = glm::vec3(diff.x > 0 ? 1 : -1, 0, 0); }
            if (fabs(diff.y) > fabs(maxDiff)) { maxDiff = diff.y; hitNormal = glm::vec3(0, diff.y > 0 ? 1 : -1, 0); }
            if (fabs(diff.z) > fabs(maxDiff)) { hitNormal = glm::vec3(0, 0, diff.z > 0 ? 1 : -1); }

            if (g_showDebug) {
                std::cout << "Raycast hit at (" << hitPos.x << ", " << hitPos.y << ", " << hitPos.z 
                          << ") Normal: (" << hitNormal.x << ", " << hitNormal.y << ", " << hitNormal.z 
                          << ") Pos: (" << hitPoint.x << ", " << hitPoint.y << ", " << hitPoint.z << ")" << std::endl;
            }
            return true;
        }

        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
            t = sideDist.x;
            sideDist.x += deltaDist.x;
            currentVoxel.x += step.x;
        } else if (sideDist.y < sideDist.z) {
            t = sideDist.y;
            sideDist.y += deltaDist.y;
            currentVoxel.y += step.y;
        } else {
            t = sideDist.z;
            sideDist.z += deltaDist.z;
            currentVoxel.z += step.z;
        }
    }

    if (g_showDebug) {
        glm::vec3 endPos = origin + dir * t;
        std::cout << "Raycast ended at t = " << t << " Pos: (" << endPos.x << ", " << endPos.y << ", " << endPos.z << ")" << std::endl;
    }
    return false;
}

bool VoxelManipulator::isAdjacentToSolid(const glm::ivec3& pos) const {
    const glm::ivec3 directions[6] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
    };

    for (const auto& dir : directions) {
        glm::ivec3 neighbor = pos + dir;
        if (neighbor.y >= FLOOR_HEIGHT && neighbor.y <= CEILING_HEIGHT) {
            Block block = worldRef.getBlock(neighbor.x, neighbor.y, neighbor.z);
            if (block.type == BlockType::GRASS || block.type == BlockType::DIRT) {
                return true;
            }
        }
    }
    return false;
}