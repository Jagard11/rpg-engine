#ifndef VOXEL_MANIPULATOR_HPP
#define VOXEL_MANIPULATOR_HPP

#include "World/World.hpp"
#include "Player/Player.hpp"
#include <glm/glm.hpp>

enum class ToolType {
    NONE,
    BUCKET,
    VACUUM
};

class VoxelManipulator {
public:
    VoxelManipulator(World& world);
    
    bool placeBlock(const Player& player, BlockType type);
    bool removeBlock(const Player& player);
    bool raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, 
                 glm::ivec3& hitPos, glm::vec3& hitNormal, ToolType tool = ToolType::NONE) const;

private:
    World& worldRef;
    // Increased reach to match the larger world scale
    static constexpr float MAX_REACH = 100.0f;  // Increased from 5.0f to 100.0f
    
    // New methods for block placement validation and collision updating
    bool isValidPlacementPosition(const glm::ivec3& pos) const;
    bool isValidRemovalPosition(const glm::ivec3& pos) const;
    bool isAdjacentToSolid(const glm::ivec3& pos) const;
    void updateChunkCollisions(const glm::ivec3& blockPos);
};

#endif