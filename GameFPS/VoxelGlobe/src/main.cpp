#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Renderer.hpp"
#include "World.hpp"
#include "Player.hpp"
#include "Debug.hpp"
#include <iostream>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

bool g_showDebug = false;
bool g_showMenu = false;
float g_fov = 70.0f; // Default FOV, adjustable via menu

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
    double lastX = 400, lastY = 300;
    bool firstMouse = true;

    // Key state tracking for one-shot toggles
    int lastEscapeState = GLFW_RELEASE;
    int lastF8State = GLFW_RELEASE;

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // Escape menu toggle (one-shot)
        int escapeState = glfwGetKey(window, GLFW_KEY_ESCAPE);
        if (escapeState == GLFW_PRESS && lastEscapeState == GLFW_RELEASE) {
            g_showMenu = !g_showMenu;
            glfwSetInputMode(window, GLFW_CURSOR, g_showMenu ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
            firstMouse = true; // Reset mouse on toggle
        }
        lastEscapeState = escapeState;

        // F8 debug toggle (one-shot)
        int f8State = glfwGetKey(window, GLFW_KEY_F8);
        if (f8State == GLFW_PRESS && lastF8State == GLFW_RELEASE) {
            g_showDebug = !g_showDebug;
            std::cout << "Debug toggled: " << (g_showDebug ? "ON" : "OFF") << std::endl;
        }
        lastF8State = f8State;

        // Mouse and movement input only when menu is closed
        if (!g_showMenu) {
            if (firstMouse) {
                lastX = mouseX;
                lastY = mouseY;
                firstMouse = false;
            }
            float deltaX = static_cast<float>(mouseX - lastX);
            float deltaY = static_cast<float>(mouseY - lastY);
            lastX = mouseX;
            lastY = mouseY;
            player.updateOrientation(deltaX, deltaY);

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) player.moveForward(deltaTime);
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) player.moveBackward(deltaTime);
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) player.moveLeft(deltaTime);
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) player.moveRight(deltaTime);
        }

        player.applyGravity(world, deltaTime);
        world.update(player.position);
        renderer.render(world, player);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Debug window
        if (g_showDebug) {
            ImGui::Begin("Debug Info");
            ImGui::Text("Player Pos: %.2f, %.2f, %.2f", player.position.x, player.position.y, player.position.z);
            ImGui::Text("Camera Dir: %.2f, %.2f, %.2f", player.cameraDirection.x, player.cameraDirection.y, player.cameraDirection.z);
            ImGui::Text("Up: %.2f, %.2f, %.2f", player.up.x, player.up.y, player.up.z);
            ImGui::End();
        }

        // Escape menu with FOV slider
        if (g_showMenu) {
            ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::SliderFloat("FOV", &g_fov, 30.0f, 110.0f, "%.1f");
            if (ImGui::Button("Close")) {
                g_showMenu = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                firstMouse = true; // Reset mouse on close
            }
            ImGui::End();
        }

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