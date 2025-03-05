// ./src/Rendering/Renderer.cpp
#include <GL/glew.h>
#include "Rendering/Renderer.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "../../third_party/stb/stb_image.h"
#include "Debug/DebugManager.hpp"
#include <iostream>
#include <vector>
#include "Utils/SphereUtils.hpp" // Added for standardized calculations

struct Frustum {
    glm::vec4 planes[6];
    
    // Construct frustum from view-projection matrix
    Frustum(const glm::mat4& vp) {
        planes[0] = glm::normalize(glm::vec4(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0], vp[3][3] + vp[3][0])); // Left
        planes[1] = glm::normalize(glm::vec4(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0], vp[3][3] - vp[3][0])); // Right
        planes[2] = glm::normalize(glm::vec4(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1], vp[3][3] + vp[3][1])); // Bottom
        planes[3] = glm::normalize(glm::vec4(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1], vp[3][3] - vp[3][1])); // Top
        planes[4] = glm::normalize(glm::vec4(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2], vp[3][3] + vp[3][2])); // Near
        planes[5] = glm::normalize(glm::vec4(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2], vp[3][3] - vp[3][2])); // Far
    }
    
    // Check if sphere is in frustum
    bool isSphereInFrustum(const glm::vec3& center, float radius) const {
        for (int i = 0; i < 6; i++) {
            if (glm::dot(glm::vec3(planes[i]), center) + planes[i].w < -radius) {
                return false;
            }
        }
        return true;
    }
};

static void checkGLError(const char* location) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL Error at " << location << ": " << err << std::endl;
    }
}

Renderer::Renderer() {
    if (!glfwGetCurrentContext()) {
        std::cerr << "No active OpenGL context in Renderer constructor!" << std::endl;
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    loadShader();

    glGenVertexArrays(1, &edgeVao);
    glGenBuffers(1, &edgeVbo);
    loadEdgeShader();

    loadTexture();
    
    frameCounter = 0;
}

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shaderProgram);
    glDeleteVertexArrays(1, &edgeVao);
    glDeleteBuffers(1, &edgeVbo);
    glDeleteProgram(edgeShaderProgram);
    glDeleteTextures(1, &texture);
}

