#include "renderer/Renderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <iostream>
#include <GLFW/glfw3.h>

// Vertex shader source with normals support
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    uniform mat4 model;
    uniform mat4 viewProjection;

    out vec2 TexCoord;
    out vec3 Normal;
    out vec3 FragPos;

    void main() {
        gl_Position = viewProjection * model * vec4(aPos, 1.0);
        TexCoord = aTexCoord;
        
        // Calculate normal based on face orientation - always pointing outward
        vec3 faceNormals[6] = vec3[6](
            vec3(0.0, 0.0, 1.0),   // Front (+Z)
            vec3(0.0, 0.0, -1.0),  // Back (-Z)
            vec3(-1.0, 0.0, 0.0),  // Left (-X)
            vec3(1.0, 0.0, 0.0),   // Right (+X)
            vec3(0.0, 1.0, 0.0),   // Top (+Y)
            vec3(0.0, -1.0, 0.0)   // Bottom (-Y)
        );
        
        // Determine face from texture coordinates
        // This uses the texture atlas layout to identify the face type
        int faceIndex = 0;
        if (TexCoord.x >= 0.667 && TexCoord.y >= 0.5) faceIndex = 0;      // Purple - Front
        else if (TexCoord.x >= 0.667 && TexCoord.y < 0.5) faceIndex = 1;  // Yellow - Back
        else if (TexCoord.x >= 0.333 && TexCoord.x < 0.667 && TexCoord.y < 0.5) faceIndex = 2;  // Green - Left
        else if (TexCoord.x < 0.333 && TexCoord.y >= 0.5) faceIndex = 3;  // Black - Right
        else if (TexCoord.x >= 0.333 && TexCoord.x < 0.667 && TexCoord.y >= 0.5) faceIndex = 4; // Red - Top
        else if (TexCoord.x < 0.333 && TexCoord.y < 0.5) faceIndex = 5;   // White - Bottom
        
        Normal = faceNormals[faceIndex];
        FragPos = vec3(model * vec4(aPos, 1.0));
    }
)";

// Fragment shader source with basic lighting
const char* fragmentShaderSource = R"(
    #version 330 core
    in vec2 TexCoord;
    in vec3 Normal;
    in vec3 FragPos;

    uniform sampler2D textureSampler;

    out vec4 FragColor;

    void main() {
        // Simple directional light from above-right-front diagonal
        vec3 lightDir = normalize(vec3(0.5, 0.7, 0.5));
        
        // Calculate diffuse lighting with normalized normal
        vec3 norm = normalize(Normal);
        float diff = max(dot(norm, lightDir), 0.0);
        
        // Add ambient light to ensure nothing is completely dark
        float ambient = 0.4;
        
        // Final lighting factor with stronger diffuse component
        float lighting = ambient + diff * 0.8;
        
        // Get base color from texture
        vec4 texColor = texture(textureSampler, TexCoord);
        
        // Apply lighting
        FragColor = vec4(texColor.rgb * lighting, texColor.a);
    }
)";

Renderer::Renderer()
    : m_shaderProgram(0)
    , m_vao(0)
    , m_vbo(0)
    , m_ebo(0)
    , m_buffersInitialized(false)
    , m_disableBackfaceCulling(false)
    , m_disableGreedyMeshing(false)
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
    
    // Configure OpenGL settings for rendering
    // Note: Backface culling is disabled as requested
    
    // Enable depth testing for proper 3D rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Set initial buffer state flag
    m_buffersInitialized = (m_vao != 0 && m_vbo != 0 && m_ebo != 0);
    std::cout << "Renderer initialization complete. Buffers initialized: " 
              << (m_buffersInitialized ? "YES" : "NO") << std::endl;

    return true;
}

