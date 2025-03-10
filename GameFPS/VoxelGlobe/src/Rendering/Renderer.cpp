// ./src/Rendering/Renderer.cpp
#include "Rendering/Renderer.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "World/World.hpp"
#include "Player/Player.hpp"
#include "Graphics/GraphicsSettings.hpp"
#include "Graphics/ResourceManager.hpp"
#include "Graphics/ShaderManager.hpp"
#include "Debug/DebugManager.hpp"
#include "Debug/DebugSystem.hpp"
#include "Debug/Logger.hpp"
#include "Debug/Profiler.hpp"
#include "../../third_party/stb/stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <sstream>

// Frustum structure for culling
struct Frustum {
    glm::vec4 planes[6];
    
    Frustum(const glm::mat4& vp) {
        planes[0] = glm::normalize(glm::vec4(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0], vp[3][3] + vp[3][0])); // Left
        planes[1] = glm::normalize(glm::vec4(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0], vp[3][3] - vp[3][0])); // Right
        planes[2] = glm::normalize(glm::vec4(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1], vp[3][3] + vp[3][1])); // Bottom
        planes[3] = glm::normalize(glm::vec4(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1], vp[3][3] - vp[3][1])); // Top
        planes[4] = glm::normalize(glm::vec4(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2], vp[3][3] + vp[3][2])); // Near
        planes[5] = glm::normalize(glm::vec4(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2], vp[3][3] - vp[3][2])); // Far
    }
    
    bool isSphereInFrustum(const glm::vec3& center, float radius) const {
        for (int i = 0; i < 6; i++) {
            if (glm::dot(glm::vec3(planes[i]), center) + planes[i].w < -radius) {
                return false;
            }
        }
        return true;
    }
};

// Helper function to check OpenGL errors
static void checkGLError(const char* location) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::stringstream ss;
        ss << "OpenGL Error at " << location << ": " << err;
        LOG_ERROR(LogCategory::RENDERING, ss.str());
    }
}

Renderer::Renderer() : frameCounter(0) {
    PROFILE_SCOPE("Renderer::Constructor", LogCategory::RENDERING);
    
    // Initialize vertex arrays and buffers
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    
    glGenVertexArrays(1, &edgeVao);
    glGenBuffers(1, &edgeVbo);
    
    // Load shaders and texture
    loadShader();
    loadEdgeShader();
    loadTexture();
    
    LOG_INFO(LogCategory::RENDERING, "Renderer initialized");
}

Renderer::~Renderer() {
    PROFILE_SCOPE("Renderer::Destructor", LogCategory::RENDERING);
    
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shaderProgram);
    
    glDeleteVertexArrays(1, &edgeVao);
    glDeleteBuffers(1, &edgeVbo);
    glDeleteProgram(edgeShaderProgram);
    
    glDeleteTextures(1, &texture);
    
    LOG_INFO(LogCategory::RENDERING, "Renderer resources released");
}

