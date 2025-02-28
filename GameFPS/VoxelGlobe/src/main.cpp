#include <GL/glew.h>    // GLEW first
#include <GLFW/glfw3.h> // Then GLFW
#include "Renderer.hpp"
#include "World.hpp"
#include "Player.hpp"
#include <iostream>

int main() {
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(800, 600, "Voxel Globe", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE; // Enable experimental features
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW initialization failed" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

    Player player;
    World world;
    Renderer renderer;

    while (!glfwWindowShouldClose(window)) {
        world.update(player.position);
        renderer.render(world, player);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}