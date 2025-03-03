// ./include/Rendering/Renderer.hpp
#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <GL/glew.h>
#include "World/World.hpp"
#include "Player/Player.hpp"
#include "Graphics/GraphicsSettings.hpp" // Added
#include <glm/glm.hpp>

class Renderer {
public:
    Renderer();
    ~Renderer();
    void render(const World& world, const Player& player, const GraphicsSettings& settings); // Updated parameter

private:
    void renderVoxelEdges(const World& world, const Player& player, const GraphicsSettings& settings); // Updated parameter
    void loadShader();
    void loadEdgeShader();
    void loadTexture();

    GLuint vao, vbo;
    GLuint edgeVao, edgeVbo;
    GLuint shaderProgram;
    GLuint edgeShaderProgram;
    GLuint texture;
};

#endif