// ./include/Rendering/Renderer.hpp
#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <GL/glew.h>
#include "World/World.hpp"
#include "Player/Player.hpp"
#include "Graphics/GraphicsSettings.hpp"
#include <glm/glm.hpp>

class Renderer {
public:
    Renderer();
    ~Renderer();
    void render(World& world, const Player& player, const GraphicsSettings& settings);

private:
    void renderVoxelEdges(const World& world, const Player& player, const GraphicsSettings& settings);
    void loadShader();
    void loadEdgeShader();
    void loadTexture();

    GLuint vao, vbo, ebo;
    GLuint edgeVao, edgeVbo;
    GLuint shaderProgram;
    GLuint edgeShaderProgram;
    GLuint texture;
    
    // Frame counter for debug output
    int frameCounter;
};

#endif