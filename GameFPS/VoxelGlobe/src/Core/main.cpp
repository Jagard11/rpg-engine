// ./src/Core/main.cpp
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Debug/DebugManager.hpp"
#include "Debug/DebugWindow.hpp"
#include "World/World.hpp"
#include "Player/Player.hpp"
#include "Rendering/Renderer.hpp"
#include "VoxelManipulator.hpp"
#include "UI/Inventory/InventoryUI.hpp"
#include "UI/VoxelHighlightUI.hpp"
#include "Graphics/GraphicsSettings.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <string>

// Loading screen function
void renderLoadingScreen(GLFWwindow* window, float progress) {
    // Clear the buffer
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Get window dimensions
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    // Setup orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Disable depth test for 2D rendering
    glDisable(GL_DEPTH_TEST);
    
    // Draw loading bar background
    int barWidth = width * 0.7f;
    int barHeight = 30;
    int barX = (width - barWidth) / 2;
    int barY = height / 2;
    
    glBegin(GL_QUADS);
    glColor3f(0.3f, 0.3f, 0.3f);
    glVertex2f(barX, barY);
    glVertex2f(barX + barWidth, barY);
    glVertex2f(barX + barWidth, barY + barHeight);
    glVertex2f(barX, barY + barHeight);
    glEnd();
    
    // Draw loading progress
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.6f, 0.0f);
    glVertex2f(barX, barY);
    glVertex2f(barX + barWidth * progress, barY);
    glVertex2f(barX + barWidth * progress, barY + barHeight);
    glVertex2f(barX, barY + barHeight);
    glEnd();
    
    // Reset color to white
    glColor3f(1.0f, 1.0f, 1.0f);
    
    // Swap buffers
    glfwSwapBuffers(window);
    
    // Poll for events
    glfwPollEvents();
}