void Renderer::render(World& world, const Player& player, const GraphicsSettings& settings) {
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
    // CRITICAL FIX: Adjust near and far planes for Earth-scale rendering
    float fovDegrees = 70.0f;
    glm::mat4 proj = glm::perspective(
        glm::radians(fovDegrees),
        static_cast<float>(settings.getWidth()) / settings.getHeight(),
        0.1f,    // Near plane increased from 0.001f to 0.1f for better depth precision
        10000.0f // Far plane decreased from 100000.0f to 10000.0f for better depth precision
    );
    
    float playerHeight = player.getHeight();
    
    // CRITICAL FIX: Create a coordinate system centered on the player
    // This is key to solving floating-point precision issues
    
    // First, reset the coordinate system to be centered on the player
    // All calculations below will be relative to player position
    glm::vec3 eyePos = glm::vec3(0, playerHeight, 0); // Player eye position in player-relative space
    glm::vec3 lookAtPos = eyePos + player.cameraDirection; // Look at position in player-relative space
    
    // Ensure up vector is orthogonal to view direction for correct camera orientation
    glm::vec3 viewDir = glm::normalize(lookAtPos - eyePos);
    glm::vec3 rightDir = glm::normalize(glm::cross(viewDir, player.up));
    glm::vec3 upDir = glm::normalize(glm::cross(rightDir, viewDir));
    
    // Use the corrected up vector for the lookAt matrix
    glm::mat4 view = glm::lookAt(eyePos, lookAtPos, upDir);
    glm::mat4 vp = proj * view;
    
    // Create frustum for culling using player-relative coordinate system
    Frustum frustum(vp);

    // Activate shader and texture
    glUseProgram(shaderProgram);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "proj"), 1, GL_FALSE, &proj[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniform1i(glGetUniformLocation(shaderProgram, "useFaceColors"), DebugManager::getInstance().useFaceColors());

    // Get the player's actual world position for debugging and relative calculations
    glm::vec3 playerWorldPos = player.position;
    
    // Get local origin for coordinate rebasing
    glm::ivec3 localOrigin = world.getLocalOrigin();
    
    // Calculate the offset values for rebasing - this is critical for Earth-scale rendering
    glm::vec3 playerOffset = player.position;
    glUniform3fv(glGetUniformLocation(shaderProgram, "playerOffset"), 1, &playerOffset[0]);
    
    // CRITICAL FIX: Add Earth radius scale information to shader
    float earthRadius = static_cast<float>(world.getRadius());
    glUniform1f(glGetUniformLocation(shaderProgram, "earthRadius"), earthRadius);
    
    // Tracking rendering statistics
    int renderedChunks = 0;
    int totalChunks = 0;
    int skippedChunks = 0;
    
    // Print debug info every 60 frames
    bool printDebug = (frameCounter % 60 == 0);
    
    if (printDebug) {
        std::cout << "------- RENDERING FRAME " << frameCounter << " -------" << std::endl;
        std::cout << "Player at: " << player.position.x << ", " << player.position.y << ", " << player.position.z << std::endl;
        std::cout << "Up vector: " << player.up.x << ", " << player.up.y << ", " << player.up.z << std::endl;
        std::cout << "Camera dir: " << player.cameraDirection.x << ", " << player.cameraDirection.y << ", " << player.cameraDirection.z << std::endl;
        std::cout << "Local origin: " << localOrigin.x << ", " << localOrigin.y << ", " << localOrigin.z << std::endl;
        std::cout << "Total chunks: " << world.getChunks().size() << std::endl;
    }
    
    // Get planet radius for visibility calculations
    float surfaceRadius = static_cast<float>(SphereUtils::getSurfaceRadiusMeters());
    
    // Limit visible distance - this helps with both performance and precision
    float maxRenderDistance = 5000.0f; // Reduced from half planet radius to a fixed 5km distance
    
    // Render all chunks using player-relative coordinates
    for (auto& [key, chunk] : world.getChunks()) {
        totalChunks++;
        
        // Get chunk coordinates
        int chunkX = std::get<0>(key);
        int chunkY = std::get<1>(key);
        int chunkZ = std::get<2>(key);
        int chunkSize = Chunk::SIZE * chunk->getMergeFactor();
        
        // Calculate chunk center world position
        glm::vec3 chunkWorldCenter(
            chunkX * Chunk::SIZE + Chunk::SIZE / 2.0f,
            chunkY * Chunk::SIZE + Chunk::SIZE / 2.0f,
            chunkZ * Chunk::SIZE + Chunk::SIZE / 2.0f
        );
        
        // CRITICAL FIX: Calculate chunk position relative to player
        glm::vec3 chunkRelativeCenter = chunkWorldCenter - playerWorldPos;
        
        // Print first few chunks for debug
        if (printDebug && totalChunks <= 5) {
            std::cout << "Chunk " << totalChunks << " at (" << chunkX << ", " << chunkY << ", " << chunkZ 
                      << ") center (rel to player): " << chunkRelativeCenter.x << ", " 
                      << chunkRelativeCenter.y << ", " << chunkRelativeCenter.z << std::endl;
        }
        
        // Apply distance culling - skip chunks that are too far away
        float dist = glm::length(chunkRelativeCenter);
        if (dist > maxRenderDistance) {
            skippedChunks++;
            continue;
        }
        
        // Apply frustum culling in player-relative space
        bool inFrustum = frustum.isSphereInFrustum(chunkRelativeCenter, chunkSize * 0.866f);
        if (!inFrustum) {
            skippedChunks++;
            continue;
        }
        
        // If mesh is dirty, regenerate it
        if (chunk->isMeshDirty()) {
            chunk->regenerateMesh();
        }
        
        // Make sure buffers are initialized and updated
        if (!chunk->isBuffersInitialized()) {
            chunk->initializeBuffers();
        } else if (chunk->isBuffersDirty()) {
            chunk->updateBuffers();
        }
        
        // Skip chunks with no mesh
        if (chunk->getIndexCount() == 0) {
            skippedChunks++;
            if (printDebug && totalChunks <= 10) {
                std::cout << "Skipping chunk " << totalChunks << " - no mesh data" << std::endl;
            }
            continue;
        }
        
        // Set up model matrix with player-relative transform
        // This is the most critical part - we're transforming chunk coordinates to be relative to player
        glm::mat4 model = glm::translate(glm::mat4(1.0f), chunkRelativeCenter);
        
        // Apply model matrix
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
        
        // Add uniform for correcting vertices in the shader
        glUniform3fv(glGetUniformLocation(shaderProgram, "chunkWorldPos"), 1, &chunkWorldCenter[0]);
        
        // Bind VAO and draw
        chunk->bindVAO();
        glDrawElements(GL_TRIANGLES, chunk->getIndexCount(), GL_UNSIGNED_INT, 0);
        checkGLError("post-draw");

        renderedChunks++;
    }
    
    if (printDebug) {
        std::cout << "Rendered chunks: " << renderedChunks << " / " << totalChunks
                  << " (Skipped: " << skippedChunks << ")" << std::endl;
    }

    if (DebugManager::getInstance().showVoxelEdges()) {
        renderVoxelEdges(world, player, settings);
    }
}