void Renderer::render(World& world, const Player& player, const GraphicsSettings& settings) {
    PROFILE_SCOPE("Renderer::render", LogCategory::RENDERING);
    frameCounter++;
    
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
    checkGLError("post-clear");

    // Set up OpenGL state
    glEnable(GL_DEPTH_TEST);
    if (DebugManager::getInstance().isCullingEnabled()) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CW);
    } else {
        glDisable(GL_CULL_FACE);
    }

    // Setup projection and view matrices
    float fovDegrees = 70.0f;
    glm::mat4 proj = glm::perspective(
        glm::radians(fovDegrees),
        static_cast<float>(settings.getWidth()) / settings.getHeight(),
        0.1f,    // Near plane
        10000.0f // Far plane
    );
    
    float playerHeight = player.getHeight();
    glm::vec3 eyePos = glm::vec3(0, playerHeight, 0); // Player eye position in player-relative space
    glm::vec3 lookAtPos = eyePos + player.cameraDirection;
    glm::vec3 viewDir = glm::normalize(lookAtPos - eyePos);
    glm::vec3 rightDir = glm::normalize(glm::cross(viewDir, player.up));
    glm::vec3 upDir = glm::normalize(glm::cross(rightDir, viewDir));
    glm::mat4 view = glm::lookAt(eyePos, lookAtPos, upDir);
    glm::mat4 vp = proj * view;

    // Create frustum for culling
    Frustum frustum(vp);

    // Activate shader and texture
    {
        PROFILE_SCOPE("Shader_Setup", LogCategory::RENDERING);
        glUseProgram(shaderProgram);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "proj"), 1, GL_FALSE, &proj[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
        glUniform1i(glGetUniformLocation(shaderProgram, "useFaceColors"), DebugManager::getInstance().useFaceColors());
        glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 0);
    }

    // Get player world position
    glm::vec3 playerWorldPos = player.position;

    // Get local origin for coordinate rebasing
    glm::ivec3 localOrigin = world.getLocalOrigin();
    
    // Pass player offset to shader
    glUniform3fv(glGetUniformLocation(shaderProgram, "playerOffset"), 1, &playerWorldPos[0]);
    
    // Pass Earth radius to shader
    float earthRadius = static_cast<float>(world.getRadius());
    glUniform1f(glGetUniformLocation(shaderProgram, "earthRadius"), earthRadius);

    // Rendering statistics
    int renderedChunks = 0;
    int totalChunks = 0;
    int skippedChunks = 0;
    bool printDebug = (frameCounter % 60 == 0);
    
    if (printDebug) {
        std::stringstream ss;
        ss << "------- RENDERING FRAME " << frameCounter << " -------";
        LOG_DEBUG(LogCategory::RENDERING, ss.str());
        
        ss.str("");
        ss << "Player at: " << playerWorldPos.x << ", " << playerWorldPos.y << ", " << playerWorldPos.z;
        LOG_DEBUG(LogCategory::RENDERING, ss.str());
    }

    float maxRenderDistance = 5000.0f;

    // Render all chunks
    {
        PROFILE_SCOPE("RenderChunks", LogCategory::RENDERING);
        
        for (auto& [key, chunk] : world.getChunks()) {
            totalChunks++;

            int chunkX = std::get<0>(key);
            int chunkY = std::get<1>(key);
            int chunkZ = std::get<2>(key);
            int chunkSize = Chunk::SIZE * chunk->getMergeFactor();

            // Calculate chunk origin in world space (corrected from center)
            glm::vec3 chunkOriginWorld(
                chunkX * Chunk::SIZE,
                chunkY * Chunk::SIZE,
                chunkZ * Chunk::SIZE
            );

            // Calculate chunk origin relative to player
            glm::vec3 chunkRelativeOrigin = chunkOriginWorld - playerWorldPos;

            // For culling, calculate center relative to player
            glm::vec3 chunkCenterRelative = chunkRelativeOrigin + glm::vec3(chunkSize / 2.0f);

            // Distance culling
            float dist = glm::length(chunkCenterRelative);
            if (dist > maxRenderDistance) {
                skippedChunks++;
                continue;
            }

            // Frustum culling
            bool inFrustum = frustum.isSphereInFrustum(chunkCenterRelative, chunkSize * 0.866f);
            if (!inFrustum) {
                skippedChunks++;
                continue;
            }

            // Regenerate mesh if dirty
            if (chunk->isMeshDirty()) {
                PROFILE_SCOPE("RegenerateMesh", LogCategory::RENDERING);
                chunk->regenerateMesh();
            }

            // Initialize or update buffers
            if (!chunk->isBuffersInitialized()) {
                PROFILE_SCOPE("InitBuffers", LogCategory::RENDERING);
                chunk->initializeBuffers();
            } else if (chunk->isBuffersDirty()) {
                PROFILE_SCOPE("UpdateBuffers", LogCategory::RENDERING);
                chunk->updateBuffers();
            }

            if (chunk->getIndexCount() == 0) {
                skippedChunks++;
                continue;
            }

            // Set up model matrix with player-relative transform using chunk origin
            glm::mat4 model = glm::translate(glm::mat4(1.0f), chunkRelativeOrigin);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);

            // Pass chunk world position for shader adjustments
            glUniform3fv(glGetUniformLocation(shaderProgram, "chunkWorldPos"), 1, &chunkOriginWorld[0]);

            // Draw chunk
            {
                PROFILE_SCOPE("Chunk::draw", LogCategory::RENDERING);
                chunk->bindVAO();
                glDrawElements(GL_TRIANGLES, chunk->getIndexCount(), GL_UNSIGNED_INT, 0);
                checkGLError("post-draw");
            }

            renderedChunks++;
        }
    }

    if (printDebug) {
        std::stringstream ss;
        ss << "Rendered chunks: " << renderedChunks << " / " << totalChunks
           << " (Skipped: " << skippedChunks << ")";
        LOG_DEBUG(LogCategory::RENDERING, ss.str());
        
        // Report profiling results every 60 frames if profiling is enabled
        if (Profiler::getInstance().isEnabled()) {
            Profiler::getInstance().reportResults();
        }
    }

    if (DebugManager::getInstance().showVoxelEdges()) {
        renderVoxelEdges(world, player, settings);
    }
}

