#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "renderer/TextureManager.hpp"
#include "world/World.hpp"
#include "player/Player.hpp"
#include "ui/SplashScreen.hpp"
#include "debug/DebugMenu.hpp"

// Forward declarations
class Player;
class World;

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool initialize();
    void render(World* world, Player* player);
    void setupCamera(const Player* player);

    // UI component setters
    void setSplashScreen(SplashScreen* splashScreen) { m_splashScreen = splashScreen; }
    void setDebugMenu(Debug::DebugMenu* debugMenu) { m_debugMenu = debugMenu; }
    
    // Buffer management
    bool isBuffersInitialized() const { return m_buffersInitialized; }
    void setupBuffers();

private:
    void setupShaders();
    void renderChunk(const Chunk* chunk, const glm::mat4& viewProjection);
    void renderWorld(World* world, Player* player);
    void renderPlayerCollisionBox(Player* player);
    void renderHUD();

    GLuint m_shaderProgram;
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;
    
    std::unique_ptr<TextureManager> m_textureManager;

    // Shader uniforms
    GLint m_modelLoc;
    GLint m_viewProjectionLoc;
    GLint m_textureLoc;

    // UI components
    SplashScreen* m_splashScreen;
    Debug::DebugMenu* m_debugMenu;
    
    // Buffer state tracking
    bool m_buffersInitialized;
}; 