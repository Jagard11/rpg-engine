// ./src/UI/VoxelHighlightUI.cpp
#include "UI/VoxelHighlightUI.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "Debug/DebugManager.hpp"
#include <iostream>
#include <vector>

VoxelHighlightUI::VoxelHighlightUI() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    loadShader();

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    lastHighlightedVoxel = glm::ivec3(-9999, -9999, -9999);
}

VoxelHighlightUI::~VoxelHighlightUI() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);
}

void VoxelHighlightUI::render(const Player& player, const glm::ivec3& voxelPos, const GraphicsSettings& settings) {
    // Skip rendering if invalid position or same as last time
    if (voxelPos.x == -1 || (voxelPos == lastHighlightedVoxel && glm::length(glm::vec3(lastHighlightedVoxel) - player.position) < 20.0f)) {
        return;
    }
    
    lastHighlightedVoxel = voxelPos;

    // Disable culling to ensure we see all edges of the highlight
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    
    // Use depth test but draw slightly offset from actual block
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);

    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Create a wireframe cube slightly larger than the block to avoid z-fighting
    float offset = 0.01f;
    std::vector<float> vertices = {
        // Front face
        0.0f - offset, 0.0f - offset, 0.0f - offset,  1.0f + offset, 0.0f - offset, 0.0f - offset,
        1.0f + offset, 0.0f - offset, 0.0f - offset,  1.0f + offset, 1.0f + offset, 0.0f - offset,
        1.0f + offset, 1.0f + offset, 0.0f - offset,  0.0f - offset, 1.0f + offset, 0.0f - offset,
        0.0f - offset, 1.0f + offset, 0.0f - offset,  0.0f - offset, 0.0f - offset, 0.0f - offset,
        
        // Back face
        0.0f - offset, 0.0f - offset, 1.0f + offset,  1.0f + offset, 0.0f - offset, 1.0f + offset,
        1.0f + offset, 0.0f - offset, 1.0f + offset,  1.0f + offset, 1.0f + offset, 1.0f + offset,
        1.0f + offset, 1.0f + offset, 1.0f + offset,  0.0f - offset, 1.0f + offset, 1.0f + offset,
        0.0f - offset, 1.0f + offset, 1.0f + offset,  0.0f - offset, 0.0f - offset, 1.0f + offset,
        
        // Connect front to back
        0.0f - offset, 0.0f - offset, 0.0f - offset,  0.0f - offset, 0.0f - offset, 1.0f + offset,
        1.0f + offset, 0.0f - offset, 0.0f - offset,  1.0f + offset, 0.0f - offset, 1.0f + offset,
        1.0f + offset, 1.0f + offset, 0.0f - offset,  1.0f + offset, 1.0f + offset, 1.0f + offset,
        0.0f - offset, 1.0f + offset, 0.0f - offset,  0.0f - offset, 1.0f + offset, 1.0f + offset
    };

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Set up view and projection matrices
    glm::mat4 proj = glm::perspective(glm::radians(70.0f), static_cast<float>(settings.getWidth()) / settings.getHeight(), 0.1f, 5000.0f);
    glm::vec3 eyePos = player.position + player.up * player.getHeight();
    glm::vec3 lookAtPos = eyePos + player.cameraDirection;
    glm::mat4 view = glm::lookAt(eyePos, lookAtPos, player.up);

    // Position the highlight at the voxel
    glm::vec3 renderPos = glm::vec3(voxelPos.x, voxelPos.y, voxelPos.z);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), renderPos);
    
    // Apply the transformation matrices
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "proj"), 1, GL_FALSE, &proj[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);

    // Draw the highlight wireframe
    glLineWidth(3.0f);
    glDrawArrays(GL_LINES, 0, vertices.size() / 3);

    // Restore GL state
    glEnable(GL_CULL_FACE);
    glDisable(GL_POLYGON_OFFSET_FILL);

    // Log highlighted voxel if debugging is enabled
    if (DebugManager::getInstance().logBlockPlacement()) {
        glm::ivec3 globalPos = voxelPos + player.getWorld().getLocalOrigin();
        std::cout << "Highlighted voxel at local pos (" << voxelPos.x << ", " << voxelPos.y << ", " << voxelPos.z 
                 << "), world pos (" << globalPos.x << ", " << globalPos.y << ", " << globalPos.z << ")" << std::endl;
    }
}

void VoxelHighlightUI::loadShader() {
    // Vertex shader for the highlight wireframe
    const char* vertSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 pos;
        uniform mat4 model, view, proj;
        void main() {
            gl_Position = proj * view * model * vec4(pos, 1.0);
        }
    )";
    
    // Fragment shader - white color for visibility
    const char* fragSrc = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 1.0, 1.0, 1.0); // White highlight
        }
    )";
    
    // Compile vertex shader
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vertSrc, NULL);
    glCompileShader(vert);
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vert, 512, NULL, infoLog);
        std::cerr << "Highlight Vertex Shader Error: " << infoLog << std::endl;
    }
    
    // Compile fragment shader
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragSrc, NULL);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(frag, 512, NULL, infoLog);
        std::cerr << "Highlight Fragment Shader Error: " << infoLog << std::endl;
    }
    
    // Link shader program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vert);
    glAttachShader(shaderProgram, frag);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Highlight Shader Program Link Error: " << infoLog << std::endl;
    }
    
    // Cleanup shader objects
    glDeleteShader(vert);
    glDeleteShader(frag);
}