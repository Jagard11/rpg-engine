// ./GameFPS/VoxelGlobe/include/Renderer.hpp
#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <GL/glew.h>
#include "World.hpp"
#include "Player.hpp"

class Renderer {
public:
    Renderer();
    ~Renderer();
    void render(const World& world, const Player& player);
    void renderVoxelEdges(const World& world, const Player& player); // New debug method

private:
    GLuint vao, vbo, shaderProgram, texture;
    GLuint edgeVao, edgeVbo, edgeShaderProgram; // For wireframe edges
    void loadShader();
    void loadEdgeShader(); // For wireframe
    void loadTexture();
};

#endif