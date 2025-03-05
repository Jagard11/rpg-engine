// ./include/Core/InputHandler.hpp
#ifndef INPUT_HANDLER_HPP
#define INPUT_HANDLER_HPP

// Include GLEW first before any other GL headers
#include <GL/glew.h>
// Then include GLFW
#include <GLFW/glfw3.h>

#include <functional>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>

// Forward declarations
class Player;
class VoxelManipulator;

/**
 * Centralized input handling system for the game
 * This class processes all input events and dispatches them to appropriate handlers
 */
class InputHandler {
public:
    // Constructor
    InputHandler();
    
    // Initialize with window
    void initialize(GLFWwindow* window);
    
    // Set up callbacks
    void setupCallbacks();
    
    // Process input for a frame
    void processInput(GLFWwindow* window, float deltaTime, Player& player, VoxelManipulator& voxelManip);
    
    // Update mouse state
    void updateMouseState(GLFWwindow* window, glm::vec2& mouseDelta, bool& firstMouse);
    
    // Handle key press 
    void handleKeyPress(int key, int scancode, int action, int mods);
    
    // Handle mouse button press
    void handleMouseButton(int button, int action, int mods);
    
    // Handle mouse scroll
    void handleScroll(double xoffset, double yoffset);
    
    // Check if a key was just pressed this frame
    bool wasKeyJustPressed(int key);
    
    // Check if a key is held down
    bool isKeyDown(int key);
    
    // Update cursor state based on UI state
    void updateCursorMode(GLFWwindow* window, bool showEscapeMenu, bool showDebugWindow);
    
    // Getters for mouse state
    double getScrollDelta() const { return scrollDelta; }
    glm::vec2 getMousePosition() const { return mousePosition; }
    
    // Reset per-frame state
    void resetFrameState();
    
    // Set key handling callback
    void setKeyCallback(std::function<void(int, int, int, int)> callback) {
        keyCallback = callback;
    }
    
    // Set mouse button callback
    void setMouseButtonCallback(std::function<void(int, int, int)> callback) {
        mouseButtonCallback = callback;
    }
    
    // Set cursor position callback
    void setCursorPosCallback(std::function<void(double, double)> callback) {
        cursorPosCallback = callback;
    }

private:
    // Mouse state
    glm::vec2 mousePosition;
    glm::vec2 lastMousePosition;
    double scrollDelta;
    
    // Keyboard state
    std::unordered_map<int, bool> keyStates;
    std::unordered_map<int, bool> keyJustPressed;
    
    // Mouse button state
    std::unordered_map<int, bool> mouseButtonStates;
    
    // Callbacks
    std::function<void(int, int, int, int)> keyCallback;
    std::function<void(int, int, int)> mouseButtonCallback;
    std::function<void(double, double)> cursorPosCallback;
    
    // GLFW callback handlers (static)
    static void keyCallbackGLFW(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallbackGLFW(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallbackGLFW(GLFWwindow* window, double xoffset, double yoffset);
    static void cursorPosCallbackGLFW(GLFWwindow* window, double xpos, double ypos);
};

#endif // INPUT_HANDLER_HPP