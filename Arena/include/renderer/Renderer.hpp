#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <memory>
#include "TextureManager.hpp"
#include "../world/World.hpp"
#include "../player/Player.hpp"

// Forward declarations
class Player;
class World;

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool initialize();
    void render(World* world, Player* player);

private:
    void setupShaders();
    void setupBuffers();
    void renderChunk(const Chunk* chunk, const glm::mat4& viewProjection);
    void setupCamera(const Player* player);
    void renderWorld(World* world, Player* player);
    void renderPlayerCollisionBox(Player* player);

    GLuint m_shaderProgram;
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;
    
    std::unique_ptr<TextureManager> m_textureManager;

    // Shader uniforms
    GLint m_modelLoc;
    GLint m_viewProjectionLoc;
    GLint m_textureLoc;
}; 