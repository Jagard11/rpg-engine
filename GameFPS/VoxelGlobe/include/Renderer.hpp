// ./VoxelGlobe/include/Renderer.hpp
#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <GL/glew.h>
#include "World.hpp"
#include "Player.hpp"

class Renderer {
public:
    Renderer();
    ~Renderer();
    void render(const World& world, const Player& player, const glm::ivec3& voxelPos); // Updated signature
    void renderVoxelEdges(const World& world, const Player& player);
    void renderHighlightedVoxel(const glm::ivec3& voxelPos, const Player& player); // Added

private:
    GLuint vao, vbo, shaderProgram;
    GLuint edgeVao, edgeVbo, edgeShaderProgram;
    GLuint highlightVao, highlightVbo, highlightShaderProgram;
    GLuint texture;
    glm::ivec3 lastHighlightedVoxel;

    void loadShader();
    void loadEdgeShader();
    void loadHighlightShader();
    void loadTexture();
};

#endif