// Helper function to consistently manage cursor state
void updateCursorMode(GLFWwindow* window, bool showEscapeMenu, bool showDebugWindow, DisplayMode displayMode) {
    if (showEscapeMenu || showDebugWindow) {
        // Always show cursor when menus are open
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
        // Always disable cursor in gameplay - regardless of display mode
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Create window with OpenGL context
    GLFWwindow* window = glfwCreateWindow(1024, 768, "Voxel Globe", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW initialization failed" << std::endl;
        return -1;
    }
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    // Show loading screen
    renderLoadingScreen(window, 0.1f);
    
    // Initialize debug manager
    DebugManager& debugManager = DebugManager::getInstance();
    debugManager.setLogChunkUpdates(true);
    debugManager.setLogPlayerInfo(true);
    debugManager.setLogBlockPlacement(true);
    debugManager.setLogRaycast(true);
    
    // Initialize core game components
    std::cout << "Creating world..." << std::endl;
    World world;
    renderLoadingScreen(window, 0.3f);
    
    std::cout << "Creating player..." << std::endl;
    Player player(world);
    renderLoadingScreen(window, 0.4f);
    
    std::cout << "Creating renderer..." << std::endl;
    Renderer renderer;
    renderLoadingScreen(window, 0.5f);
    
    std::cout << "Creating additional components..." << std::endl;
    VoxelManipulator voxelManip(world);
    renderLoadingScreen(window, 0.6f);
    
    InventoryUI inventoryUI;
    renderLoadingScreen(window, 0.7f);
    
    VoxelHighlightUI voxelHighlightUI;
    renderLoadingScreen(window, 0.8f);
    
    DebugWindow debugWindow(debugManager, player);
    renderLoadingScreen(window, 0.9f);
    
    GraphicsSettings graphicsSettings(window);
    
    // Initial world generation (first set of chunks around player)
    std::cout << "Generating initial chunks..." << std::endl;
    world.update(player.position);
    
    // Initialize chunk buffers
    std::cout << "Initializing chunk buffers..." << std::endl;
    for (auto& [key, chunk] : world.getChunks()) {
        chunk->setWorld(&world);
        chunk->initializeBuffers();
    }
    
    renderLoadingScreen(window, 1.0f);
    
    // Signal that initial loading is complete (enables physics)
    std::cout << "Loading complete, enabling physics" << std::endl;
    player.finishLoading();
    
    // Initialize cursor to disabled state
    updateCursorMode(window, false, false, graphicsSettings.getMode());
    
    // Track keyboard/mouse states
    int lastEscapeState = GLFW_RELEASE;
    int lastF8State = GLFW_RELEASE;
    int lastLeftClickState = GLFW_RELEASE;
    int lastRightClickState = GLFW_RELEASE;
    bool showEscapeMenu = false;
    
    // Timing variables for game loop
    double lastTime = glfwGetTime();
    double currentTime;
    float deltaTime;
    int frameCount = 0;
    double fpsTime = 0.0;
    float fov = 70.0f;
    
    // Flag to track if we're on the first frame
    static bool firstFrame = true;
    
    std::cout << "Starting main game loop" << std::endl;
    
    // Main game loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        currentTime = glfwGetTime();
        deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;
        
        // FPS counter
        frameCount++;
        fpsTime += deltaTime;
        if (fpsTime >= 1.0) {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            fpsTime = 0.0;
        }
        
        // Handle escape key for menu
        int escapeState = glfwGetKey(window, GLFW_KEY_ESCAPE);
        if (escapeState == GLFW_PRESS && lastEscapeState == GLFW_RELEASE) {
            showEscapeMenu = !showEscapeMenu;
            updateCursorMode(window, showEscapeMenu, debugWindow.isVisible(), graphicsSettings.getMode());
        }
        lastEscapeState = escapeState;
        
        // Handle F8 key for debug window
        int f8State = glfwGetKey(window, GLFW_KEY_F8);
        if (f8State == GLFW_PRESS && lastF8State == GLFW_RELEASE) {
            debugWindow.toggleVisibility();
            updateCursorMode(window, showEscapeMenu, debugWindow.isVisible(), graphicsSettings.getMode());
        }
        lastF8State = f8State;
        
        // Update player if menus are closed
        if (!showEscapeMenu && !debugWindow.isVisible()) {
            player.update(window, deltaTime);
            
            // Handle block placement (left click)
            int leftClickState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
            if (leftClickState == GLFW_PRESS && lastLeftClickState == GLFW_RELEASE) {
                BlockType selectedBlock = player.inventory.slots[player.inventory.selectedSlot];
                if (selectedBlock != BlockType::AIR) {
                    if (voxelManip.placeBlock(player, selectedBlock)) {
                        std::cout << "Block placed successfully: " << static_cast<int>(selectedBlock) << std::endl;
                    } else {
                        std::cout << "Failed to place block" << std::endl;
                    }
                }
            }
            lastLeftClickState = leftClickState;
            
            // Handle block removal (right click)
            int rightClickState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
            if (rightClickState == GLFW_PRESS && lastRightClickState == GLFW_RELEASE) {
                if (voxelManip.removeBlock(player)) {
                    std::cout << "Block removed successfully" << std::endl;
                } else {
                    std::cout << "Failed to remove block" << std::endl;
                }
            }
            lastRightClickState = rightClickState;
        }
        
        // Update world (load/unload chunks around player)
        world.update(player.position);
        
        // Skip gravity on first frame to avoid falling through terrain
        if (!firstFrame) {
            // Gravity is handled in Player::update()
        }
        firstFrame = false;
        
        // Render the world
        renderer.render(world, player, graphicsSettings);
        
        // Render block highlight
        glm::ivec3 hitPos;
        glm::vec3 hitNormal;
        glm::vec3 eyePos = player.position + player.up * player.getHeight();
        if (voxelManip.raycast(eyePos, player.cameraDirection, 5.0f, hitPos, hitNormal, ToolType::NONE)) {
            voxelHighlightUI.render(player, hitPos, graphicsSettings);
        }
        
        // Begin ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Render debug window if visible
        debugWindow.render();
        
        // Render escape menu if visible
        if (showEscapeMenu) {
            ImGui::Begin("Menu", &showEscapeMenu, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::SliderFloat("FOV", &fov, 30.0f, 110.0f, "%.1f");
            graphicsSettings.renderUI();
            if (ImGui::Button("Close")) {
                showEscapeMenu = false;
                updateCursorMode(window, false, debugWindow.isVisible(), graphicsSettings.getMode());
            }
            ImGui::End();
        }
        
        // Render inventory UI
        inventoryUI.render(player.inventory, io);
        
        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        // Periodically check cursor state to ensure consistency
        if (!firstFrame && frameCount % 30 == 0) {
            updateCursorMode(window, showEscapeMenu, debugWindow.isVisible(), graphicsSettings.getMode());
        }
        
        // Limit frame rate if needed for testing
        if (deltaTime < 0.016) { // Cap at ~60 FPS during debugging
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((0.016 - deltaTime) * 1000)));
        }
    }
    
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}