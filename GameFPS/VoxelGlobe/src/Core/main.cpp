// ./src/Core/main.cpp
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Core/Debug.hpp"
#include "World/World.hpp"
#include "Player/Player.hpp"
#include "Rendering/Renderer.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <glm/gtx/intersect.hpp>

bool g_showDebug = false;
bool g_showMenu = false;
bool g_showVoxelEdges = false;
float g_fov = 70.0f;

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

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    int lastEscapeState = GLFW_RELEASE;
    int lastF8State = GLFW_RELEASE;
    int lastF12State = GLFW_RELEASE;
    int lastLeftClickState = GLFW_RELEASE;
    int lastRightClickState = GLFW_RELEASE;

    double lastTime = glfwGetTime();
    static bool firstFrame = true;

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        int escapeState = glfwGetKey(window, GLFW_KEY_ESCAPE);
        if (escapeState == GLFW_PRESS && lastEscapeState == GLFW_RELEASE) {
            g_showMenu = !g_showMenu;
            glfwSetInputMode(window, GLFW_CURSOR, g_showMenu ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        }
        lastEscapeState = escapeState;

        int f8State = glfwGetKey(window, GLFW_KEY_F8);
        if (f8State == GLFW_PRESS && lastF8State == GLFW_RELEASE) {
            g_showDebug = !g_showDebug;
            std::cout << "Debug toggled: " << (g_showDebug ? "ON" : "OFF") << std::endl;
        }
        lastF8State = f8State;

        int f12State = glfwGetKey(window, GLFW_KEY_F12);
        if (f12State == GLFW_PRESS && lastF12State == GLFW_RELEASE) {
            g_showVoxelEdges = !g_showVoxelEdges;
            std::cout << "Voxel Edges toggled: " << (g_showVoxelEdges ? "ON" : "OFF") << std::endl;
        }
        lastF12State = f12State;

        if (!g_showMenu) {
            player.update(window, deltaTime); // Delegate to Player

            float scrollY = io.MouseWheel;
            if (scrollY != 0) player.inventory.scroll(scrollY);

            int leftClickState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
            if (leftClickState == GLFW_PRESS && lastLeftClickState == GLFW_RELEASE) {
                glm::vec3 rayOrigin = player.position + player.up * 1.75f; // Use hardcoded height for now
                glm::vec3 rayDir = player.cameraDirection;
                for (float t = 0; t < 5.0f; t += 0.1f) {
                    glm::vec3 pos = rayOrigin + rayDir * t;
                    int x = static_cast<int>(floor(pos.x));
                    int y = static_cast<int>(floor(pos.y));
                    int z = static_cast<int>(floor(pos.z));
                    if (world.getBlock(x, y, z).type == BlockType::AIR) {
                        world.setBlock(x, y, z, player.inventory.slots[player.inventory.selectedSlot]);
                        break;
                    }
                }
            }
            lastLeftClickState = leftClickState;

            int rightClickState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
            if (rightClickState == GLFW_PRESS && lastRightClickState == GLFW_RELEASE) {
                glm::vec3 rayOrigin = player.position + player.up * 1.75f;
                glm::vec3 rayDir = player.cameraDirection;
                for (float t = 0; t < 5.0f; t += 0.1f) {
                    glm::vec3 pos = rayOrigin + rayDir * t;
                    int x = static_cast<int>(floor(pos.x));
                    int y = static_cast<int>(floor(pos.y));
                    int z = static_cast<int>(floor(pos.z));
                    if (world.getBlock(x, y, z).type != BlockType::AIR) {
                        world.setBlock(x, y, z, BlockType::AIR);
                        break;
                    }
                }
            }
            lastRightClickState = rightClickState;
        }

        world.update(player.position);
        if (!firstFrame) {
            // Gravity now handled in Player::update()
        }
        firstFrame = false;

        renderer.render(world, player);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (g_showDebug) {
            ImGui::Begin("Debug Info");
            ImGui::Text("Player Pos: %.2f, %.2f, %.2f", player.position.x, player.position.y, player.position.z);
            ImGui::Text("Camera Dir: %.2f, %.2f, %.2f", player.cameraDirection.x, player.cameraDirection.y, player.cameraDirection.z);
            ImGui::Text("Up: %.2f, %.2f, %.2f", player.up.x, player.up.y, player.up.z);
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

        ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y - 40));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, 40));
        ImGui::Begin("Inventory", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
        for (int i = 0; i < 10; i++) {
            ImGui::PushID(i);
            if (i == player.inventory.selectedSlot) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
            if (ImGui::Button(std::to_string(i).c_str(), ImVec2(40, 40))) {
                player.inventory.selectedSlot = i;
            }
            if (i == player.inventory.selectedSlot) ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::PopID();
        }
        ImGui::End();

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