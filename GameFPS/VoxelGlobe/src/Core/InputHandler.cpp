// ./src/Core/InputHandler.cpp
#include <GL/glew.h>
#include "Core/InputHandler.hpp"
#include "Player/Player.hpp"
#include "VoxelManipulator.hpp"
#include "World/World.hpp"
#include <iostream>

// Static instance for callback handling
static InputHandler* s_instance = nullptr;

// Callback handlers for GLFW
void InputHandler::keyCallbackGLFW(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (s_instance) {
        s_instance->handleKeyPress(key, scancode, action, mods);
    }
}

void InputHandler::mouseButtonCallbackGLFW(GLFWwindow* window, int button, int action, int mods) {
    if (s_instance) {
        s_instance->handleMouseButton(button, action, mods);
    }
}

void InputHandler::scrollCallbackGLFW(GLFWwindow* window, double xoffset, double yoffset) {
    if (s_instance) {
        s_instance->handleScroll(xoffset, yoffset);
    }
}

void InputHandler::cursorPosCallbackGLFW(GLFWwindow* window, double xpos, double ypos) {
    if (s_instance) {
        s_instance->mousePosition = glm::vec2(xpos, ypos);
        if (s_instance->cursorPosCallback) {
            s_instance->cursorPosCallback(xpos, ypos);
        }
    }
}

// Constructor
InputHandler::InputHandler() 
    : mousePosition(0.0f, 0.0f), 
      lastMousePosition(0.0f, 0.0f),
      scrollDelta(0.0) {
    // Set static instance for callbacks
    s_instance = this;
}

// Initialize
void InputHandler::initialize(GLFWwindow* window) {
    setupCallbacks();
    
    // Get initial mouse position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    mousePosition = glm::vec2(xpos, ypos);
    lastMousePosition = mousePosition;
}

// Set up GLFW callbacks
void InputHandler::setupCallbacks() {
    glfwSetKeyCallback(glfwGetCurrentContext(), keyCallbackGLFW);
    glfwSetMouseButtonCallback(glfwGetCurrentContext(), mouseButtonCallbackGLFW);
    glfwSetScrollCallback(glfwGetCurrentContext(), scrollCallbackGLFW);
    glfwSetCursorPosCallback(glfwGetCurrentContext(), cursorPosCallbackGLFW);
}

// Process input for a frame
void InputHandler::processInput(GLFWwindow* window, float deltaTime, Player& player, VoxelManipulator& voxelManip) {
    // Handle basic movement - now using Player's public methods that delegate to Movement
    if (isKeyDown(GLFW_KEY_W)) {
        player.moveForward(deltaTime);
    }
    if (isKeyDown(GLFW_KEY_S)) {
        player.moveBackward(deltaTime);
    }
    if (isKeyDown(GLFW_KEY_A)) {
        player.moveLeft(deltaTime);
    }
    if (isKeyDown(GLFW_KEY_D)) {
        player.moveRight(deltaTime);
    }
    
    // Jump
    if (isKeyDown(GLFW_KEY_SPACE)) {
        player.jump();
    }
    
    // Sprint
    bool sprinting = isKeyDown(GLFW_KEY_LEFT_SHIFT);
    player.setSprinting(sprinting);
    
    // Handle block placement (left click)
    if (isKeyDown(GLFW_MOUSE_BUTTON_LEFT) && wasKeyJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        BlockType selectedBlock = player.inventory.slots[player.inventory.selectedSlot];
        if (selectedBlock != BlockType::AIR) {
            if (voxelManip.placeBlock(player, selectedBlock)) {
                std::cout << "Block placed successfully: " << static_cast<int>(selectedBlock) << std::endl;
            } else {
                std::cout << "Failed to place block" << std::endl;
            }
        }
    }
    
    // Handle block removal (right click)
    if (isKeyDown(GLFW_MOUSE_BUTTON_RIGHT) && wasKeyJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        if (voxelManip.removeBlock(player)) {
            std::cout << "Block removed successfully" << std::endl;
        } else {
            std::cout << "Failed to remove block" << std::endl;
        }
    }
    
    // Handle inventory selection with scroll
    if (scrollDelta != 0.0) {
        player.inventory.scroll(static_cast<float>(scrollDelta));
        scrollDelta = 0.0; // Reset scroll delta
    }
}

// Update mouse state
void InputHandler::updateMouseState(GLFWwindow* window, glm::vec2& mouseDelta, bool& firstMouse) {
    // Calculate mouse delta
    if (firstMouse) {
        lastMousePosition = mousePosition;
        firstMouse = false;
    }
    
    mouseDelta.x = mousePosition.x - lastMousePosition.x;
    mouseDelta.y = lastMousePosition.y - mousePosition.y; // Reversed for camera control
    
    lastMousePosition = mousePosition;
}

// Handle key press
void InputHandler::handleKeyPress(int key, int scancode, int action, int mods) {
    // Update key state
    if (action == GLFW_PRESS) {
        keyStates[key] = true;
        keyJustPressed[key] = true;
    } else if (action == GLFW_RELEASE) {
        keyStates[key] = false;
    }
    
    // Call custom callback if set
    if (keyCallback) {
        keyCallback(key, scancode, action, mods);
    }
}

// Handle mouse button press
void InputHandler::handleMouseButton(int button, int action, int mods) {
    // Update button state
    if (action == GLFW_PRESS) {
        mouseButtonStates[button] = true;
        keyJustPressed[button] = true; // Use same map for simplicity
    } else if (action == GLFW_RELEASE) {
        mouseButtonStates[button] = false;
    }
    
    // Call custom callback if set
    if (mouseButtonCallback) {
        mouseButtonCallback(button, action, mods);
    }
}

// Handle mouse scroll
void InputHandler::handleScroll(double xoffset, double yoffset) {
    scrollDelta = yoffset;
}

// Check if a key was just pressed this frame
bool InputHandler::wasKeyJustPressed(int key) {
    auto it = keyJustPressed.find(key);
    return it != keyJustPressed.end() && it->second;
}

// Check if a key is held down
bool InputHandler::isKeyDown(int key) {
    auto it = keyStates.find(key);
    if (it != keyStates.end()) {
        return it->second;
    } else if (key >= GLFW_MOUSE_BUTTON_1 && key <= GLFW_MOUSE_BUTTON_LAST) {
        // Handle mouse buttons
        int button = key;
        auto buttonIt = mouseButtonStates.find(button);
        return buttonIt != mouseButtonStates.end() && buttonIt->second;
    }
    return false;
}

// Update cursor mode based on UI state
void InputHandler::updateCursorMode(GLFWwindow* window, bool showEscapeMenu, bool showDebugWindow) {
    if (showEscapeMenu || showDebugWindow) {
        // Show cursor when menus are open
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
        // Disable cursor during gameplay
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}

// Reset per-frame state
void InputHandler::resetFrameState() {
    // Clear all "just pressed" flags
    keyJustPressed.clear();
    
    // Reset scroll delta (consumed by inventory)
    scrollDelta = 0.0;
}