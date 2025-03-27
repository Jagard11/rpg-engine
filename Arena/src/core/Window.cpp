#include <GL/glew.h>
#include "core/Window.hpp"
#include <stdexcept>
#include "ui/SplashScreen.hpp"
#include "debug/DebugMenu.hpp"
#include <iostream>

// Global variables to store active UI components
static SplashScreen* g_activeSplashScreen = nullptr;
static Debug::DebugMenu* g_activeDebugMenu = nullptr;

// GLFW character callback
static void character_callback(GLFWwindow* window, unsigned int codepoint) {
    // First try to handle with debug menu if active
    if (g_activeDebugMenu && g_activeDebugMenu->isActive()) {
        g_activeDebugMenu->characterCallback(codepoint);
        std::cout << "Character sent to debug menu: " << (char)codepoint << std::endl;
    }
    // Otherwise forward to splash screen if available
    else if (g_activeSplashScreen) {
        g_activeSplashScreen->characterCallback(codepoint);
    }
}

// Add this before Window::initialize()
static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

Window::Window(int width, int height, const std::string& title)
    : m_window(nullptr)
    , m_width(width)
    , m_height(height)
    , m_title(title)
    , m_activeSplashScreen(nullptr)
    , m_activeDebugMenu(nullptr)
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
    
    // Initialize GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW initialization failed: " << glewGetErrorString(err) << std::endl;
        glfwDestroyWindow(m_window);
        glfwTerminate();
        return false;
    }
    
    // Setup callbacks
    glfwSetCharCallback(m_window, character_callback);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
    
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
    m_activeSplashScreen = splashScreen;
}

void Window::setActiveDebugMenu(Debug::DebugMenu* debugMenu) {
    g_activeDebugMenu = debugMenu;
    m_activeDebugMenu = debugMenu;
} 