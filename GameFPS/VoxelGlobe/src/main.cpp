// ./VoxelGlobe/src/main.cpp
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Renderer.hpp"
#include "World.hpp"
#include "Player.hpp"
#include "VoxelManipulator.hpp"
#include "Debug.hpp"
#include <iostream>
#include <glm/gtx/intersect.hpp>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

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
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    World world;
    world.update(glm::vec3(0, 0, 0));
    Player player(world);
    VoxelManipulator voxelManip(world);
    Renderer renderer;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    double lastX = 400, lastY = 300;
    bool firstMouse = true;

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

        glfwPollEvents();

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        int escapeState = glfwGetKey(window, GLFW_KEY_ESCAPE);
        if (escapeState == GLFW_PRESS && lastEscapeState == GLFW_RELEASE) {
            g_showMenu = !g_showMenu;
            glfwSetInputMode(window, GLFW_CURSOR, g_showMenu ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
            firstMouse = true;
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
            if (firstMouse) {
                lastX = mouseX;
                lastY = mouseY;
                firstMouse = false;
            }
            float deltaX = static_cast<float>(mouseX - lastX);
            float deltaY = static_cast<float>(lastY - mouseY); // Inverted for natural control
            lastX = mouseX;
            lastY = mouseY;
            player.updateOrientation(deltaX, deltaY);

            float scrollY = io.MouseWheel;
            if (scrollY != 0) player.scrollInventory(scrollY);

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) player.moveForward(deltaTime);
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) player.moveBackward(deltaTime);
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) player.moveLeft(deltaTime);
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) player.moveRight(deltaTime);

            int leftClickState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
            if (leftClickState == GLFW_PRESS && lastLeftClickState == GLFW_RELEASE) {
                voxelManip.placeBlock(player, player.inventory[player.selectedSlot]);
            }
            lastLeftClickState = leftClickState;

            int rightClickState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
            if (rightClickState == GLFW_PRESS && lastRightClickState == GLFW_RELEASE) {
                voxelManip.removeBlock(player);
            }
            lastRightClickState = rightClickState;
        }

        world.update(player.position);
        if (!firstFrame) {
            player.applyGravity(world, deltaTime);
        }
        firstFrame = false;

        if (g_showDebug) {
            int chunkX = static_cast<int>(player.position.x / Chunk::SIZE);
            int chunkZ = static_cast<int>(player.position.z / Chunk::SIZE);
            float surfaceY = world.findSurfaceHeight(chunkX, chunkZ);
            std::cout << "Player Y: " << player.position.y << ", Surface Y: " << surfaceY << std::endl;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // In the main loop, after raycast:
        glm::vec3 eyePos = player.position + player.up * player.height;
        glm::ivec3 hitPos;
        glm::vec3 hitNormal;
        bool hit = voxelManip.raycast(eyePos, player.cameraDirection, 10.0f, hitPos, hitNormal);
        if (hit) {
            // Convert hitPos to world coordinates
            hitPos.y += static_cast<int>(1591.55f + 8.0f); // Add chunkBaseY
            if (g_showDebug) {
                std::cout << "Raycast hit at (world): " << hitPos.x << ", " << hitPos.y << ", " << hitPos.z << std::endl;
            }
        } else {
            hitPos = glm::ivec3(-9999, -9999, -9999);
            if (g_showDebug) std::cout << "No raycast hit" << std::endl;
        }
        renderer.render(world, player, hitPos);

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
                firstMouse = true;
            }
            ImGui::End();
        }

        ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y - 40));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, 40));
        ImGui::Begin("Inventory", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
        for (int i = 0; i < 10; i++) {
            ImGui::PushID(i);
            if (i == player.selectedSlot) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
            if (ImGui::Button(std::to_string(i).c_str(), ImVec2(40, 40))) {
                player.selectedSlot = i;
            }
            if (i == player.selectedSlot) ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::PopID();
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}