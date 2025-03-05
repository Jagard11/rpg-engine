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

// Improved sphere projection function (matching the one in Chunk.cpp exactly)
glm::vec3 projectHighlightToSphere(const glm::vec3& worldPos, float surfaceR, bool isInner, int faceType) {
    // Safety checks for invalid input
    if (glm::length(worldPos) < 0.001f) {
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }
    
    // Get the voxel's integer block position (using floor to ensure consistency)
    glm::ivec3 blockPos = glm::ivec3(floor(worldPos.x), floor(worldPos.y), floor(worldPos.z));
    
    // Calculate block center in world space (always at x.5, y.5, z.5)
    glm::vec3 blockCenter = glm::vec3(blockPos) + glm::vec3(0.5f);
    
    // Get normalized direction from world origin to block center
    // This ensures all vertices from the same block use the same direction vector
    glm::vec3 blockDir = glm::normalize(blockCenter);
    
    // Calculate distance from center to block center using double precision
    // This is crucial for maintaining precision far from origin
    double bx = static_cast<double>(blockCenter.x);
    double by = static_cast<double>(blockCenter.y);
    double bz = static_cast<double>(blockCenter.z);
    double blockDistance = sqrt(bx*bx + by*by + bz*bz);
    
    // Calculate height layer using floor to ensure consistent layers
    int heightLayer = static_cast<int>(floor(blockDistance - surfaceR));
    
    // Base radius for this height layer (use precise integer offsets)
    float baseRadius = surfaceR + static_cast<float>(heightLayer);
    
    // Select appropriate radius based on face type
    float radius;
    
    // Face types:
    // 2 = top face (+Y), 3 = bottom face (-Y), others = side faces
    if (faceType == 2) { // Top face (+Y)
        // Exactly 1.0 unit above the base radius
        radius = baseRadius + 1.0f;
    } else if (faceType == 3) { // Bottom face (-Y)
        // Exactly at the base radius
        radius = baseRadius;
    } else { // Side faces
        // For side faces, use exact local Y position (0.0 to 1.0)
        float localY = worldPos.y - blockPos.y;
        radius = baseRadius + localY;
    }
    
    // Project the vertex onto the sphere at the calculated radius
    // Using the block's center direction for all vertices ensures alignment
    return blockDir * radius;
}