void Renderer::renderVoxelEdges(const World& world, const Player& player, const GraphicsSettings& settings) {
    glUseProgram(edgeShaderProgram);
    glBindVertexArray(edgeVao);

    glm::mat4 proj = glm::perspective(glm::radians(70.0f), static_cast<float>(settings.getWidth()) / settings.getHeight(), 0.1f, 10000.0f);
    
    // Use player-relative coordinate system for edge rendering
    float playerHeight = player.getHeight();
    glm::vec3 eyePos = glm::vec3(0, playerHeight, 0); // Player is at origin in player-relative space
    glm::vec3 lookAtPos = eyePos + player.cameraDirection;
    
    // Use corrected up vector for edge rendering too
    glm::vec3 viewDir = glm::normalize(lookAtPos - eyePos);
    glm::vec3 rightDir = glm::normalize(glm::cross(viewDir, player.up));
    glm::vec3 upDir = glm::normalize(glm::cross(rightDir, viewDir));
    
    glm::mat4 view = glm::lookAt(eyePos, lookAtPos, upDir);
    glUniformMatrix4fv(glGetUniformLocation(edgeShaderProgram, "proj"), 1, GL_FALSE, &proj[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(edgeShaderProgram, "view"), 1, GL_FALSE, &view[0][0]);

    // Get player world position for relative positioning
    glm::vec3 playerWorldPos = player.position;
    glUniform3fv(glGetUniformLocation(edgeShaderProgram, "playerOffset"), 1, &playerWorldPos[0]);

    std::vector<float> edgeVertices;
    float radius = 100.0f; // Distance limit for edge rendering
    
    for (const auto& [key, chunkPtr] : world.getChunks()) {
        // FIXED: Use the pointer directly, not the dereferenced object
        int chunkX = std::get<0>(key);
        int chunkY = std::get<1>(key);
        int chunkZ = std::get<2>(key);
        
        // Calculate chunk center position relative to player
        glm::vec3 chunkCenter(
            chunkX * Chunk::SIZE + Chunk::SIZE / 2.0f - playerWorldPos.x,
            chunkY * Chunk::SIZE + Chunk::SIZE / 2.0f - playerWorldPos.y,
            chunkZ * Chunk::SIZE + Chunk::SIZE / 2.0f - playerWorldPos.z
        );
            
        // Distance culling for edges
        float dist = glm::length(chunkCenter);
        if (dist > radius) continue;

        // Get chunk mesh data - FIXED: use correct pointer access
        const std::vector<float>& mesh = chunkPtr->getMesh();
        
        // Not enough vertices to form edges
        if (mesh.size() < 20) continue;
        
        // Process each quad in the mesh (4 vertices per quad)
        for (size_t i = 0; i < mesh.size(); i += 20) {
            if (i + 19 >= mesh.size()) break; // Safety check
            
            // Extract the quad vertices and transform to player-relative space
            float x1 = mesh[i]     + chunkX * Chunk::SIZE - playerWorldPos.x;
            float y1 = mesh[i + 1] + chunkY * Chunk::SIZE - playerWorldPos.y;
            float z1 = mesh[i + 2] + chunkZ * Chunk::SIZE - playerWorldPos.z;
            
            float x2 = mesh[i + 5] + chunkX * Chunk::SIZE - playerWorldPos.x;
            float y2 = mesh[i + 6] + chunkY * Chunk::SIZE - playerWorldPos.y;
            float z2 = mesh[i + 7] + chunkZ * Chunk::SIZE - playerWorldPos.z;
            
            float x3 = mesh[i + 10] + chunkX * Chunk::SIZE - playerWorldPos.x;
            float y3 = mesh[i + 11] + chunkY * Chunk::SIZE - playerWorldPos.y;
            float z3 = mesh[i + 12] + chunkZ * Chunk::SIZE - playerWorldPos.z;
            
            float x4 = mesh[i + 15] + chunkX * Chunk::SIZE - playerWorldPos.x;
            float y4 = mesh[i + 16] + chunkY * Chunk::SIZE - playerWorldPos.y;
            float z4 = mesh[i + 17] + chunkZ * Chunk::SIZE - playerWorldPos.z;

            // Add the edges to the vertex buffer
            edgeVertices.insert(edgeVertices.end(), {x1, y1, z1, x2, y2, z2});
            edgeVertices.insert(edgeVertices.end(), {x2, y2, z2, x3, y3, z3});
            edgeVertices.insert(edgeVertices.end(), {x3, y3, z3, x4, y4, z4});
            edgeVertices.insert(edgeVertices.end(), {x4, y4, z4, x1, y1, z1});
        }
    }

    // Only proceed if we have edges to render
    if (!edgeVertices.empty()) {
        // Load edge vertices into GPU
        glBindBuffer(GL_ARRAY_BUFFER, edgeVbo);
        glBufferData(GL_ARRAY_BUFFER, edgeVertices.size() * sizeof(float), edgeVertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Draw edges
        glLineWidth(2.0f);
        glDrawArrays(GL_LINES, 0, edgeVertices.size() / 3);
        checkGLError("post-edge-draw");
    }
}

void Renderer::loadShader() {
    // CRITICAL FIX: Updated vertex shader with relative positioning for Earth-scale coordinates
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
            // The model matrix has already transformed the position to player-relative space
            // Apply view and projection transformations
            gl_Position = proj * view * model * vec4(pos, 1.0);
            
            // Pass texture coordinates unchanged
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
                float faceId = floor(TexCoord.x + 0.5);
                if (faceId == 0.0) FragColor = vec4(1.0, 1.0, 1.0, 1.0);
                else if (faceId == 1.0) FragColor = vec4(0.0, 0.0, 0.0, 1.0);
                else if (faceId == 2.0) FragColor = vec4(1.0, 0.0, 0.0, 1.0);
                else if (faceId == 3.0) FragColor = vec4(0.0, 1.0, 0.0, 1.0);
                else if (faceId == 4.0) FragColor = vec4(0.5, 0.0, 0.5, 1.0);
                else if (faceId == 5.0) FragColor = vec4(1.0, 1.0, 0.0, 1.0);
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
        std::cerr << "Vertex Shader Error: " << infoLog << std::endl;
    }
    
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragSrc, NULL);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(frag, 512, NULL, infoLog);
        std::cerr << "Fragment Shader Error: " << infoLog << std::endl;
    }
    
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vert);
    glAttachShader(shaderProgram, frag);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader Program Link Error: " << infoLog << std::endl;
    }
    
    glDeleteShader(vert);
    glDeleteShader(frag);
}

