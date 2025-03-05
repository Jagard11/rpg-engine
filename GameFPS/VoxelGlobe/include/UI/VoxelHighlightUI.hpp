// ./include/UI/VoxelHighlightUI.hpp
#ifndef VOXEL_HIGHLIGHT_UI_HPP
#define VOXEL_HIGHLIGHT_UI_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include "Player/Player.hpp"
#include "Graphics/GraphicsSettings.hpp"

class VoxelHighlightUI {
public:
    VoxelHighlightUI();
    ~VoxelHighlightUI();
    void render(const Player& player, const glm::ivec3& voxelPos, const GraphicsSettings& settings);

private:
    GLuint vao, vbo, shaderProgram;
    glm::ivec3 lastHighlightedVoxel;
    void loadShader();
    
    // Helper method to project a point to the planet surface
    glm::vec3 projectToSphere(const glm::vec3& pos, float surfaceR, double distFromCenter) const;
};

#endif // VOXEL_HIGHLIGHT_UI_HPP