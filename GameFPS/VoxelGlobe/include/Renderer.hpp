#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <GL/glew.h>
#include "World.hpp"
#include "Player.hpp"

class Renderer {
public:
    Renderer();
    ~Renderer();
    void render(const World& world, const Player& player); // Match order with implementation

private:
    GLuint vao, vbo, shaderProgram, texture;
    void loadShader();
    void loadTexture();
};

#endif