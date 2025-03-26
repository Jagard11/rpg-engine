#include <GL/glew.h>
#include "core/Window.hpp"
#include <stdexcept>
#include "ui/SplashScreen.hpp"

// Global variable to store the active SplashScreen
static SplashScreen* g_activeSplashScreen = nullptr;

// GLFW character callback
static void character_callback(GLFWwindow* window, unsigned int codepoint) {
    if (g_activeSplashScreen) {
        g_activeSplashScreen->characterCallback(codepoint);
    }
}

Window::Window(int width, int height, const std::string& title)
    : m_window(nullptr)
    , m_width(width)
    , m_height(height)
    , m_title(title)
{}

Window::~Window() {
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

bool Window::initialize() {
    if (!glfwInit()) {
        return false;
    }

    // Set OpenGL version and compatibility profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    
    // Setup character callback
    glfwSetCharCallback(m_window, character_callback);
    
    return true;
}

void Window::swapBuffers() {
    glfwSwapBuffers(m_window);
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void Window::pollEvents() {
    // Update previous key states before polling
    for (auto& pair : m_prevKeyState) {
        pair.second = glfwGetKey(m_window, pair.first) == GLFW_PRESS;
    }
    
    glfwPollEvents();
}

bool Window::isKeyPressed(int key) const {
    return glfwGetKey(m_window, key) == GLFW_PRESS;
}

bool Window::isKeyJustPressed(int key) {
    bool currentState = glfwGetKey(m_window, key) == GLFW_PRESS;
    
    // If we haven't seen this key before, add it to our map
    if (m_prevKeyState.find(key) == m_prevKeyState.end()) {
        m_prevKeyState[key] = false;
    }
    
    // Check if key was just pressed (current = pressed, previous = not pressed)
    bool justPressed = currentState && !m_prevKeyState[key];
    
    // Update previous state for next frame
    m_prevKeyState[key] = currentState;
    
    return justPressed;
}

void Window::getCursorPos(double& x, double& y) const {
    glfwGetCursorPos(m_window, &x, &y);
}

void Window::setCursorPos(double x, double y) {
    glfwSetCursorPos(m_window, x, y);
}

void Window::setInputMode(int mode, int value) {
    glfwSetInputMode(m_window, mode, value);
}

void Window::setActiveSplashScreen(SplashScreen* splashScreen) {
    g_activeSplashScreen = splashScreen;
} 