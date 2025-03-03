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
    void render(const World& world, const Player& player, const glm::ivec3& highlightedVoxel = glm::ivec3(-1, -1, -1));
    void renderVoxelEdges(const World& world, const Player& player);

private:
    GLuint vao, vbo, shaderProgram, texture;
    GLuint edgeVao, edgeVbo, edgeShaderProgram;
    void loadShader();
    void loadEdgeShader();
    void loadTexture();
};

#endif