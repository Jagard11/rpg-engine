// ./src/Core/main.cpp
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Core/Debug.hpp"
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

// Debug globals
bool g_showDebug = true; // Enabled by default
bool g_showMenu = false;
bool g_showVoxelEdges = false;
float g_fov = 70.0f;
bool g_enableCulling = true;
bool g_useFaceColors = false;

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

    for (auto& [key, chunk] : world.getChunks()) {
        chunk.setWorld(&world);
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    int lastEscapeState = GLFW_RELEASE;
    int lastF12State = GLFW_RELEASE;
    int lastLeftClickState = GLFW_RELEASE;
    int lastRightClickState = GLFW_RELEASE;
    int lastF7State = GLFW_RELEASE; // Culling toggle
    int lastF8State = GLFW_RELEASE; // Face colors toggle

    double lastTime = glfwGetTime();
    static bool firstFrame = true;
    bool debugSettingsChanged = false; // Track changes to force mesh update

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        // Input handling
        int escapeState = glfwGetKey(window, GLFW_KEY_ESCAPE);
        if (escapeState == GLFW_PRESS && lastEscapeState == GLFW_RELEASE) {
            g_showMenu = !g_showMenu;
            glfwSetInputMode(window, GLFW_CURSOR, g_showMenu ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        }
        lastEscapeState = escapeState;

        int f12State = glfwGetKey(window, GLFW_KEY_F12);
        if (f12State == GLFW_PRESS && lastF12State == GLFW_RELEASE) {
            g_showVoxelEdges = !g_showVoxelEdges;
            std::cout << "Voxel Edges toggled: " << (g_showVoxelEdges ? "ON" : "OFF") << std::endl;
        }
        lastF12State = f12State;

        int f7State = glfwGetKey(window, GLFW_KEY_F7);
        if (f7State == GLFW_PRESS && lastF7State == GLFW_RELEASE) {
            g_enableCulling = !g_enableCulling;
            std::cout << "Culling toggled: " << (g_enableCulling ? "ON" : "OFF") << std::endl;
            debugSettingsChanged = true;
        }
        lastF7State = f7State;

        int f8State = glfwGetKey(window, GLFW_KEY_F8);
        if (f8State == GLFW_PRESS && lastF8State == GLFW_RELEASE) {
            g_useFaceColors = !g_useFaceColors;
            std::cout << "Face Colors toggled: " << (g_useFaceColors ? "ON" : "OFF") << std::endl;
            debugSettingsChanged = true;
        }
        lastF8State = f8State;

        if (!g_showMenu) {
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
            if (debugSettingsChanged) {
                chunk.regenerateMesh(); // Update mesh only when settings change
            }
        }
        debugSettingsChanged = false; // Reset flag after update

        if (!firstFrame) {
            // Gravity handled in Player::update()
        }
        firstFrame = false;

        renderer.render(world, player);

        glm::ivec3 hitPos;
        glm::vec3 hitNormal;
        glm::vec3 eyePos = player.position + player.up * player.getHeight();
        if (voxelManip.raycast(eyePos, player.cameraDirection, 5.0f, hitPos, hitNormal, ToolType::NONE)) {
            voxelHighlightUI.render(player, hitPos);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (g_showDebug) {
            ImGui::Begin("Debug Tools");
            ImGui::Text("Player Pos: %.2f, %.2f, %.2f", player.position.x, player.position.y, player.position.z);
            ImGui::Text("Camera Dir: %.2f, %.2f, %.2f", player.cameraDirection.x, player.cameraDirection.y, player.cameraDirection.z);
            ImGui::Separator();
            ImGui::Checkbox("Enable Culling (F7)", &g_enableCulling);
            ImGui::Text("Culling State: %s", g_enableCulling ? "ON" : "OFF");
            ImGui::Checkbox("Use Face Colors (F8)", &g_useFaceColors);
            ImGui::Text("Face Colors: %s", g_useFaceColors ? "ON" : "OFF");
            ImGui::End();
        }

        if (g_showMenu) {
            ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::SliderFloat("FOV", &g_fov, 30.0f, 110.0f, "%.1f");
            if (ImGui::Button("Close")) {
                g_showMenu = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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