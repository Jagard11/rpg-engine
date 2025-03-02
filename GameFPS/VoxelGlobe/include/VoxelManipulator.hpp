// ./VoxelGlobe/include/VoxelManipulator.hpp
#ifndef VOXEL_MANIPULATOR_HPP
#define VOXEL_MANIPULATOR_HPP

#include "World.hpp"
#include "Player.hpp"
#include <glm/glm.hpp>

class VoxelManipulator {
public:
    VoxelManipulator(World& world);
    
    bool placeBlock(const Player& player, BlockType type);
    bool removeBlock(const Player& player);
    bool raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, 
                 glm::ivec3& hitPos, glm::vec3& hitNormal) const; // Moved from private to public

private:
    World& worldRef;
};

#endif