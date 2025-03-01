// ./GameFPS/VoxelGlobe/include/VoxelManipulator.hpp
#ifndef VOXEL_MANIPULATOR_HPP
#define VOXEL_MANIPULATOR_HPP

#include "World.hpp"
#include "Player.hpp"
#include <glm/glm.hpp>

class VoxelManipulator {
public:
    VoxelManipulator(World& world);
    
    // Place a block at the targeted position (snaps to terrain)
    bool placeBlock(const Player& player, BlockType type);
    
    // Remove the targeted block
    bool removeBlock(const Player& player);

private:
    World& worldRef;

    // Raycast to detect voxel hits
    bool raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, 
                 glm::ivec3& hitPos, glm::vec3& hitNormal) const;
};

#endif