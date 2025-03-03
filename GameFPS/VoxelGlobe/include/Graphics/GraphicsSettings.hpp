// ./include/Graphics/GraphicsSettings.hpp
#ifndef GRAPHICS_SETTINGS_HPP
#define GRAPHICS_SETTINGS_HPP

#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include "../../third_party/nlohmann/json.hpp" 

enum class DisplayMode {
    WINDOWED,
    FULLSCREEN,
    FULLSCREEN_WINDOWED
};

struct Resolution {
    int width;
    int height;
    std::string label;
    bool operator==(const Resolution& other) const {
        return width == other.width && height == other.height;
    }
};

struct MonitorInfo {
    GLFWmonitor* monitor;
    std::string name;
    std::vector<Resolution> resolutions;
};

class GraphicsSettings {
public:
    GraphicsSettings(GLFWwindow* window);
    ~GraphicsSettings();

    void applySettings(bool confineCursor = false); // Added confineCursor parameter
    void renderUI();

    void loadFromConfig();
    void saveToConfig();

    int getWidth() const { return currentResolution.width; }
    int getHeight() const { return currentResolution.height; }
    DisplayMode getMode() const { return currentMode; }

private:
    GLFWwindow* window;
    std::vector<MonitorInfo> monitors; // Changed to store monitor-specific resolutions
    Resolution currentResolution;
    DisplayMode currentMode;
    int selectedMonitorIndex;
    int selectedResolutionIndex;

    void populateMonitors(); // New method to query monitors and resolutions
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
};

#endif