void Renderer::renderVoxelEdges(const World& world, const Player& player, const GraphicsSettings& settings) {
    PROFILE_SCOPE("Renderer::renderVoxelEdges", LogCategory::RENDERING);
    
    glUseProgram(edgeShaderProgram);
    glBindVertexArray(edgeVao);

    glm::mat4 proj = glm::perspective(glm::radians(70.0f), static_cast<float>(settings.getWidth()) / settings.getHeight(), 0.1f, 10000.0f);
    glm::vec3 eyePos = glm::vec3(0, player.getHeight(), 0);
    glm::vec3 lookAtPos = eyePos + player.cameraDirection;
    glm::vec3 viewDir = glm::normalize(lookAtPos - eyePos);
    glm::vec3 rightDir = glm::normalize(glm::cross(viewDir, player.up));
    glm::vec3 upDir = glm::normalize(glm::cross(rightDir, viewDir));
    glm::mat4 view = glm::lookAt(eyePos, lookAtPos, upDir);

    glUniformMatrix4fv(glGetUniformLocation(edgeShaderProgram, "proj"), 1, GL_FALSE, &proj[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(edgeShaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniform3fv(glGetUniformLocation(edgeShaderProgram, "playerOffset"), 1, &player.position[0]);

    std::vector<float> edgeVertices;
    float radius = 100.0f;

    for (const auto& [key, chunk] : world.getChunks()) {
        int chunkX = std::get<0>(key);
        int chunkY = std::get<1>(key);
        int chunkZ = std::get<2>(key);

        glm::vec3 chunkOriginWorld(chunkX * Chunk::SIZE, chunkY * Chunk::SIZE, chunkZ * Chunk::SIZE);
        glm::vec3 chunkRelativeOrigin = chunkOriginWorld - player.position;

        float dist = glm::length(chunkRelativeOrigin + glm::vec3(Chunk::SIZE / 2.0f));
        if (dist > radius) continue;

        const std::vector<float>& mesh = chunk->getMesh();
        if (mesh.size() < 20) continue;

        for (size_t i = 0; i < mesh.size(); i += 20) {
            if (i + 19 >= mesh.size()) break;

            float x1 = mesh[i]     + chunkX * Chunk::SIZE - player.position.x;
            float y1 = mesh[i + 1] + chunkY * Chunk::SIZE - player.position.y;
            float z1 = mesh[i + 2] + chunkZ * Chunk::SIZE - player.position.z;

            float x2 = mesh[i + 5]  + chunkX * Chunk::SIZE - player.position.x;
            float y2 = mesh[i + 6]  + chunkY * Chunk::SIZE - player.position.y;
            float z2 = mesh[i + 7]  + chunkZ * Chunk::SIZE - player.position.z;

            float x3 = mesh[i + 10] + chunkX * Chunk::SIZE - player.position.x;
            float y3 = mesh[i + 11] + chunkY * Chunk::SIZE - player.position.y;
            float z3 = mesh[i + 12] + chunkZ * Chunk::SIZE - player.position.z;

            float x4 = mesh[i + 15] + chunkX * Chunk::SIZE - player.position.x;
            float y4 = mesh[i + 16] + chunkY * Chunk::SIZE - player.position.y;
            float z4 = mesh[i + 17] + chunkZ * Chunk::SIZE - player.position.z;

            edgeVertices.insert(edgeVertices.end(), {x1, y1, z1, x2, y2, z2});
            edgeVertices.insert(edgeVertices.end(), {x2, y2, z2, x3, y3, z3});
            edgeVertices.insert(edgeVertices.end(), {x3, y3, z3, x4, y4, z4});
            edgeVertices.insert(edgeVertices.end(), {x4, y4, z4, x1, y1, z1});
        }
    }

    if (!edgeVertices.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, edgeVbo);
        glBufferData(GL_ARRAY_BUFFER, edgeVertices.size() * sizeof(float), edgeVertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glLineWidth(2.0f);
        glDrawArrays(GL_LINES, 0, edgeVertices.size() / 3);
        checkGLError("post-edge-draw");
    }
}

void Renderer::loadShader() {
    PROFILE_SCOPE("Renderer::loadShader", LogCategory::RENDERING);
    
    const char* vertSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 pos;
        layout(location = 1) in vec2 uv;
        out vec2 TexCoord;
        uniform mat4 model, view, proj;
        uniform vec3 playerOffset;
        uniform vec3 chunkWorldPos;
        uniform float earthRadius;
        
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
        uniform bool useFaceColors;
        
        void main() {
            if (useFaceColors) {
                float faceId = floor(TexCoord.x * 4.0);
                if (faceId == 0.0) FragColor = vec4(1.0, 0.0, 0.0, 1.0);
                else if (faceId == 1.0) FragColor = vec4(0.0, 1.0, 0.0, 1.0);
                else if (faceId == 2.0) FragColor = vec4(0.0, 0.0, 1.0, 1.0);
                else if (faceId == 3.0) FragColor = vec4(1.0, 1.0, 0.0, 1.0);
                else if (faceId == 4.0) FragColor = vec4(1.0, 0.0, 1.0, 1.0);
                else FragColor = vec4(0.0, 1.0, 1.0, 1.0);
            } else {
                FragColor = texture(tex, TexCoord);
            }
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
        LOG_ERROR(LogCategory::RENDERING, std::string("Vertex Shader Error: ") + infoLog);
    }
    
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragSrc, NULL);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(frag, 512, NULL, infoLog);
        LOG_ERROR(LogCategory::RENDERING, std::string("Fragment Shader Error: ") + infoLog);
    }
    
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vert);
    glAttachShader(shaderProgram, frag);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        LOG_ERROR(LogCategory::RENDERING, std::string("Shader Program Link Error: ") + infoLog);
    }
    
    glDeleteShader(vert);
    glDeleteShader(frag);

    // Set up vertex attributes
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBindVertexArray(0);
    
    LOG_INFO(LogCategory::RENDERING, "Default shader loaded successfully");
}