void Renderer::loadEdgeShader() {
    // Updated edge shader with player-relative positioning
    const char* vertSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 pos;
        uniform mat4 view, proj;
        uniform vec3 playerOffset;
        
        void main() {
            // Vertex positions are already relative to player
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
        std::cerr << "Edge Vertex Shader Error: " << infoLog << std::endl;
    }
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragSrc, NULL);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(frag, 512, NULL, infoLog);
        std::cerr << "Edge Fragment Shader Error: " << infoLog << std::endl;
    }
    edgeShaderProgram = glCreateProgram();
    glAttachShader(edgeShaderProgram, vert);
    glAttachShader(edgeShaderProgram, frag);
    glLinkProgram(edgeShaderProgram);
    glGetProgramiv(edgeShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
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
        
        // Create a fallback texture (checkerboard pattern) with grass and dirt colors
        const int checkerSize = 64;
        const int textureSize = 256;
        unsigned char checkerboard[textureSize * textureSize * 4];
        
        for (int y = 0; y < textureSize; y++) {
            for (int x = 0; x < textureSize; x++) {
                int idx = (y * textureSize + x) * 4;
                bool isGreen = ((x / checkerSize) + (y / checkerSize)) % 2 == 0;
                
                if (isGreen) {
                    // Green (for grass)
                    checkerboard[idx + 0] = 34;     // R
                    checkerboard[idx + 1] = 139;    // G
                    checkerboard[idx + 2] = 34;     // B
                } else {
                    // Brown (for dirt)
                    checkerboard[idx + 0] = 139;    // R
                    checkerboard[idx + 1] = 69;     // G
                    checkerboard[idx + 2] = 19;     // B
                }
                checkerboard[idx + 3] = 255;  // A (fully opaque)
            }
        }
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureSize, textureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, checkerboard);
        std::cout << "Created fallback texture: " << textureSize << "x" << textureSize << std::endl;
    }
    
    stbi_image_free(data);
}