#pragma once

#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

// Forward declarations
class Game;
class World;
class Player;
namespace Render {
    class TextRenderer;
}

namespace Debug {

/**
 * DebugStats class for displaying real-time game statistics
 * on screen, including FPS, position, and chunk information.
 */
class DebugStats {
public:
    DebugStats();
    ~DebugStats();

    // Initialize the stats display with required dependencies
    void initialize(GLFWwindow* window, Game* game);
    
    // Update stats (called every frame)
    void update(float deltaTime);
    
    // Render the stats overlay
    void render();
    
    // Toggle visibility
    void toggleVisibility() { 
        std::cout << "[DebugStats] Before toggle: m_isVisible = " << m_isVisible << std::endl;
        m_isVisible = !m_isVisible; 
        std::cout << "[DebugStats] After toggle: m_isVisible = " << m_isVisible << std::endl;
    }
    
    // Check if stats are visible
    bool isVisible() const { return m_isVisible; }

private:
    // Helpers
    void updateFpsStats(float deltaTime);
    
    // Helper to draw text manually using OpenGL primitives
    void drawManualText(const std::string& text, float x, float y);
    
    // Dependencies
    GLFWwindow* m_window;
    Game* m_game;
    
    // Text renderer for displaying stats
    std::unique_ptr<Render::TextRenderer> m_textRenderer;
    
    // Stats visibility
    bool m_isVisible;
    
    // FPS tracking
    int m_currentFps;
    float m_highFps;
    float m_lowFps;
    float m_avgFps;
    float m_fpsAccumulator;
    int m_fpsFrameCount;
    
    // History of FPS measurements for averaging
    std::deque<float> m_fpsHistory;
    
    // Last update time
    float m_lastUpdateTime;
};

} // namespace Debug 