void Renderer::setupShaders() {
    // Create vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (vertexShader == 0) {
        std::cerr << "ERROR: Failed to create vertex shader" << std::endl;
        return;
    }
    
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Check vertex shader compilation
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return;
    }
    std::cout << "Vertex shader compiled successfully" << std::endl;

    // Create fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (fragmentShader == 0) {
        std::cerr << "ERROR: Failed to create fragment shader" << std::endl;
        glDeleteShader(vertexShader);
        return;
    }
    
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Check fragment shader compilation
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return;
    }
    std::cout << "Fragment shader compiled successfully" << std::endl;

    // Create shader program
    m_shaderProgram = glCreateProgram();
    if (m_shaderProgram == 0) {
        std::cerr << "ERROR: Failed to create shader program" << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return;
    }
    
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);

    // Check program linking
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
        return;
    }
    std::cout << "Shader program linked successfully" << std::endl;

    // Cleanup
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Renderer::setupBuffers() {
    // Generate VAO
    glGenVertexArrays(1, &m_vao);
    if (m_vao == 0) {
        std::cerr << "ERROR: Failed to create VAO" << std::endl;
        return;
    }
    std::cout << "VAO created successfully: " << m_vao << std::endl;

    // Generate VBO and EBO
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
    if (m_vbo == 0 || m_ebo == 0) {
        std::cerr << "ERROR: Failed to create buffers" << std::endl;
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
        return;
    }
    std::cout << "VBO created successfully: " << m_vbo << std::endl;
    std::cout << "EBO created successfully: " << m_ebo << std::endl;

    // Bind VAO first
    glBindVertexArray(m_vao);

    // Allocate memory for vertex buffer (16MB initial size)
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    const GLsizeiptr vboSize = 16 * 1024 * 1024 * sizeof(float);
    glBufferData(GL_ARRAY_BUFFER, vboSize, nullptr, GL_DYNAMIC_DRAW);
    
    // Check if allocation was successful
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "ERROR: Failed to allocate vertex buffer. Error code: " << err << std::endl;
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);
        glDeleteVertexArrays(1, &m_vao);
        m_vao = m_vbo = m_ebo = 0;
        return;
    }
    std::cout << "Vertex buffer allocated with " << (vboSize / 1024 / 1024) << "MB initial size" << std::endl;

    // Allocate memory for index buffer (4MB initial size)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    const GLsizeiptr eboSize = 4 * 1024 * 1024 * sizeof(unsigned int);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, eboSize, nullptr, GL_DYNAMIC_DRAW);
    
    // Check if allocation was successful
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "ERROR: Failed to allocate index buffer. Error code: " << err << std::endl;
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);
        glDeleteVertexArrays(1, &m_vao);
        m_vao = m_vbo = m_ebo = 0;
        return;
    }
    std::cout << "Index buffer allocated with " << (eboSize / 1024 / 1024) << "MB initial size" << std::endl;

    // Set up vertex attributes
    // Position attribute (3 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    std::cout << "Position attribute set up (3 floats at offset 0)" << std::endl;
    
    // Texture coordinate attribute (2 floats)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    std::cout << "Texture coordinate attribute set up (2 floats at offset 12)" << std::endl;

    // Check for OpenGL errors
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error in vertex attribute setup: " << err << std::endl;
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);
        glDeleteVertexArrays(1, &m_vao);
        m_vao = m_vbo = m_ebo = 0;
        return;
    }

    // Unbind buffers
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Set buffer initialization flag
    m_buffersInitialized = true;
    std::cout << "Buffer setup completed successfully. m_buffersInitialized = " << m_buffersInitialized << std::endl;
}

