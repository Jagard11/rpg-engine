// ./src/Player/Player.cpp
#include "Player/Player.hpp"
#include <iostream>
#include "Debug/DebugManager.hpp"
#include <glm/gtc/matrix_transform.hpp>

static double scrollY = 0.0;
static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    scrollY = yoffset;
}

Player::Player(const World& w) 
    : world(w), 
      movement(w, position, cameraDirection, movementDirection, up),
      isLoading(false) {  // Start with isLoading = false to enable movement immediately
    if (!&w) {
        std::cerr << "Error: World reference is null in Player constructor" << std::endl;
        position = glm::vec3(0.0f, 10.0f, 0.0f);
        up = glm::vec3(0.0f, 1.0f, 0.0f);
        cameraDirection = glm::vec3(0.0f, 0.0f, -1.0f);
        movementDirection = cameraDirection;
    } else {
        // Position player at north pole, directly ON the surface, not above
        float surfaceR = w.getRadius() + 8.0f; // Surface radius is 1599.55
        position = glm::vec3(0.0f, surfaceR, 0.0f); // Place exactly at surface
        
        // At north pole, up vector points straight up
        up = glm::vec3(0.0f, 1.0f, 0.0f);
        
        // Camera direction is along Z axis (looking forward along globe surface)
        cameraDirection = glm::vec3(0.0f, 0.0f, 1.0f);
        
        // Movement direction same as camera initially
        movementDirection = cameraDirection;
        
        std::cout << "************ PLAYER INITIALIZATION ************" << std::endl;
        std::cout << "Sphere radius: " << w.getRadius() << " units" << std::endl;
        std::cout << "Surface height: " << surfaceR << " units (Y coordinate)" << std::endl;
        std::cout << "Player at: " << position.x << ", " << position.y << ", " << position.z << std::endl;
        std::cout << "Distance from center: " << glm::length(position) << std::endl;
        std::cout << "Height above surface: " << position.y - surfaceR << " units" << std::endl;
        std::cout << "********************************************" << std::endl;
    }
}


void Player::update(GLFWwindow* window, float deltaTime) {
    // Set the scroll callback
    glfwSetScrollCallback(window, scrollCallback);

    // Get mouse input
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
    
    // Update up vector to point radially outward from center
    if (glm::length(position) > 0) {
        up = glm::normalize(position);
    }
    
    // Update orientation based on mouse movement
    movement.updateOrientation(deltaX, -deltaY);

    // Process keyboard input for movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement.moveForward(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement.moveBackward(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement.moveLeft(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement.moveRight(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) movement.jump();
    
    // Process sprint key
    bool sprinting = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    movement.setSprinting(sprinting);
    
    // Apply gravity
    movement.applyGravity(deltaTime);

    // Handle inventory scrolling
    if (scrollY != 0.0) {
        inventory.scroll(static_cast<float>(scrollY));
        scrollY = 0.0;
    }
    
    // Debug logging
    if (DebugManager::getInstance().logPlayerInfo()) {
        static int frameCounter = 0;
        if (++frameCounter % 60 == 0) {
            std::cout << "Player position: " << position.x << ", " << position.y << ", " << position.z << std::endl;
        }
    }
}

void Player::finishLoading() {
    // IMPORTANT FIX: Changed to false to enable player movement
    isLoading = false;
    std::cout << "Player loading complete, physics enabled" << std::endl;
    std::cout << "Player position: " << position.x << ", " << position.y << ", " << position.z << std::endl;
}