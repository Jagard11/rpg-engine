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
        std::cerr << "Error: World pointer is null in Player constructor" << std::endl;
        position = glm::vec3(0.0f, 10.0f, 0.0f);
        up = glm::vec3(0.0f, 1.0f, 0.0f);
        cameraDirection = glm::vec3(0.0f, 0.0f, -1.0f);
        movementDirection = cameraDirection;
    } else {
        // Position player close to the origin to avoid floating-point precision issues
        // But still at the surface of our sphere
        float surfaceR = w.getRadius() + 8.0f; // Surface radius
        
        // Position at the "north pole" with a slight offset to see the horizon better
        float angle = 10.0f * 3.14159f / 180.0f; // 10 degrees in radians
        position = glm::vec3(sin(angle) * surfaceR, cos(angle) * surfaceR, 0.0f);
        
        // Initialize up vector to point away from planet center
        up = glm::normalize(position);
        
        // Camera direction should look along the surface
        glm::vec3 northPole = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 tangent = glm::normalize(glm::cross(up, northPole));
        cameraDirection = glm::normalize(glm::cross(tangent, up));
        
        // Movement direction same as camera initially
        movementDirection = cameraDirection;
        
        // Ensure player starts at exactly the right height above surface
        // This prevents falling through on game start
        float exactHeight = surfaceR + 0.3f; // Position slightly above surface
        position = glm::normalize(position) * exactHeight;
        
        std::cout << "************ PLAYER INITIALIZATION ************" << std::endl;
        std::cout << "Sphere radius: " << w.getRadius() << " units" << std::endl;
        std::cout << "Surface height: " << surfaceR << " units (radius)" << std::endl;
        std::cout << "Player at: " << position.x << ", " << position.y << ", " << position.z << std::endl;
        std::cout << "Distance from center: " << glm::length(position) << std::endl;
        std::cout << "Height above surface: " << glm::length(position) - surfaceR << " units" << std::endl;
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
    // Calculate with double precision to avoid issues far from origin
    double px = static_cast<double>(position.x);
    double py = static_cast<double>(position.y);
    double pz = static_cast<double>(position.z);
    double posLength = sqrt(px*px + py*py + pz*pz);
    
    if (posLength > 0.1) {
        // Normalize with double precision then convert back to float
        double upX = px / posLength;
        double upY = py / posLength;
        double upZ = pz / posLength;
        
        glm::vec3 targetUp = glm::vec3(upX, upY, upZ);
        
        // Use a very small smooth factor (almost instant transition)
        // Smooth interpolation can cause issues with alignment at the surface
        float smoothFactor = 0.2f; // Increased from 0.05f for faster update
        up = glm::normalize(glm::mix(up, targetUp, smoothFactor));
    }
    
    // IMPORTANT: We pass positive deltaY to movement to make up look up
    movement.updateOrientation(deltaX, deltaY);

    // Process keyboard input for movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement.moveForward(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement.moveBackward(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement.moveLeft(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement.moveRight(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) movement.jump();
    
    // Process sprint key
    bool sprinting = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    movement.setSprinting(sprinting);
    
    // Apply gravity (only if not loading)
    if (!isLoading) {
        movement.applyGravity(deltaTime);
    }

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
            float surfaceR = world.getRadius() + 8.0f;
            
            // Reuse px, py, pz from above - don't redeclare
            double distFromCenter = sqrt(px*px + py*py + pz*pz);
            
            std::cout << "Distance from center: " << distFromCenter << ", height above surface: " 
                      << (distFromCenter - surfaceR) << std::endl;
            
            // Also log the up vector for debugging orientation issues
            std::cout << "Up vector: " << up.x << ", " << up.y << ", " << up.z 
                      << " (length: " << glm::length(up) << ")" << std::endl;
                      
            // Verify that up is pointing outward from center
            glm::vec3 normalizedPos = glm::normalize(position);
            float dotProduct = glm::dot(normalizedPos, up);
            std::cout << "Alignment (up·normalized_pos): " << dotProduct 
                      << " (should be close to 1.0)" << std::endl;
        }
    }
    
    // Extra safety check: ensure player doesn't fall below surface
    // Reuse the px, py, pz variables from above - don't redeclare
    double distFromCenter = sqrt(px*px + py*py + pz*pz);
    float surfaceR = world.getRadius() + 8.0f;
    
    // If player somehow gets below surface, reset to surface
    if (distFromCenter < surfaceR + 0.1f) {
        float safeHeight = surfaceR + 0.3f;
        position = glm::normalize(position) * safeHeight;
        
        if (DebugManager::getInstance().logPlayerInfo()) {
            std::cout << "SAFETY: Player below surface, resetting to safe height" << std::endl;
        }
    }
}

void Player::finishLoading() {
    // IMPORTANT FIX: Changed to false to enable player movement
    isLoading = false;
    std::cout << "Player loading complete, physics enabled" << std::endl;
    std::cout << "Player position: " << position.x << ", " << position.y << ", " << position.z << std::endl;
    
    // Add an extra safety position fix after loading completes
    float surfaceR = world.getRadius() + 8.0f;
    float safeHeight = surfaceR + 0.3f;
    position = glm::normalize(position) * safeHeight;
    std::cout << "Set player to safe height: " << glm::length(position) 
              << " (surface at: " << surfaceR << ")" << std::endl;
}