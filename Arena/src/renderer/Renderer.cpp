#include "renderer/Renderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <iostream>

// Vertex shader source
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    uniform mat4 model;
    uniform mat4 viewProjection;

    out vec2 TexCoord;

    void main() {
        gl_Position = viewProjection * model * vec4(aPos, 1.0);
        TexCoord = aTexCoord;
    }
)";

// Fragment shader source
const char* fragmentShaderSource = R"(
    #version 330 core
    in vec2 TexCoord;

    uniform sampler2D textureSampler;

    out vec4 FragColor;

    void main() {
        FragColor = texture(textureSampler, TexCoord);
    }
)";

Renderer::Renderer()
    : m_shaderProgram(0)
    , m_vao(0)
    , m_vbo(0)
    , m_ebo(0)
{
}

Renderer::~Renderer() {
    if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
}

bool Renderer::initialize() {
    m_textureManager = std::make_unique<TextureManager>();
    if (!m_textureManager->initialize()) {
        return false;
    }

    setupShaders();
    setupBuffers();

    // Get shader uniform locations
    m_modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    m_viewProjectionLoc = glGetUniformLocation(m_shaderProgram, "viewProjection");
    m_textureLoc = glGetUniformLocation(m_shaderProgram, "textureSampler");

    return true;
}

void Renderer::setupShaders() {
    // Create vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Check vertex shader compilation
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        // Handle error
        glDeleteShader(vertexShader);
        return;
    }

    // Create fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Check fragment shader compilation
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        // Handle error
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return;
    }

    // Create shader program
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);

    // Check program linking
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_shaderProgram, 512, NULL, infoLog);
        // Handle error
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
        return;
    }

    // Cleanup
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Renderer::setupBuffers() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
}

void Renderer::render(World* world, Player* player) {
    setupCamera(player);
    
    // Render world blocks
    glPushMatrix();
    renderWorld(world, player);
    glPopMatrix();
    
    // Render debug collision box
    renderPlayerCollisionBox(player);
}

void Renderer::renderWorld(World* world, Player* player) {
    if (!world || !player) return;

    glUseProgram(m_shaderProgram);

    // Set up view and projection matrices
    glm::mat4 projection = glm::perspective(glm::radians(70.0f), 800.0f / 600.0f, 0.25f, 1000.0f);
    
    // Apply the same camera height offset here
    glm::vec3 cameraPos = player->getPosition();
    float cameraHeight = 1.5f; // Reduced from 10.0f to 1.5 voxels high
    cameraPos.y += cameraHeight;
    
    std::cout << "RENDER_WORLD DEBUG: Camera at position (" << cameraPos.x << ", " 
              << cameraPos.y << ", " << cameraPos.z << ")" << std::endl;
    
    glm::mat4 view = glm::lookAt(
        cameraPos,
        cameraPos + player->getForward(),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    glm::mat4 viewProjection = projection * view;

    // Set texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureManager->getTextureID(0)); // Assuming 0 is the first texture
    glUniform1i(m_textureLoc, 0);

    // Set view projection matrix
    glUniformMatrix4fv(m_viewProjectionLoc, 1, GL_FALSE, glm::value_ptr(viewProjection));

    // Render all chunks
    for (const auto& pair : world->getChunks()) {
        const Chunk* chunk = pair.second.get();
        if (chunk) {
            renderChunk(chunk, viewProjection);
        }
    }
}

void Renderer::renderChunk(const Chunk* chunk, const glm::mat4& viewProjection) {
    if (!chunk) return;

    // Get chunk mesh data
    const std::vector<float>& vertices = chunk->getMeshVertices();
    const std::vector<unsigned int>& indices = chunk->getMeshIndices();

    if (vertices.empty() || indices.empty()) return;

    // Calculate model matrix
    glm::vec2 pos = chunk->getPosition();
    glm::mat4 model = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(pos.x * Chunk::CHUNK_SIZE, 0.0f, pos.y * Chunk::CHUNK_SIZE)
    );

    // Set model matrix
    glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Bind VAO and upload mesh data
    glBindVertexArray(m_vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);

    // Set vertex attributes
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Draw the chunk
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

    // Cleanup
    glBindVertexArray(0);
}

