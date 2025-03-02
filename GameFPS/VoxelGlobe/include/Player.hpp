// ./VoxelGlobe/include/Player.hpp
#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <glm/glm.hpp>
#include "World.hpp"

class Player {
public:
    Player(const World& world);
    glm::vec3 position;
    glm::vec3 cameraDirection;
    glm::vec3 movementDirection;
    glm::vec3 up;
    float height = 1.75f;
    float speed = 5.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;

    int selectedSlot = 0;
    BlockType inventory[10] = {
        BlockType::GRASS, BlockType::DIRT, BlockType::AIR, BlockType::AIR,
        BlockType::AIR, BlockType::AIR, BlockType::AIR, BlockType::AIR,
        BlockType::AIR, BlockType::AIR
    };

    void moveForward(float deltaTime);
    void moveBackward(float deltaTime);
    void moveLeft(float deltaTime);
    void moveRight(float deltaTime);
    void applyGravity(const World& world, float deltaTime);
    void updateOrientation(float deltaX, float deltaY);
    void scrollInventory(float delta);
};

#endif