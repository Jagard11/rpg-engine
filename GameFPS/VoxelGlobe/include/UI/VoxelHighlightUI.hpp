// ./include/UI/VoxelHighlightUI.hpp
#ifndef VOXEL_HIGHLIGHT_UI_HPP
#define VOXEL_HIGHLIGHT_UI_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include "Player/Player.hpp"
#include "Graphics/GraphicsSettings.hpp" // Added

class VoxelHighlightUI {
public:
    VoxelHighlightUI();
    ~VoxelHighlightUI();
    void render(const Player& player, const glm::ivec3& voxelPos, const GraphicsSettings& settings); // Updated parameter

private:
    GLuint vao, vbo, shaderProgram;
    glm::ivec3 lastHighlightedVoxel;
    void loadShader();
};

#endif