void Renderer::renderPlayerCollisionBox(Player* player) {
    // Only render in debug mode
    static bool debugCollisionBox = true;
    
    if (!debugCollisionBox || !player) return;
    
    // Get player collision bounds
    glm::vec3 min = player->getMinBounds();
    glm::vec3 max = player->getMaxBounds();
    
    // Debug output collision box dimensions occasionally
    static int boxDebugCounter = 0;
    if (boxDebugCounter++ % 300 == 0) { // Output every ~5 seconds at 60fps
        std::cout << "Collision box dimensions:" << std::endl;
        std::cout << "  Min: (" << min.x << ", " << min.y << ", " << min.z << ")" << std::endl;
        std::cout << "  Max: (" << max.x << ", " << max.y << ", " << max.z << ")" << std::endl;
        std::cout << "  Width: " << (max.x - min.x) << std::endl;
        std::cout << "  Height: " << (max.y - min.y) << std::endl;
        std::cout << "  Depth: " << (max.z - min.z) << std::endl;
    }
    
    // Save current OpenGL state
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    
    // Disable texturing and lighting for wireframe rendering
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST); // Draw on top
    
    glPushMatrix();
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw filled box with low alpha
    glColor4f(1.0f, 0.0f, 0.0f, 0.15f); // Very transparent red
    glBegin(GL_QUADS);
    
    // Front face
    glVertex3f(min.x, min.y, min.z);
    glVertex3f(max.x, min.y, min.z);
    glVertex3f(max.x, max.y, min.z);
    glVertex3f(min.x, max.y, min.z);
    
    // Back face
    glVertex3f(min.x, min.y, max.z);
    glVertex3f(max.x, min.y, max.z);
    glVertex3f(max.x, max.y, max.z);
    glVertex3f(min.x, max.y, max.z);
    
    // Left face
    glVertex3f(min.x, min.y, min.z);
    glVertex3f(min.x, min.y, max.z);
    glVertex3f(min.x, max.y, max.z);
    glVertex3f(min.x, max.y, min.z);
    
    // Right face
    glVertex3f(max.x, min.y, min.z);
    glVertex3f(max.x, min.y, max.z);
    glVertex3f(max.x, max.y, max.z);
    glVertex3f(max.x, max.y, min.z);
    
    // Top face
    glVertex3f(min.x, max.y, min.z);
    glVertex3f(max.x, max.y, min.z);
    glVertex3f(max.x, max.y, max.z);
    glVertex3f(min.x, max.y, max.z);
    
    // Bottom face
    glVertex3f(min.x, min.y, min.z);
    glVertex3f(max.x, min.y, min.z);
    glVertex3f(max.x, min.y, max.z);
    glVertex3f(min.x, min.y, max.z);
    
    glEnd();
    
    // Draw wireframe box with higher opacity
    glColor4f(1.0f, 0.0f, 0.0f, 0.7f); // More opaque red
    glLineWidth(3.0f); // Thicker lines
    
    // Draw the wireframe box
    glBegin(GL_LINES);
    
    // Bottom square
    glVertex3f(min.x, min.y, min.z);
    glVertex3f(max.x, min.y, min.z);
    
    glVertex3f(max.x, min.y, min.z);
    glVertex3f(max.x, min.y, max.z);
    
    glVertex3f(max.x, min.y, max.z);
    glVertex3f(min.x, min.y, max.z);
    
    glVertex3f(min.x, min.y, max.z);
    glVertex3f(min.x, min.y, min.z);
    
    // Top square
    glVertex3f(min.x, max.y, min.z);
    glVertex3f(max.x, max.y, min.z);
    
    glVertex3f(max.x, max.y, min.z);
    glVertex3f(max.x, max.y, max.z);
    
    glVertex3f(max.x, max.y, max.z);
    glVertex3f(min.x, max.y, max.z);
    
    glVertex3f(min.x, max.y, max.z);
    glVertex3f(min.x, max.y, min.z);
    
    // Vertical lines
    glVertex3f(min.x, min.y, min.z);
    glVertex3f(min.x, max.y, min.z);
    
    glVertex3f(max.x, min.y, min.z);
    glVertex3f(max.x, max.y, min.z);
    
    glVertex3f(max.x, min.y, max.z);
    glVertex3f(max.x, max.y, max.z);
    
    glVertex3f(min.x, min.y, max.z);
    glVertex3f(min.x, max.y, max.z);
    
    glEnd();
    
    // Highlight ground check points in a different color
    glColor4f(0.0f, 1.0f, 0.0f, 1.0f); // Bright green for ground check points
    glPointSize(8.0f);
    glBegin(GL_POINTS);
    
    // Draw points at bottom corners to show ground check positions
    glVertex3f(min.x, min.y, min.z);
    glVertex3f(max.x, min.y, min.z);
    glVertex3f(max.x, min.y, max.z);
    glVertex3f(min.x, min.y, max.z);
    
    // Draw a point at player's exact position
    glColor4f(0.0f, 0.0f, 1.0f, 1.0f); // Blue for player position
    glm::vec3 playerPos = player->getPosition();
    glVertex3f(playerPos.x, playerPos.y, playerPos.z);
    
    glEnd();
    
    // Draw block grid for reference
    glColor4f(0.7f, 0.7f, 0.7f, 0.3f); // Light gray for grid
    glLineWidth(1.0f);
    
    // Calculate grid boundaries around player
    int gridSize = 5; // Grid cells in each direction
    int playerX = static_cast<int>(std::floor(playerPos.x));
    int playerY = static_cast<int>(std::floor(playerPos.y));
    int playerZ = static_cast<int>(std::floor(playerPos.z));
    
    glBegin(GL_LINES);
    
    // Horizontal grid (XZ plane)
    for (int x = playerX - gridSize; x <= playerX + gridSize; x++) {
        glVertex3f(static_cast<float>(x), static_cast<float>(playerY), static_cast<float>(playerZ - gridSize));
        glVertex3f(static_cast<float>(x), static_cast<float>(playerY), static_cast<float>(playerZ + gridSize));
    }
    
    for (int z = playerZ - gridSize; z <= playerZ + gridSize; z++) {
        glVertex3f(static_cast<float>(playerX - gridSize), static_cast<float>(playerY), static_cast<float>(z));
        glVertex3f(static_cast<float>(playerX + gridSize), static_cast<float>(playerY), static_cast<float>(z));
    }
    
    glEnd();
    
    // Restore OpenGL state
    glPopMatrix();
    glPopAttrib();
}

