#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
#include <map>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "render/TextRenderer.hpp"

// Forward declarations
class Game;

namespace Debug {

// FreeType text rendering
struct Character {
    GLuint TextureID;       // ID of glyph texture
    glm::ivec2 Size;        // Size of glyph
    glm::ivec2 Bearing;     // Offset from baseline to left/top of glyph
    GLuint Advance;         // Horizontal offset to advance to next glyph
};

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();
    
    void renderText(const std::string& text, float x, float y, float scale, const glm::vec3& color);
    void updateProjection(float width, float height);
    
    // Test rendering function
    void renderTestText();
    
    // Character map
    std::map<GLchar, Character> Characters;

protected:
    // Fallback rendering method when regular text rendering fails
    void renderFallbackText(const std::string& text, float x, float y, float scale, const glm::vec3& color);

    // OpenGL resources
    GLuint VAO;               // Vertex Array Object
    GLuint VBO;               // Vertex Buffer Object
    GLuint shaderProgram;     // Shader program for text rendering

    // Shader uniform locations
    GLint textUniformLoc;     // Location of text sampler uniform
    GLint textColorUniformLoc; // Location of text color uniform
    GLint projectionUniformLoc; // Location of projection matrix uniform

    // FreeType resources
    FT_Library ft;            // FreeType library instance
    FT_Face face;             // FreeType face object

    // Projection matrix
    glm::mat4 projection;
};

// Command structure to store debug commands
struct Command {
    std::string name;
    std::string description;
    std::function<void(const std::vector<std::string>&)> callback;
};

// Command history entry
struct CommandHistoryEntry {
    std::string text;
    bool isCommand;
};

// Debug menu class for handling voxel debugging
class DebugMenu {
public:
    DebugMenu();
    ~DebugMenu();

    // Initialize the debug menu
    void initialize(GLFWwindow* window, Game* game);
    
    // Key handling
    bool handleKeyPress(int key, int action);
    
    // Character input
    void characterCallback(unsigned int codepoint);
    
    // Update and render functions
    void update(float deltaTime);
    void render();
    
    // Add a command to the debug menu
    void registerCommand(const std::string& name, 
                         const std::string& description,
                         std::function<void(const std::vector<std::string>&)> callback);
    
    // Add output to the debug console
    void commandOutput(const std::string& text);
    
    // Check if the debug menu is active
    bool isActive() const { return m_isActive; }
    
    // Toggle the debug menu visibility
    void toggleVisibility();

    void renderSplashScreen();

private:
    // Helper methods for rendering
    void renderBox(float x, float y, float width, float height);
    void renderText(const std::string& text, float x, float y, float scale, const glm::vec3& color);
    void renderTextAtCenter(const std::string& text, float centerX, float centerY, float scale);
    void renderCharacter(char c, float x, float y, float width, float height, float scale);
    void parseCommand(const std::string& input);
    std::vector<std::string> splitString(const std::string& input, char delimiter);
    
    // Command history navigation
    void navigateHistory(bool up);
    
    GLFWwindow* m_window;
    Game* m_game;
    
    bool m_isActive;
    bool m_isTyping;
    std::string m_commandInput;
    std::string m_currentInput;
    std::vector<CommandHistoryEntry> m_commandHistory;
    int m_historyIndex;
    
    std::unordered_map<std::string, Command> m_commands;
    std::vector<std::string> m_outputLines;
    
    float m_lastUpdateTime;
    bool m_cursorVisible;

    // Text renderer
    std::unique_ptr<Render::TextRenderer> m_textRenderer;
};

} // namespace Debug 