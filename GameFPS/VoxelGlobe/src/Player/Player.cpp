// ./GameFPS/VoxelGlobe/src/Player/Player.cpp
#include "Player/Player.hpp"
#include <iostream>
#include "Core/Debug.hpp"

// Scroll callback function (global to work with GLFW)
static double scrollY = 0.0;
static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    scrollY = yoffset;
}

Player::Player(const World& w) 
    : world(w), 
      movement(w, position, cameraDirection, movementDirection, up) { // Initialize movement with required args
    position = glm::vec3(0.0f, 1640.0f, 0.0f);
    up = glm::normalize(position);
    float yaw = 0.0f, pitch = 45.0f;
    float radYaw = glm::radians(yaw);
    float radPitch = glm::radians(pitch);
    cameraDirection = glm::vec3(cos(radPitch) * cos(radYaw), sin(radPitch), cos(radPitch) * sin(radYaw));
    cameraDirection = glm::normalize(cameraDirection);
    movementDirection = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);

    if (g_showDebug) {
        std::cout << "Initial Player Pos: " << position.x << ", " << position.y << ", " << position.z << std::endl;
        std::cout << "Initial Camera Dir: " << cameraDirection.x << ", " << cameraDirection.y << ", " << cameraDirection.z << std::endl;
    }
}

void Player::update(GLFWwindow* window, float deltaTime) {
    // Set scroll callback (only needs to be set once, typically in main, but here for simplicity)
    glfwSetScrollCallback(window, scrollCallback);

    // Mouse movement
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    static double lastX = 400, lastY = 300;
    static bool firstMouse = true;
    if (firstMouse) {
        lastX = mouseX;
        lastY = mouseY;
        firstMouse = false;
    }
    float deltaX = static_cast<float>(mouseX - lastX);
    float deltaY = static_cast<float>(mouseY - lastY);
    lastX = mouseX;
    lastY = mouseY;
    movement.updateOrientation(deltaX, deltaY);

    // Keyboard movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement.moveForward(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement.moveBackward(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement.moveLeft(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement.moveRight(deltaTime);

    // Gravity
    movement.applyGravity(deltaTime);

    // Inventory scrolling
    if (scrollY != 0.0) {
        inventory.scroll(static_cast<float>(scrollY));
        scrollY = 0.0; // Reset after use
    }
}