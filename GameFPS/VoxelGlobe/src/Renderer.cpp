// ./GameFPS/VoxelGlobe/src/Renderer.cpp
#include <GL/glew.h>
#include "Renderer.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "stb_image.h"
#include "Debug.hpp"
#include <iostream>
#include <vector>

extern float g_fov;
extern bool g_showVoxelEdges;

Renderer::Renderer() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    loadShader();

    glGenVertexArrays(1, &edgeVao);
    glGenBuffers(1, &edgeVbo);
    loadEdgeShader();

    loadTexture();
}

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);
    glDeleteVertexArrays(1, &edgeVao);
    glDeleteBuffers(1, &edgeVbo);
    glDeleteProgram(edgeShaderProgram);
    glDeleteTextures(1, &texture);
}

void Renderer::render(const World& world, const Player& player) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glBindTexture(GL_TEXTURE_2D, texture);

    glm::mat4 proj = glm::perspective(glm::radians(g_fov), 800.0f / 600.0f, 0.1f, 2000.0f);
    glm::vec3 eyePos = player.position + player.up * player.height;
    glm::vec3 lookAtPos = eyePos + player.cameraDirection;
    glm::mat4 view = glm::lookAt(eyePos, lookAtPos, player.up);
    if (g_showDebug) {
        std::cout << "Eye Pos: " << eyePos.x << ", " << eyePos.y << ", " << eyePos.z << std::endl;
        std::cout << "LookAt Pos: " << lookAtPos.x << ", " << lookAtPos.y << ", " << lookAtPos.z << std::endl;
    }
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "proj"), 1, GL_FALSE, &proj[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);

    for (const auto& [pos, chunk] : world.getChunks()) {
        int face = pos.first / 1000;
        int localX = pos.first % 1000;
        glm::vec3 sphericalPos = world.cubeToSphere(face, localX, pos.second, 8.0f);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), sphericalPos);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
        const std::vector<float>& mesh = chunk.getMesh();
        if (!mesh.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, mesh.size() * sizeof(float), mesh.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glDrawArrays(GL_TRIANGLES, 0, mesh.size() / 5);
        }
    }

    if (g_showVoxelEdges) {
        renderVoxelEdges(world, player);
    }
}

void Renderer::renderVoxelEdges(const World& world, const Player& player) {
    glUseProgram(edgeShaderProgram);
    glBindVertexArray(edgeVao);

    glm::mat4 proj = glm::perspective(glm::radians(g_fov), 800.0f / 600.0f, 0.1f, 2000.0f);
    glm::vec3 eyePos = player.position + player.up * player.height;
    glm::vec3 lookAtPos = eyePos + player.cameraDirection;
    glm::mat4 view = glm::lookAt(eyePos, lookAtPos, player.up);
    glUniformMatrix4fv(glGetUniformLocation(edgeShaderProgram, "proj"), 1, GL_FALSE, &proj[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(edgeShaderProgram, "view"), 1, GL_FALSE, &view[0][0]);

    std::vector<float> edgeVertices;
    float radius = 5.0f;
    for (const auto& [pos, chunk] : world.getChunks()) {
        int face = pos.first / 1000;
        int localX = pos.first % 1000;
        glm::vec3 chunkBase = world.cubeToSphere(face, localX, pos.second, 8.0f);
        float dist = glm::length(chunkBase - player.position);
        if (dist > radius * Chunk::SIZE) continue;

        const std::vector<float>& mesh = chunk.getMesh();
        for (size_t i = 0; i < mesh.size(); i += 30) {
            float x1 = mesh[i] + chunkBase.x;
            float y1 = mesh[i + 1] + chunkBase.y;
            float z1 = mesh[i + 2] + chunkBase.z;
            float x2 = mesh[i + 5] + chunkBase.x;
            float y2 = mesh[i + 6] + chunkBase.y;
            float z2 = mesh[i + 7] + chunkBase.z;
            float x3 = mesh[i + 10] + chunkBase.x;
            float y3 = mesh[i + 11] + chunkBase.y;
            float z3 = mesh[i + 12] + chunkBase.z;
            float x4 = mesh[i + 25] + chunkBase.x;
            float y4 = mesh[i + 26] + chunkBase.y;
            float z4 = mesh[i + 27] + chunkBase.z;

            edgeVertices.insert(edgeVertices.end(), {x1, y1, z1, x2, y2, z2});
            edgeVertices.insert(edgeVertices.end(), {x2, y2, z2, x3, y3, z3});
            edgeVertices.insert(edgeVertices.end(), {x3, y3, z3, x4, y4, z4});
            edgeVertices.insert(edgeVertices.end(), {x4, y4, z4, x1, y1, z1});
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, edgeVbo);
    glBufferData(GL_ARRAY_BUFFER, edgeVertices.size() * sizeof(float), edgeVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, edgeVertices.size() / 3);
}

void Renderer::loadShader() {
    const char* vertSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 pos;
        layout(location = 1) in vec2 uv;
        out vec2 TexCoord;
        uniform mat4 model, view, proj;
        void main() {
            gl_Position = proj * view * model * vec4(pos, 1.0);
            TexCoord = uv;
        }
    )";
    const char* fragSrc = R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D tex;
        void main() {
            FragColor = texture(tex, TexCoord);
        }
    )";
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vertSrc, NULL);
    glCompileShader(vert);
    GLint success;
    glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(vert, 512, NULL, infoLog);
        std::cerr << "Vertex Shader Error: " << infoLog << std::endl;
    }
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragSrc, NULL);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(frag, 512, NULL, infoLog);
        std::cerr << "Fragment Shader Error: " << infoLog << std::endl;
    }
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vert);
    glAttachShader(shaderProgram, frag);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader Program Link Error: " << infoLog << std::endl;
    }
    glDeleteShader(vert);
    glDeleteShader(frag);
}

void Renderer::loadEdgeShader() {
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
            FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    )";
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vertSrc, NULL);
    glCompileShader(vert);
    GLint success;
    glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(vert, 512, NULL, infoLog);
        std::cerr << "Edge Vertex Shader Error: " << infoLog << std::endl;
    }
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragSrc, NULL);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(frag, 512, NULL, infoLog);
        std::cerr << "Edge Fragment Shader Error: " << infoLog << std::endl;
    }
    edgeShaderProgram = glCreateProgram();
    glAttachShader(edgeShaderProgram, vert);
    glAttachShader(edgeShaderProgram, frag);
    glLinkProgram(edgeShaderProgram);
    glGetProgramiv(edgeShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(edgeShaderProgram, 512, NULL, infoLog);
        std::cerr << "Edge Shader Program Link Error: " << infoLog << std::endl;
    }
    glDeleteShader(vert);
    glDeleteShader(frag);
}

void Renderer::loadTexture() {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, channels;
    unsigned char* data = stbi_load("textures/grass.png", &width, &height, &channels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
        std::cout << "Texture loaded: " << width << "x" << height << ", channels: " << channels << std::endl;
    } else {
        std::cerr << "Failed to load texture: " << stbi_failure_reason() << std::endl;
    }
    stbi_image_free(data);
}