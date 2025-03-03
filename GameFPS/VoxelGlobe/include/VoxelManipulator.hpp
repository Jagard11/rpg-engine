// ./include/VoxelManipulator.hpp
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
    static constexpr int FLOOR_HEIGHT = 1500;    // Configurable floor height (world Y)
    static constexpr int CEILING_HEIGHT = 1755;  // Configurable ceiling height (world Y, 255 above floor)
    static constexpr float MAX_REACH = 5.0f;     // Maximum reach distance (5 meters)
    bool isAdjacentToSolid(const glm::ivec3& pos) const;
};

#endif