void Renderer::loadEdgeShader() {
    PROFILE_SCOPE("Renderer::loadEdgeShader", LogCategory::RENDERING);
    
    const char* vertSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 pos;
        uniform mat4 view, proj;
        uniform vec3 playerOffset;
        
        void main() {
            gl_Position = proj * view * vec4(pos, 1.0);
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
    GLchar infoLog[512];
    glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vert, 512, NULL, infoLog);
        LOG_ERROR(LogCategory::RENDERING, std::string("Edge Vertex Shader Error: ") + infoLog);
    }
    
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragSrc, NULL);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(frag, 512, NULL, infoLog);
        LOG_ERROR(LogCategory::RENDERING, std::string("Edge Fragment Shader Error: ") + infoLog);
    }
    
    edgeShaderProgram = glCreateProgram();
    glAttachShader(edgeShaderProgram, vert);
    glAttachShader(edgeShaderProgram, frag);
    glLinkProgram(edgeShaderProgram);
    glGetProgramiv(edgeShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(edgeShaderProgram, 512, NULL, infoLog);
        LOG_ERROR(LogCategory::RENDERING, std::string("Edge Shader Program Link Error: ") + infoLog);
    }
    
    glDeleteShader(vert);
    glDeleteShader(frag);

    glBindVertexArray(edgeVao);
    glBindBuffer(GL_ARRAY_BUFFER, edgeVbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    
    LOG_INFO(LogCategory::RENDERING, "Edge shader loaded successfully");
}

void Renderer::loadTexture() {
    PROFILE_SCOPE("Renderer::loadTexture", LogCategory::RENDERING);
    
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, channels;
    unsigned char* data = stbi_load("textures/blocks.png", &width, &height, &channels, 0);
    
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, 
                     channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
        
        std::stringstream ss;
        ss << "Texture loaded: " << width << "x" << height << ", channels: " << channels;
        LOG_INFO(LogCategory::RENDERING, ss.str());
    } else {
        LOG_ERROR(LogCategory::RENDERING, std::string("Failed to load texture: ") + stbi_failure_reason());
        
        const int atlasSize = 512;
        const int tileSize = 128;
        unsigned char* atlas = new unsigned char[atlasSize * atlasSize * 4];
        
        for (int y = 0; y < atlasSize; y++) {
            for (int x = 0; x < atlasSize; x++) {
                int idx = (y * atlasSize + x) * 4;
                int tileX = x / tileSize;
                int tileY = y / tileSize;
                int tile = tileY * (atlasSize / tileSize) + tileX;
                
                int patternX = (x % tileSize) / (tileSize/8);
                int patternY = (y % tileSize) / (tileSize/8);
                bool isAlternate = (patternX + patternY) % 2 == 0;
                
                switch (tile) {
                    case 0: // Grass
                        atlas[idx + 0] = isAlternate ? 34 : 85;
                        atlas[idx + 1] = isAlternate ? 139 : 170;
                        atlas[idx + 2] = isAlternate ? 34 : 85;
                        atlas[idx + 3] = 255;
                        break;
                    case 1: // Dirt
                        atlas[idx + 0] = isAlternate ? 139 : 160;
                        atlas[idx + 1] = isAlternate ? 69 : 90;
                        atlas[idx + 2] = isAlternate ? 19 : 40;
                        atlas[idx + 3] = 255;
                        break;
                    default:
                        atlas[idx + 0] = 255;
                        atlas[idx + 1] = 105;
                        atlas[idx + 2] = 180;
                        atlas[idx + 3] = 255;
                        break;
                }
            }
        }
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlasSize, atlasSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas);
        LOG_INFO(LogCategory::RENDERING, "Created fallback texture atlas: " + std::to_string(atlasSize) + "x" + std::to_string(atlasSize));
        
        delete[] atlas;
    }
    
    stbi_image_free(data);
}