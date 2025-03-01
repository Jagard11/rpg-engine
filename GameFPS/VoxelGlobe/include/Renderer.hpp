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

private:
    GLuint vao, vbo, shaderProgram, texture;
    void loadShader();
    void loadTexture();
};

#endif