// ./src/Player/Player.cpp
#include "Player/Player.hpp"
#include <iostream>
#include "Debug/DebugManager.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "Utils/SphereUtils.hpp"

// Static scrollback function for inventory interaction
static double scrollY = 0.0;
static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    scrollY = yoffset;
}

Player::Player(const World& w) 
    : world(w), 
      movement(w, position, cameraDirection, movementDirection, up),
      isLoading(true) {
    
    if (!&w) {
        std::cerr << "Error: World pointer is null in Player constructor" << std::endl;
        position = glm::vec3(0.0f, 10.0f, 0.0f);
        up = glm::vec3(0.0f, 1.0f, 0.0f);
        cameraDirection = glm::vec3(0.0f, 0.0f, -1.0f);
        movementDirection = cameraDirection;
    } else {
        // SPAWN CORRECTLY: Position player at Earth's surface at true north pole 
        double surfaceR = SphereUtils::getSurfaceRadiusMeters();
        
        // Position at exactly the "north pole" on the surface plus a small offset
        position = glm::vec3(0.0f, static_cast<float>(surfaceR) + 2.0f, 0.0f);
        
        // Initialize up vector to point away from planet center
        up = glm::normalize(position);
        
        // Camera direction should look along the surface (in the +X direction)
        cameraDirection = glm::vec3(1.0f, 0.0f, 0.0f);
        
        // Make sure cameraDirection is perpendicular to up
        cameraDirection = glm::normalize(cameraDirection - glm::dot(cameraDirection, up) * up);
        
        // Movement direction same as camera initially
        movementDirection = cameraDirection;
        
        std::cout << "************ PLAYER INITIALIZATION ************" << std::endl;
        std::cout << "Earth radius: " << w.getRadius() << " meters" << std::endl;
        std::cout << "Surface height: " << surfaceR << " meters (radius)" << std::endl;
        std::cout << "Player at: " << position.x << ", " << position.y << ", " << position.z << std::endl;
        std::cout << "Distance from center: " << glm::length(position) << std::endl;
        std::cout << "Height above surface: " << glm::length(position) - surfaceR << " meters" << std::endl;
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
    float deltaY = static_cast<float>(lastY - mouseY); // Invert Y for natural camera control
    lastX = mouseX;
    lastY = mouseY;
    
    // Update up vector more responsively for Earth-scale
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
        
        // Use immediate transition for Earth-scale
        // No smooth transition needed - Earth is so big that small position changes
        // barely affect the up vector direction
        up = glm::vec3(upX, upY, upZ);
    }
    
    // Update camera orientation - pass positive deltaY to make up look up
    movement.updateOrientation(deltaX, deltaY);

    // Process keyboard input for movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement.moveForward(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement.moveBackward(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement.moveLeft(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement.moveRight(deltaTime);
    
    // JUMPING: Check direct key state rather than relying on keyboard callback
    // This handles key repeats and ensures jump is detected properly
    static int lastSpaceState = GLFW_RELEASE;
    int currentSpaceState = glfwGetKey(window, GLFW_KEY_SPACE);
    
    // Only jump on the initial press (not held)
    if (currentSpaceState == GLFW_PRESS && lastSpaceState == GLFW_RELEASE) {
        std::cout << "Space key pressed - triggering jump" << std::endl;
        movement.jump();
    }
    lastSpaceState = currentSpaceState;
    
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
            double surfaceR = SphereUtils::getSurfaceRadiusMeters();
            
            // Calculate distance from center with high precision
            double distFromCenter = sqrt(px*px + py*py + pz*pz);
            
            std::cout << "Distance from center: " << distFromCenter << ", height above surface: " 
                      << (distFromCenter - surfaceR) << " meters" << std::endl;
            
            // Also log the up vector for debugging orientation issues
            std::cout << "Up vector: " << up.x << ", " << up.y << ", " << up.z 
                      << " (length: " << glm::length(up) << ")" << std::endl;
                      
            // Verify that up is pointing outward from center
            glm::vec3 normalizedPos = glm::normalize(position);
            float dotProduct = glm::dot(normalizedPos, up);
            std::cout << "Alignment (up·normalized_pos): " << dotProduct 
                      << " (should be close to 1.0)" << std::endl;
                      
            // Log jump/grounded state
            std::cout << "Player is " << (movement.isPlayerGrounded() ? "grounded" : "in the air") 
                      << ", Vertical velocity: " << movement.getVerticalVelocity() << std::endl;
        }
    }
    
    // Extra safety check: ensure player doesn't fall below surface
    double distFromCenter = sqrt(px*px + py*py + pz*pz);
    double surfaceR = SphereUtils::getSurfaceRadiusMeters();
    
    // If player somehow gets below surface, reset to surface
    if (distFromCenter < surfaceR + 0.1) {
        float safeHeight = static_cast<float>(surfaceR + 2.0); // Set to 2.0 meters above surface
        position = glm::normalize(position) * safeHeight;
        
        if (DebugManager::getInstance().logPlayerInfo()) {
            std::cout << "SAFETY: Player below surface, resetting to safe height" << std::endl;
        }
    }
}

void Player::finishLoading() {
    isLoading = false;
    std::cout << "Player loading complete, physics enabled" << std::endl;
    std::cout << "Player position: " << position.x << ", " << position.y << ", " << position.z << std::endl;
    
    // Add an extra safety position fix after loading completes
    double surfaceR = SphereUtils::getSurfaceRadiusMeters();
    float safeHeight = static_cast<float>(surfaceR + 2.0); // 2.0 meters above surface
    position = glm::normalize(position) * safeHeight;
    std::cout << "Set player to safe height: " << glm::length(position) 
              << " (surface at: " << surfaceR << ")" << std::endl;
}