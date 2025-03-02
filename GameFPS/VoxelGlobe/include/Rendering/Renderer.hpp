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
    void render(const World& world, const Player& player, const glm::ivec3& highlightedVoxel = glm::ivec3(-1, -1, -1)); // Default to no highlight
    void renderVoxelEdges(const World& world, const Player& player);

private:
    GLuint vao, vbo, shaderProgram, texture;
    GLuint edgeVao, edgeVbo, edgeShaderProgram;
    GLuint highlightVao, highlightVbo, highlightShaderProgram; // Added missing declarations
    glm::ivec3 lastHighlightedVoxel; // Added missing declaration
    void loadShader();
    void loadEdgeShader();
    void loadTexture();
    void renderHighlightedVoxel(const glm::ivec3& voxelPos, const Player& player); // Added declaration
    void loadHighlightShader(); // Added declaration
};

#endif