void Renderer::render(World* world, Player* player) {
    // Track if we need to restore GPU state between renders
    bool stateModified = false;
    
    // Get current window dimensions
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    
    // 1. First render the 3D game world
    if (world && m_buffersInitialized) {
        stateModified = true;
        glEnable(GL_DEPTH_TEST);
        // Removed backface culling as requested
        glUseProgram(m_shaderProgram);
        setupCamera(player);
        renderWorld(world, player);
        glUseProgram(0);
        renderPlayerCollisionBox(player);
    } else if (!m_buffersInitialized) {
        std::cerr << "ERROR: Cannot render world - buffers not initialized" << std::endl;
    }

    // 2. Switch to legacy OpenGL for UI rendering
    stateModified = true;
    glDisable(GL_DEPTH_TEST);
    // No need to disable culling since we're not enabling it
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set up orthographic projection for UI
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    // Use top-left as origin (0,0) for consistency with all UI elements
    std::cout << "Renderer: Setting up orthographic projection with dimensions " 
              << width << "x" << height << " and top-left origin (0,0)" << std::endl;
    glOrtho(0, width, height, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Crosshair
    float crosshairSize = 10.0f;
    float centerX = width / 2.0f;
    float centerY = height / 2.0f;

    glLineWidth(2.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
    glBegin(GL_LINES);
    glVertex2f(centerX - crosshairSize, centerY);
    glVertex2f(centerX + crosshairSize, centerY);
    glVertex2f(centerX, centerY - crosshairSize);
    glVertex2f(centerX, centerY + crosshairSize);
    glEnd();

    // Unbind any VAO before calling debug menu render
    glBindVertexArray(0);
    
    // Debug menu
    if (m_debugMenu && m_debugMenu->isActive()) {
        m_debugMenu->render();
        
        // Check if buffers are still valid after debug menu render
        if (m_vao == 0 || m_vbo == 0 || m_ebo == 0) {
            std::cerr << "WARNING: OpenGL buffers were invalidated during debug menu rendering!" << std::endl;
            m_buffersInitialized = false;
            
            // Attempt to restore buffers immediately
            setupBuffers();
        }
    }

    // Splash screen
    if (m_splashScreen && m_splashScreen->isActive()) {
        m_splashScreen->render();
    }

    // Restore projection/modelview matrices
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    // Final check to ensure buffers are still valid after rendering
    if ((m_vao == 0 || m_vbo == 0 || m_ebo == 0) && m_buffersInitialized) {
        std::cerr << "WARNING: OpenGL buffers were invalidated during frame rendering!" << std::endl;
        m_buffersInitialized = false;
    }
}

void Renderer::renderWorld(World* world, Player* player) {
    if (!world || !player) return;

    glUseProgram(m_shaderProgram);

    // Get current window dimensions
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    float aspectRatio = static_cast<float>(width) / height;

    // Set up view and projection matrices
    glm::mat4 projection = glm::perspective(glm::radians(70.0f), aspectRatio, 0.25f, 1000.0f);
    
    // Apply the same camera height offset here
    glm::vec3 cameraPos = player->getPosition();
    float cameraHeight = 1.5f; // Reduced from 10.0f to 1.5 voxels high
    cameraPos.y += cameraHeight;
    
    // Only print camera position debug occasionally
    static int cameraDebugCounter = 0;
    if (cameraDebugCounter++ % 300 == 0) { // Every 5 seconds at 60fps
        std::cout << "RENDER_WORLD DEBUG: Camera at position (" << cameraPos.x << ", " 
                << cameraPos.y << ", " << cameraPos.z << ")" << std::endl;
    }
    
    glm::mat4 view = glm::lookAt(
        cameraPos,
        cameraPos + player->getForward(),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    glm::mat4 viewProjection = projection * view;

    // Set up OpenGL state for 3D rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Apply backface culling based on debug setting
    if (!m_disableBackfaceCulling) {
        // Enable backface culling with correct winding order
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CW); // Clockwise for front faces to match our index order
    } else {
        // Disable backface culling if requested
        glDisable(GL_CULL_FACE);
    }
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Set texture for all chunks
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureManager->getTextureID(0));
    glUniform1i(m_textureLoc, 0);

    // Set view projection matrix
    glUniformMatrix4fv(m_viewProjectionLoc, 1, GL_FALSE, glm::value_ptr(viewProjection));

    // Calculate frustum planes for frustum culling
    glm::vec3 forward = player->getForward();
    glm::vec3 right = player->getRight();
    glm::vec3 up = player->getUp();
    float renderDistance = 16 * Chunk::CHUNK_SIZE; // 16 chunks of render distance
    
    // Get player chunk position
    glm::ivec3 playerChunkPos(
        static_cast<int>(std::floor(player->getPosition().x / Chunk::CHUNK_SIZE)),
        static_cast<int>(std::floor(player->getPosition().y / Chunk::CHUNK_HEIGHT)),
        static_cast<int>(std::floor(player->getPosition().z / Chunk::CHUNK_SIZE))
    );
    
    // Render all chunks, sorted by distance to player for better rendering priority
    std::vector<std::pair<float, const Chunk*>> sortedChunks;
    int visibleChunks = 0;
    
    for (const auto& pair : world->getChunks()) {
        const Chunk* chunk = pair.second.get();
        if (!chunk) continue;
        
        // Get chunk position
        glm::ivec3 chunkPos = chunk->getPosition();
        
        // Calculate chunk center in world space
        glm::vec3 chunkCenter = glm::vec3(
            (chunkPos.x * Chunk::CHUNK_SIZE) + (Chunk::CHUNK_SIZE / 2.0f),
            (chunkPos.y * Chunk::CHUNK_HEIGHT) + (Chunk::CHUNK_HEIGHT / 2.0f),
            (chunkPos.z * Chunk::CHUNK_SIZE) + (Chunk::CHUNK_SIZE / 2.0f)
        );
        
        // Calculate distance to chunk
        float distToChunk = glm::length(chunkCenter - cameraPos);
        
        // Skip chunks that are too far away
        if (distToChunk > renderDistance) {
            continue;
        }
        
        // Simple frustum culling
        // Calculate vector from camera to chunk center
        glm::vec3 toCenterVec = glm::normalize(chunkCenter - cameraPos);
        
        // If the dot product is negative, the chunk is behind the camera
        if (glm::dot(forward, toCenterVec) < 0.0f && distToChunk > Chunk::CHUNK_SIZE * 1.5f) {
            continue;
        }
        
        // Check if the chunk has any mesh data to render
        if (chunk->getMeshVertices().empty() || chunk->getMeshIndices().empty()) {
            continue;
        }
        
        // Add to sorted list
        sortedChunks.push_back(std::make_pair(distToChunk, chunk));
        visibleChunks++;
    }
    
    // Sort chunks by distance (closest first)
    std::sort(sortedChunks.begin(), sortedChunks.end(),
        [](const std::pair<float, const Chunk*>& a, const std::pair<float, const Chunk*>& b) {
            return a.first < b.first;
        }
    );
    
    // Print debug info occasionally
    static int renderDebugCounter = 0;
    if (renderDebugCounter++ % 300 == 0) { // Every 5 seconds at 60fps
        std::cout << "Rendering " << visibleChunks << " visible chunks of " 
                  << world->getChunks().size() << " total chunks" << std::endl;
        std::cout << "Backface culling: " << (m_disableBackfaceCulling ? "DISABLED" : "ENABLED") << std::endl;
        std::cout << "Greedy meshing: " << (m_disableGreedyMeshing ? "DISABLED" : "ENABLED") << std::endl;
    }
    
    // Render sorted chunks
    for (const auto& pair : sortedChunks) {
        renderChunk(pair.second, viewProjection);
    }

    // Cleanup
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void Renderer::renderChunk(const Chunk* chunk, const glm::mat4& viewProjection) {
    if (!chunk) return;

    // Check if OpenGL objects are properly initialized
    if (m_vao == 0 || m_vbo == 0 || m_ebo == 0) {
        std::cerr << "ERROR: OpenGL buffers not initialized" << std::endl;
        m_buffersInitialized = false;
        return;
    }

    if (m_shaderProgram == 0) {
        std::cerr << "ERROR: Shader program not initialized" << std::endl;
        return;
    }

    if (!m_textureManager) {
        std::cerr << "ERROR: Texture manager not initialized" << std::endl;
        return;
    }

    // Get chunk mesh data
    const std::vector<float>& vertices = chunk->getMeshVertices();
    const std::vector<unsigned int>& indices = chunk->getMeshIndices();

    if (vertices.empty() || indices.empty()) return;

    // Check buffer sizes
    const GLsizeiptr vertexDataSize = vertices.size() * sizeof(float);
    const GLsizeiptr indexDataSize = indices.size() * sizeof(unsigned int);
    
    // Get current buffer sizes
    GLint vboSize = 0;
    GLint eboSize = 0;
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &vboSize);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &eboSize);

    // Check for buffer overflow
    if (vertexDataSize > vboSize) {
        std::cerr << "ERROR: Vertex buffer overflow. Required: " << vertexDataSize 
                  << " bytes, Available: " << vboSize << " bytes" << std::endl;
        return;
    }

    if (indexDataSize > eboSize) {
        std::cerr << "ERROR: Index buffer overflow. Required: " << indexDataSize 
                  << " bytes, Available: " << eboSize << " bytes" << std::endl;
        return;
    }

    // Debug: Print number of vertices and indices being rendered
    static int debugCounter = 0;
    if (debugCounter++ % 600 == 0) {
        std::cout << "DEBUG: Rendering chunk with " << vertices.size() / 5 << " vertices and " 
                  << indices.size() << " indices (" << indices.size()/3 << " triangles)" << std::endl;
        std::cout << "Chunk position: (" << chunk->getPosition().x << ", " 
                  << chunk->getPosition().y << ", " << chunk->getPosition().z << ")" << std::endl;
        std::cout << "Vertex data size: " << vertexDataSize << " bytes" << std::endl;
        std::cout << "Index data size: " << indexDataSize << " bytes" << std::endl;
    }

    // Use shader program
    glUseProgram(m_shaderProgram);

    // Set model matrix based on chunk position
    glm::mat4 model = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(
            chunk->getPosition().x * Chunk::CHUNK_SIZE,
            chunk->getPosition().y * Chunk::CHUNK_HEIGHT,
            chunk->getPosition().z * Chunk::CHUNK_SIZE
        )
    );

    // Set uniforms
    glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(m_viewProjectionLoc, 1, GL_FALSE, glm::value_ptr(viewProjection));

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureManager->getTextureID(0));
    glUniform1i(m_textureLoc, 0);

    // Bind VAO
    glBindVertexArray(m_vao);

    // Update vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexDataSize, vertices.data());

    // Check for errors after vertex buffer update
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "ERROR: Failed to update vertex buffer. Error code: " << err << std::endl;
        return;
    }

    // Update index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexDataSize, indices.data());

    // Check for errors after index buffer update
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "ERROR: Failed to update index buffer. Error code: " << err << std::endl;
        return;
    }

    // Draw the chunk
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

    // Check for errors after drawing
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "ERROR: Failed to draw chunk. Error code: " << err << std::endl;
    }

    // Cleanup
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::renderPlayerCollisionBox(Player* player) {
    if (!player) return;
    
    // Save current OpenGL state
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    
    // Set up rendering state for collision box
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Get player collision bounds
    glm::vec3 min = player->getMinBounds();
    glm::vec3 max = player->getMaxBounds();
    
    // Draw wireframe box
    glColor4f(1.0f, 0.0f, 0.0f, 0.7f);
    glLineWidth(2.0f);
    
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
    
    // Restore previous state
    if (depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    }
    glDisable(GL_BLEND);
}

