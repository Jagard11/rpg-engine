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
#include "../world/Chunk.hpp"

// Forward declarations
class Player;
class World;
class Chunk;
class LodChunk;

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
    
    // Debug toggle methods
    void setDisableGreedyMeshing(bool disable) { m_disableGreedyMeshing = disable; }
    void setDisableBackfaceCulling(bool disable) { m_disableBackfaceCulling = disable; }
    bool isGreedyMeshingDisabled() const { return m_disableGreedyMeshing; }
    bool isBackfaceCullingDisabled() const { return m_disableBackfaceCulling; }

    // LOD settings methods
    void setEnableLodRendering(bool enable) { m_enableLodRendering = enable; }
    void setLodRenderDistance(float distance) { m_lodRenderDistance = distance; }
    bool isLodRenderingEnabled() const { return m_enableLodRendering; }
    float getLodRenderDistance() const { return m_lodRenderDistance; }

    // Debug visualization
    void setShowCollisionBox(bool show) { m_showCollisionBox = show; }
    bool isShowingCollisionBox() const { return m_showCollisionBox; }
    void setPlayer(Player* player) { m_player = player; }

private:
    void setupShaders();
    void renderChunk(const Chunk* chunk, const glm::mat4& viewProjection);
    void renderWorld(World* world, Player* player);
    void renderPlayerCollisionBox(Player* player);
    void renderHUD();

    GLuint m_shaderProgram;
    GLuint m_hudShaderProgram;  // Added for HUD rendering
    GLuint m_textureAtlas;      // Added for texture atlas
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;
    GLuint m_hudVao;           // Added for HUD VAO
    GLuint m_hudVbo;           // Added for HUD VBO
    GLuint m_hudEbo;           // Added for HUD EBO
    
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
    
    // Debug toggles
    bool m_disableGreedyMeshing = false;
    bool m_disableBackfaceCulling = false;
    
    // LOD settings
    bool m_enableLodRendering = true;
    float m_lodRenderDistance = 1609.34f * 4.0f; // 4 miles in meters

    bool m_showCollisionBox;  // State for collision box visualization
    Player* m_player;  // Pointer to the player for collision box rendering
}; 