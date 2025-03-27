#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <unordered_map>

// Forward declaration
class SplashScreen;
namespace Debug {
    class DebugMenu;
}

class Window {
public:
    Window(int width = 1280, int height = 720, const std::string& title = "Voxel Game");
    ~Window();

    bool initialize();
    void swapBuffers();
    bool shouldClose() const;
    void pollEvents();

    // Getters
    GLFWwindow* getHandle() const { return m_window; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getAspectRatio() const { return static_cast<float>(m_width) / m_height; }

    // Input state
    bool isKeyPressed(int key) const;
    bool isKeyJustPressed(int key);
    void getCursorPos(double& x, double& y) const;
    void setCursorPos(double x, double y);
    void setInputMode(int mode, int value);
    
    // Set the active splash screen for character input
    void setActiveSplashScreen(SplashScreen* splashScreen);
    
    // Set the active debug menu for character input
    void setActiveDebugMenu(Debug::DebugMenu* debugMenu);

private:
    GLFWwindow* m_window;
    int m_width;
    int m_height;
    std::string m_title;
    
    // Key state tracking
    std::unordered_map<int, bool> m_prevKeyState;
    
    // UI references for forwarding character events
    SplashScreen* m_activeSplashScreen;
    Debug::DebugMenu* m_activeDebugMenu;
}; 