void Renderer::setupCamera(const Player* player) {
    if (!player) return;
    
    // Set up the projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // Get current window dimensions
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    float aspectRatio = static_cast<float>(width) / height;
    
    // Calculate perspective matrix using GLM
    float fov = 70.0f; // Field of view in degrees
    float nearPlane = 0.25f;
    float farPlane = 1000.0f;
    
    // Convert FOV to radians
    float fovRadians = fov * 3.14159f / 180.0f;
    
    // Calculate perspective matrix values
    float tanHalfFovy = tan(fovRadians / 2.0f);
    float focalLength = 1.0f / tanHalfFovy;
    float A = (farPlane + nearPlane) / (nearPlane - farPlane);
    float B = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    
    // Load perspective matrix with correct aspect ratio
    float perspMatrix[16] = {
        focalLength / aspectRatio, 0.0f, 0.0f, 0.0f,
        0.0f, focalLength, 0.0f, 0.0f,
        0.0f, 0.0f, A, -1.0f,
        0.0f, 0.0f, B, 0.0f
    };
    glLoadMatrixf(perspMatrix);
    
    // Set up the modelview matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Calculate camera vectors
    glm::vec3 position = player->getPosition();
    glm::vec3 forward = player->getForward();
    glm::vec3 up = player->getUp();
    glm::vec3 right = player->getRight();
    
    // Set camera height
    float cameraHeight = 1.5f;
    position.y += cameraHeight;
    
    // Calculate camera target
    glm::vec3 target = position + forward;
    
    // Set up view matrix
    gluLookAt(
        position.x, position.y, position.z,
        target.x, target.y, target.z,
        up.x, up.y, up.z
    );
    
    // Debug output
    static int debugCounter = 0;
    if (debugCounter++ % 300 == 0) {
        std::cout << "CAMERA HEIGHT DEBUG: Setting camera at height " << cameraHeight 
                  << " voxels above player position: " << position.y << std::endl;
    }
}

void Renderer::renderHUD() {
    // Get window dimensions
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    
    // Save matrices and set up 2D projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    // Use top-left as origin (0,0) for consistency with all UI elements
    std::cout << "RenderHUD: Setting up orthographic projection with dimensions " 
              << width << "x" << height << " and top-left origin (0,0)" << std::endl;
    glOrtho(0, width, height, 0, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Disable depth testing for HUD
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    if (depthTestEnabled) glDisable(GL_DEPTH_TEST);
    
    // Render crosshair at screen center with top-left origin
    float crosshairSize = 10.0f;
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(width/2 - crosshairSize, height/2);
    glVertex2f(width/2 + crosshairSize, height/2);
    glVertex2f(width/2, height/2 - crosshairSize);
    glVertex2f(width/2, height/2 + crosshairSize);
    glEnd();
    
    // Restore matrices
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    // Restore depth testing if it was enabled
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
} 
