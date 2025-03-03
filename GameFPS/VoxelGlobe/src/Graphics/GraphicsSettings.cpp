// ./src/Graphics/GraphicsSettings.cpp
#include "Graphics/GraphicsSettings.hpp"
#include "imgui.h"
#include <iostream>
#include <fstream>
#include "../../third_party/nlohmann/json.hpp"

using json = nlohmann::json;

GraphicsSettings::GraphicsSettings(GLFWwindow* window) 
    : window(window), 
      currentResolution({800, 600, "800x600"}), 
      currentMode(DisplayMode::WINDOWED), 
      selectedMonitorIndex(0), 
      selectedResolutionIndex(0) {
    populateMonitors();
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    loadFromConfig();
    applySettings();
}

GraphicsSettings::~GraphicsSettings() {
    saveToConfig();
}

void GraphicsSettings::populateMonitors() {
    int monitorCount;
    GLFWmonitor** glfwMonitors = glfwGetMonitors(&monitorCount);
    monitors.clear();

    for (int i = 0; i < monitorCount; ++i) {
        MonitorInfo info;
        info.monitor = glfwMonitors[i];
        info.name = glfwGetMonitorName(glfwMonitors[i]);

        int modeCount;
        const GLFWvidmode* modes = glfwGetVideoModes(glfwMonitors[i], &modeCount);
        for (int j = 0; j < modeCount; ++j) {
            Resolution res;
            res.width = modes[j].width;
            res.height = modes[j].height;
            res.label = std::to_string(res.width) + "x" + std::to_string(res.height);
            // Avoid duplicates
            if (std::find(info.resolutions.begin(), info.resolutions.end(), res) == info.resolutions.end()) {
                info.resolutions.push_back(res);
            }
        }
        monitors.push_back(info);
    }

    // Default to primary monitor’s first resolution if no config exists
    if (!monitors.empty() && !monitors[0].resolutions.empty()) {
        currentResolution = monitors[0].resolutions[0];
    }
}

void GraphicsSettings::applySettings(bool confineCursor) {
    GLFWmonitor* monitor = (monitors.empty() || selectedMonitorIndex < 0 || selectedMonitorIndex >= monitors.size()) 
        ? glfwGetPrimaryMonitor() 
        : monitors[selectedMonitorIndex].monitor;
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    switch (currentMode) {
        case DisplayMode::WINDOWED:
            glfwSetWindowMonitor(window, nullptr, 100, 100, currentResolution.width, currentResolution.height, 0);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Ensure cursor is free in windowed mode
            break;
        case DisplayMode::FULLSCREEN:
            glfwSetWindowMonitor(window, monitor, 0, 0, currentResolution.width, currentResolution.height, mode->refreshRate);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Fullscreen typically confines cursor
            break;
        case DisplayMode::FULLSCREEN_WINDOWED:
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            currentResolution = {mode->width, mode->height, std::string("Fullscreen Windowed")};
            if (confineCursor) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Confine cursor when active
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Free cursor in menus
            }
            break;
    }

    glViewport(0, 0, currentResolution.width, currentResolution.height);
}

void GraphicsSettings::renderUI() {
    ImGui::Begin("Graphics Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    // Monitor selection
    std::vector<const char*> monitorNames;
    for (const auto& mon : monitors) {
        monitorNames.push_back(mon.name.c_str());
    }
    if (ImGui::Combo("Monitor", &selectedMonitorIndex, monitorNames.data(), monitorNames.size())) {
        if (selectedMonitorIndex >= 0 && selectedMonitorIndex < monitors.size()) {
            selectedResolutionIndex = 0; // Reset resolution index when monitor changes
            currentResolution = monitors[selectedMonitorIndex].resolutions[0];
            applySettings();
        }
    }

    // Resolution dropdown (for current monitor)
    if (selectedMonitorIndex >= 0 && selectedMonitorIndex < monitors.size()) {
        std::vector<const char*> resolutionLabels;
        for (const auto& res : monitors[selectedMonitorIndex].resolutions) {
            resolutionLabels.push_back(res.label.c_str());
        }
        if (ImGui::Combo("Resolution", &selectedResolutionIndex, resolutionLabels.data(), resolutionLabels.size())) {
            currentResolution = monitors[selectedMonitorIndex].resolutions[selectedResolutionIndex];
            if (currentMode != DisplayMode::FULLSCREEN_WINDOWED) {
                applySettings();
            }
        }
    }

    // Display mode radio buttons
    int modeInt = static_cast<int>(currentMode);
    if (ImGui::RadioButton("Windowed", &modeInt, static_cast<int>(DisplayMode::WINDOWED))) {
        currentMode = DisplayMode::WINDOWED;
        applySettings();
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Fullscreen", &modeInt, static_cast<int>(DisplayMode::FULLSCREEN))) {
        currentMode = DisplayMode::FULLSCREEN;
        applySettings();
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Fullscreen Windowed", &modeInt, static_cast<int>(DisplayMode::FULLSCREEN_WINDOWED))) {
        currentMode = DisplayMode::FULLSCREEN_WINDOWED;
        applySettings(true); // Confine cursor by default in fullscreen windowed
    }

    ImGui::End();
}

void GraphicsSettings::loadFromConfig() {
    std::ifstream file("config.json");
    if (file.is_open()) {
        json config;
        file >> config;

        if (config.contains("monitor_index")) {
            selectedMonitorIndex = config["monitor_index"];
            if (selectedMonitorIndex < 0 || selectedMonitorIndex >= monitors.size()) {
                selectedMonitorIndex = 0; // Fallback to primary monitor
            }
        }

        if (config.contains("resolution_index")) {
            selectedResolutionIndex = config["resolution_index"];
            if (selectedMonitorIndex >= 0 && selectedMonitorIndex < monitors.size() &&
                selectedResolutionIndex >= 0 && selectedResolutionIndex < monitors[selectedMonitorIndex].resolutions.size()) {
                currentResolution = monitors[selectedMonitorIndex].resolutions[selectedResolutionIndex];
            }
        }

        if (config.contains("display_mode")) {
            int mode = config["display_mode"];
            if (mode >= 0 && mode <= static_cast<int>(DisplayMode::FULLSCREEN_WINDOWED)) {
                currentMode = static_cast<DisplayMode>(mode);
            }
        }

        file.close();
    } else {
        std::cerr << "No config.json found, using defaults" << std::endl;
    }
}

void GraphicsSettings::saveToConfig() {
    json config;
    config["monitor_index"] = selectedMonitorIndex;
    config["resolution_index"] = selectedResolutionIndex;
    config["display_mode"] = static_cast<int>(currentMode);

    std::ofstream file("config.json");
    if (file.is_open()) {
        file << config.dump(4);
        file.close();
    } else {
        std::cerr << "Failed to save config.json" << std::endl;
    }
}

void GraphicsSettings::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}