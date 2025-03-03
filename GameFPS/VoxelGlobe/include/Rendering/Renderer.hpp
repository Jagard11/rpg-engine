// ./include/Rendering/Renderer.hpp
#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <GL/glew.h>
#include "World/World.hpp"
#include "Player/Player.hpp"
#include <glm/glm.hpp>

class Renderer {
public:
    Renderer();
    ~Renderer();
    void render(const World& world, const Player& player);
private:
    void renderVoxelEdges(const World& world, const Player& player);
    void loadShader();
    void loadEdgeShader();
    void loadTexture();

    GLuint vao, vbo;
    GLuint edgeVao, edgeVbo;
    GLuint shaderProgram;
    GLuint edgeShaderProgram; // Fixed from "GL uint" to "GLuint"
    GLuint texture;
};

#endif