void Renderer::setupCamera(const Player* player) {
    if (!player) return;
    
    // Set up the projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // Calculate perspective matrix using GLM instead of gluPerspective
    float fov = 70.0f; // Field of view in degrees
    float aspectRatio = 800.0f / 600.0f;
    float nearPlane = 0.25f; // Increased from 0.1f to prevent clipping
    float farPlane = 1000.0f;
    
    // Convert FOV to radians
    float fovRadians = fov * 3.14159f / 180.0f;
    
    // Calculate perspective matrix values
    float tanHalfFovy = tan(fovRadians / 2.0f);
    float focalLength = 1.0f / tanHalfFovy;
    float A = (farPlane + nearPlane) / (nearPlane - farPlane);
    float B = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    
    // Load perspective matrix
    float perspMatrix[16] = {
        focalLength / aspectRatio, 0.0f, 0.0f, 0.0f,
        0.0f, focalLength, 0.0f, 0.0f,
        0.0f, 0.0f, A, -1.0f,
        0.0f, 0.0f, B, 0.0f
    };
    glMultMatrixf(perspMatrix);
    
    // Set up the modelview matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Calculate camera vectors
    glm::vec3 position = player->getPosition();
    glm::vec3 forward = player->getForward();
    glm::vec3 up = player->getUp();
    glm::vec3 right = player->getRight();
    
    // Set camera at eye level (1.5 blocks high)
    float cameraHeight = 1.5f;
    position.y += cameraHeight;
    
    // Debug print to verify camera height is being applied
    static int cameraDebugCounter = 0;
    if (cameraDebugCounter++ % 60 == 0) {
        std::cout << "CAMERA HEIGHT DEBUG: Setting camera at height " << cameraHeight 
                  << " voxels above player position: " << position.y << std::endl;
    }
    
    // Create lookAt matrix using GLM vectors instead of gluLookAt
    glm::vec3 fwd = glm::normalize(forward);
    glm::vec3 side = glm::normalize(glm::cross(fwd, up));
    glm::vec3 upVector = glm::cross(side, fwd);
    
    float lookAtMatrix[16] = {
        side.x, upVector.x, -fwd.x, 0.0f,
        side.y, upVector.y, -fwd.y, 0.0f,
        side.z, upVector.z, -fwd.z, 0.0f,
        0.0f,   0.0f,       0.0f,   1.0f
    };
    glMultMatrixf(lookAtMatrix);
    
    // Translate to camera position (with negated coordinates for OpenGL view matrix)
    glTranslatef(-position.x, -position.y, -position.z);
} 