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
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>

int main() {
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(800, 600, "Voxel Globe", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW initialization failed" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    World world;
    world.update(glm::vec3(0, 0, 0));
    Player player(world);
    Renderer renderer;
    VoxelManipulator voxelManip(world);
    InventoryUI inventoryUI;
    VoxelHighlightUI voxelHighlightUI;
    DebugManager& debugManager = DebugManager::getInstance();
    DebugWindow debugWindow(debugManager);

    float fov = 70.0f; // FOV now lives here

    for (auto& [key, chunk] : world.getChunks()) {
        chunk.setWorld(&world);
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    int lastEscapeState = GLFW_RELEASE;
    int lastF8State = GLFW_RELEASE; // Changed to F8
    int lastLeftClickState = GLFW_RELEASE;
    int lastRightClickState = GLFW_RELEASE;
    bool showEscapeMenu = false;

    double lastTime = glfwGetTime();
    static bool firstFrame = true;

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        // Input handling
        int escapeState = glfwGetKey(window, GLFW_KEY_ESCAPE);
        if (escapeState == GLFW_PRESS && lastEscapeState == GLFW_RELEASE) {
            showEscapeMenu = !showEscapeMenu;
            glfwSetInputMode(window, GLFW_CURSOR, showEscapeMenu || debugWindow.isVisible() ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        }
        lastEscapeState = escapeState;

        int f8State = glfwGetKey(window, GLFW_KEY_F8);
        if (f8State == GLFW_PRESS && lastF8State == GLFW_RELEASE) {
            debugWindow.toggleVisibility();
            glfwSetInputMode(window, GLFW_CURSOR, debugWindow.isVisible() || showEscapeMenu ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        }
        lastF8State = f8State;

        if (!showEscapeMenu && !debugWindow.isVisible()) {
            player.update(window, deltaTime);

            int leftClickState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
            if (leftClickState == GLFW_PRESS && lastLeftClickState == GLFW_RELEASE) {
                voxelManip.placeBlock(player, player.inventory.slots[player.inventory.selectedSlot]);
            }
            lastLeftClickState = leftClickState;

            int rightClickState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
            if (rightClickState == GLFW_PRESS && lastRightClickState == GLFW_RELEASE) {
                voxelManip.removeBlock(player);
            }
            lastRightClickState = rightClickState;
        }

        world.update(player.position);
        for (auto& [key, chunk] : world.getChunks()) {
            chunk.setWorld(&world);
        }

        if (!firstFrame) {
            // Gravity handled in Player::update()
        }
        firstFrame = false;

        renderer.render(world, player, fov);

        glm::ivec3 hitPos;
        glm::vec3 hitNormal;
        glm::vec3 eyePos = player.position + player.up * player.getHeight();
        if (voxelManip.raycast(eyePos, player.cameraDirection, 5.0f, hitPos, hitNormal, ToolType::NONE)) {
            voxelHighlightUI.render(player, hitPos, fov);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        debugWindow.render(player);

        if (showEscapeMenu) {
            ImGui::Begin("Menu", &showEscapeMenu, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::SliderFloat("FOV", &fov, 30.0f, 110.0f, "%.1f");
            if (ImGui::Button("Close")) {
                showEscapeMenu = false;
                glfwSetInputMode(window, GLFW_CURSOR, debugWindow.isVisible() ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
            }
            ImGui::End();
        }

        inventoryUI.render(player.inventory, io);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glClear(GL_DEPTH_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}