void VoxelHighlightUI::render(const Player& player, const glm::ivec3& voxelPos, const GraphicsSettings& settings) {
    // Only skip if there's no valid voxel to highlight
    if (voxelPos.x == -1) {
        return;
    }
    
    // Update last highlighted position
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

    // Get surface radius from world
    float surfaceR = player.getWorld().getRadius() + 8.0f;
    
    // Determine if block is below surface using double precision
    double vx = static_cast<double>(voxelPos.x);
    double vy = static_cast<double>(voxelPos.y);
    double vz = static_cast<double>(voxelPos.z);
    double dist = sqrt(vx*vx + vy*vy + vz*vz);
    bool isInner = (dist < surfaceR);
    
    // Define vertices for the cube highlight with a slight expansion for visibility
    float expand = 0.01f; // Expand highlight slightly beyond block bounds
    std::vector<glm::vec3> cubeVertices = {
        // Bottom face vertices
        glm::vec3(0.0f - expand, 0.0f - expand, 0.0f - expand),
        glm::vec3(1.0f + expand, 0.0f - expand, 0.0f - expand),
        glm::vec3(1.0f + expand, 0.0f - expand, 1.0f + expand),
        glm::vec3(0.0f - expand, 0.0f - expand, 1.0f + expand),
        
        // Top face vertices
        glm::vec3(0.0f - expand, 1.0f + expand, 0.0f - expand),
        glm::vec3(1.0f + expand, 1.0f + expand, 0.0f - expand),
        glm::vec3(1.0f + expand, 1.0f + expand, 1.0f + expand),
        glm::vec3(0.0f - expand, 1.0f + expand, 1.0f + expand)
    };
    
    // Project vertices to sphere - with proper face information
    // First determine which face each vertex belongs to
    int faceTypes[8] = {
        3, 3, 3, 3,  // Bottom face vertices (face type 3 = -Y)
        2, 2, 2, 2   // Top face vertices (face type 2 = +Y)
    };
    
    for (int i = 0; i < 8; i++) {
        // Convert to world coordinate
        glm::vec3 worldVertex = glm::vec3(voxelPos) + cubeVertices[i];
        
        // Project to sphere with proper face type info
        glm::vec3 projectedVertex = projectHighlightToSphere(worldVertex, surfaceR, isInner, faceTypes[i]);
        
        // Convert back to local space
        cubeVertices[i] = projectedVertex - glm::vec3(voxelPos);
    }

    // Define lines for wireframe cube using the projected vertices
    std::vector<float> vertices;
    
    // Bottom face
    vertices.push_back(cubeVertices[0].x); vertices.push_back(cubeVertices[0].y); vertices.push_back(cubeVertices[0].z);
    vertices.push_back(cubeVertices[1].x); vertices.push_back(cubeVertices[1].y); vertices.push_back(cubeVertices[1].z);
    
    vertices.push_back(cubeVertices[1].x); vertices.push_back(cubeVertices[1].y); vertices.push_back(cubeVertices[1].z);
    vertices.push_back(cubeVertices[2].x); vertices.push_back(cubeVertices[2].y); vertices.push_back(cubeVertices[2].z);
    
    vertices.push_back(cubeVertices[2].x); vertices.push_back(cubeVertices[2].y); vertices.push_back(cubeVertices[2].z);
    vertices.push_back(cubeVertices[3].x); vertices.push_back(cubeVertices[3].y); vertices.push_back(cubeVertices[3].z);
    
    vertices.push_back(cubeVertices[3].x); vertices.push_back(cubeVertices[3].y); vertices.push_back(cubeVertices[3].z);
    vertices.push_back(cubeVertices[0].x); vertices.push_back(cubeVertices[0].y); vertices.push_back(cubeVertices[0].z);
    
    // Top face
    vertices.push_back(cubeVertices[4].x); vertices.push_back(cubeVertices[4].y); vertices.push_back(cubeVertices[4].z);
    vertices.push_back(cubeVertices[5].x); vertices.push_back(cubeVertices[5].y); vertices.push_back(cubeVertices[5].z);
    
    vertices.push_back(cubeVertices[5].x); vertices.push_back(cubeVertices[5].y); vertices.push_back(cubeVertices[5].z);
    vertices.push_back(cubeVertices[6].x); vertices.push_back(cubeVertices[6].y); vertices.push_back(cubeVertices[6].z);
    
    vertices.push_back(cubeVertices[6].x); vertices.push_back(cubeVertices[6].y); vertices.push_back(cubeVertices[6].z);
    vertices.push_back(cubeVertices[7].x); vertices.push_back(cubeVertices[7].y); vertices.push_back(cubeVertices[7].z);
    
    vertices.push_back(cubeVertices[7].x); vertices.push_back(cubeVertices[7].y); vertices.push_back(cubeVertices[7].z);
    vertices.push_back(cubeVertices[4].x); vertices.push_back(cubeVertices[4].y); vertices.push_back(cubeVertices[4].z);
    
    // Connecting edges
    vertices.push_back(cubeVertices[0].x); vertices.push_back(cubeVertices[0].y); vertices.push_back(cubeVertices[0].z);
    vertices.push_back(cubeVertices[4].x); vertices.push_back(cubeVertices[4].y); vertices.push_back(cubeVertices[4].z);
    
    vertices.push_back(cubeVertices[1].x); vertices.push_back(cubeVertices[1].y); vertices.push_back(cubeVertices[1].z);
    vertices.push_back(cubeVertices[5].x); vertices.push_back(cubeVertices[5].y); vertices.push_back(cubeVertices[5].z);
    
    vertices.push_back(cubeVertices[2].x); vertices.push_back(cubeVertices[2].y); vertices.push_back(cubeVertices[2].z);
    vertices.push_back(cubeVertices[6].x); vertices.push_back(cubeVertices[6].y); vertices.push_back(cubeVertices[6].z);
    
    vertices.push_back(cubeVertices[3].x); vertices.push_back(cubeVertices[3].y); vertices.push_back(cubeVertices[3].z);
    vertices.push_back(cubeVertices[7].x); vertices.push_back(cubeVertices[7].y); vertices.push_back(cubeVertices[7].z);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Set up view and projection matrices
    glm::mat4 proj = glm::perspective(glm::radians(70.0f), static_cast<float>(settings.getWidth()) / settings.getHeight(), 0.1f, 5000.0f);
    glm::vec3 eyePos = player.position + player.up * player.getHeight();
    glm::vec3 lookAtPos = eyePos + player.cameraDirection;
    
    // Use corrected up vector for highlight rendering
    glm::vec3 viewDir = glm::normalize(lookAtPos - eyePos);
    glm::vec3 rightDir = glm::normalize(glm::cross(viewDir, player.up));
    glm::vec3 upDir = glm::normalize(glm::cross(rightDir, viewDir));
    
    glm::mat4 view = glm::lookAt(eyePos, lookAtPos, upDir);

    // Position the highlight at the voxel
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(voxelPos));
    
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