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

void VoxelHighlightUI::render(const Player& player, const glm::ivec3& voxelPos, float fov) {
    if (voxelPos == lastHighlightedVoxel || voxelPos.x == -1) return;
    lastHighlightedVoxel = voxelPos;

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    float offset = 0.01f;
    std::vector<float> vertices = {
        0.0f - offset, 0.0f - offset, 0.0f - offset,  1.0f + offset, 0.0f - offset, 0.0f - offset,
        1.0f + offset, 0.0f - offset, 0.0f - offset,  1.0f + offset, 1.0f + offset, 0.0f - offset,
        1.0f + offset, 1.0f + offset, 0.0f - offset,  0.0f - offset, 1.0f + offset, 0.0f - offset,
        0.0f - offset, 1.0f + offset, 0.0f - offset,  0.0f - offset, 0.0f - offset, 0.0f - offset,
        0.0f - offset, 0.0f - offset, 1.0f + offset,  1.0f + offset, 0.0f - offset, 1.0f + offset,
        1.0f + offset, 0.0f - offset, 1.0f + offset,  1.0f + offset, 1.0f + offset, 1.0f + offset,
        1.0f + offset, 1.0f + offset, 1.0f + offset,  0.0f - offset, 1.0f + offset, 1.0f + offset,
        0.0f - offset, 1.0f + offset, 1.0f + offset,  0.0f - offset, 0.0f - offset, 1.0f + offset,
        0.0f - offset, 0.0f - offset, 0.0f - offset,  0.0f - offset, 0.0f - offset, 1.0f + offset,
        1.0f + offset, 0.0f - offset, 0.0f - offset,  1.0f + offset, 0.0f - offset, 1.0f + offset,
        1.0f + offset, 1.0f + offset, 0.0f - offset,  1.0f + offset, 1.0f + offset, 1.0f + offset,
        0.0f - offset, 1.0f + offset, 0.0f - offset,  0.0f - offset, 1.0f + offset, 1.0f + offset
    };

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glm::mat4 proj = glm::perspective(glm::radians(fov), 800.0f / 600.0f, 0.1f, 2000.0f);
    glm::vec3 eyePos = player.position + player.up * player.getHeight();
    glm::vec3 lookAtPos = eyePos + player.cameraDirection;
    glm::mat4 view = glm::lookAt(eyePos, lookAtPos, player.up);

    glm::vec3 renderPos = glm::vec3(voxelPos.x, voxelPos.y, voxelPos.z);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), renderPos);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "proj"), 1, GL_FALSE, &proj[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);

    glLineWidth(3.0f);
    glDrawArrays(GL_LINES, 0, vertices.size() / 3);

    glEnable(GL_CULL_FACE);

    if (DebugManager::getInstance().logBlockPlacement()) {
        std::cout << "Highlighted voxel at world pos (" << renderPos.x << ", " << renderPos.y << ", " << renderPos.z << ")" << std::endl;
    }
}

void VoxelHighlightUI::loadShader() {
    const char* vertSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 pos;
        uniform mat4 model, view, proj;
        void main() {
            gl_Position = proj * view * model * vec4(pos, 1.0);
        }
    )";
    const char* fragSrc = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 1.0, 1.0, 1.0); // White highlight
        }
    )";
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
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragSrc, NULL);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(frag, 512, NULL, infoLog);
        std::cerr << "Highlight Fragment Shader Error: " << infoLog << std::endl;
    }
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vert);
    glAttachShader(shaderProgram, frag);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Highlight Shader Program Link Error: " << infoLog << std::endl;
    }
    glDeleteShader(vert);
    glDeleteShader(frag);
}