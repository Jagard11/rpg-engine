// ./src/UI/VoxelHighlightUI.cpp
#include "UI/VoxelHighlightUI.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "Debug/DebugManager.hpp"
#include <iostream>
#include <vector>
#include "Utils/SphereUtils.hpp"

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

// Improved sphere projection function for voxel highlight
glm::vec3 VoxelHighlightUI::projectToSphere(const glm::vec3& pos, float surfaceR, double distFromCenter) const {
    // Get the normalized direction from center
    glm::vec3 dirFromCenter = glm::normalize(pos);
    
    // Project to the correct radius
    return dirFromCenter * static_cast<float>(distFromCenter);
}

void VoxelHighlightUI::render(const Player& player, const glm::ivec3& voxelPos, const GraphicsSettings& settings) {
    // Skip if there's no valid voxel to highlight
    if (voxelPos.x == -1 && voxelPos.y == -1 && voxelPos.z == -1) {
        return;
    }
    
    // Update last highlighted position
    lastHighlightedVoxel = voxelPos;

    // Disable culling to ensure we see all edges of the highlight
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    
    // Draw slightly offset from actual blocks to avoid z-fighting
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);

    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Get surface radius from world
    float surfaceR = static_cast<float>(SphereUtils::getSurfaceRadiusMeters());
    
    // Calculate distance from center for the voxel
    double vx = static_cast<double>(voxelPos.x);
    double vy = static_cast<double>(voxelPos.y);
    double vz = static_cast<double>(voxelPos.z);
    double voxelDistFromCenter = sqrt(vx*vx + vy*vy + vz*vz);
    
    // Debug logs
    if (DebugManager::getInstance().logRaycast()) {
        std::cout << "Highlighting voxel at (" << voxelPos.x << ", " << voxelPos.y << ", " << voxelPos.z << ")" << std::endl;
        std::cout << "Voxel distance from center: " << voxelDistFromCenter << ", surface at: " << surfaceR << std::endl;
    }
    
    // EARTH-SCALE FIX: Rebase the voxel position relative to player position
    // This treats the player as the origin (0,0,0) for rendering
    glm::vec3 rebasedVoxelPos = glm::vec3(voxelPos) - player.position;
    
    // Define vertices for the cube highlight with a slight expansion for visibility
    float expand = 0.01f; // Expand highlight slightly beyond block bounds
    
    // EARTH-SCALE FIX: Use a local coordinate frame aligned with planet surface
    glm::vec3 upVector = glm::normalize(glm::vec3(voxelPos));  // Up is away from planet center
    
    // Create orthogonal basis for the highlighted voxel
    glm::vec3 refVector = (std::abs(upVector.y) > 0.99f) ? 
                          glm::vec3(1.0f, 0.0f, 0.0f) : 
                          glm::vec3(0.0f, 1.0f, 0.0f);
    
    glm::vec3 rightVector = glm::normalize(glm::cross(refVector, upVector));
    glm::vec3 forwardVector = glm::normalize(glm::cross(upVector, rightVector));
    
    // Calculate inner and outer distances from center
    double innerDistance = voxelDistFromCenter - 0.5;
    double outerDistance = voxelDistFromCenter + 0.5;
    
    // Calculate corner positions relative to the voxel center
    std::vector<glm::vec3> cornerDirections = {
        upVector * -0.5f - rightVector * 0.5f - forwardVector * 0.5f,  // Bottom, left, back
        upVector * -0.5f + rightVector * 0.5f - forwardVector * 0.5f,  // Bottom, right, back
        upVector * -0.5f + rightVector * 0.5f + forwardVector * 0.5f,  // Bottom, right, front
        upVector * -0.5f - rightVector * 0.5f + forwardVector * 0.5f,  // Bottom, left, front
        upVector * 0.5f - rightVector * 0.5f - forwardVector * 0.5f,   // Top, left, back
        upVector * 0.5f + rightVector * 0.5f - forwardVector * 0.5f,   // Top, right, back
        upVector * 0.5f + rightVector * 0.5f + forwardVector * 0.5f,   // Top, right, front
        upVector * 0.5f - rightVector * 0.5f + forwardVector * 0.5f    // Top, left, front
    };
    
    // Calculate corner positions in player-relative space
    std::vector<glm::vec3> corners;
    for (int i = 0; i < 8; i++) {
        // Apply expansion for visibility
        cornerDirections[i] *= (1.0f + expand);
        
        // Adjust the distance from center based on whether this is a top or bottom corner
        float cornerDist = (i < 4) ? innerDistance : outerDistance;
        
        // Calculate corner position in world space
        glm::vec3 cornerVoxelPos = rebasedVoxelPos + cornerDirections[i];
        
        // Project to correct sphere radius
        corners.push_back(cornerVoxelPos);
    }

    // Define lines for wireframe cube 
    std::vector<float> vertices;
    
    // Bottom face edges
    vertices.push_back(corners[0].x); vertices.push_back(corners[0].y); vertices.push_back(corners[0].z);
    vertices.push_back(corners[1].x); vertices.push_back(corners[1].y); vertices.push_back(corners[1].z);
    
    vertices.push_back(corners[1].x); vertices.push_back(corners[1].y); vertices.push_back(corners[1].z);
    vertices.push_back(corners[2].x); vertices.push_back(corners[2].y); vertices.push_back(corners[2].z);
    
    vertices.push_back(corners[2].x); vertices.push_back(corners[2].y); vertices.push_back(corners[2].z);
    vertices.push_back(corners[3].x); vertices.push_back(corners[3].y); vertices.push_back(corners[3].z);
    
    vertices.push_back(corners[3].x); vertices.push_back(corners[3].y); vertices.push_back(corners[3].z);
    vertices.push_back(corners[0].x); vertices.push_back(corners[0].y); vertices.push_back(corners[0].z);
    
    // Top face edges
    vertices.push_back(corners[4].x); vertices.push_back(corners[4].y); vertices.push_back(corners[4].z);
    vertices.push_back(corners[5].x); vertices.push_back(corners[5].y); vertices.push_back(corners[5].z);
    
    vertices.push_back(corners[5].x); vertices.push_back(corners[5].y); vertices.push_back(corners[5].z);
    vertices.push_back(corners[6].x); vertices.push_back(corners[6].y); vertices.push_back(corners[6].z);
    
    vertices.push_back(corners[6].x); vertices.push_back(corners[6].y); vertices.push_back(corners[6].z);
    vertices.push_back(corners[7].x); vertices.push_back(corners[7].y); vertices.push_back(corners[7].z);
    
    vertices.push_back(corners[7].x); vertices.push_back(corners[7].y); vertices.push_back(corners[7].z);
    vertices.push_back(corners[4].x); vertices.push_back(corners[4].y); vertices.push_back(corners[4].z);
    
    // Connecting edges
    vertices.push_back(corners[0].x); vertices.push_back(corners[0].y); vertices.push_back(corners[0].z);
    vertices.push_back(corners[4].x); vertices.push_back(corners[4].y); vertices.push_back(corners[4].z);
    
    vertices.push_back(corners[1].x); vertices.push_back(corners[1].y); vertices.push_back(corners[1].z);
    vertices.push_back(corners[5].x); vertices.push_back(corners[5].y); vertices.push_back(corners[5].z);
    
    vertices.push_back(corners[2].x); vertices.push_back(corners[2].y); vertices.push_back(corners[2].z);
    vertices.push_back(corners[6].x); vertices.push_back(corners[6].y); vertices.push_back(corners[6].z);
    
    vertices.push_back(corners[3].x); vertices.push_back(corners[3].y); vertices.push_back(corners[3].z);
    vertices.push_back(corners[7].x); vertices.push_back(corners[7].y); vertices.push_back(corners[7].z);

    // Load vertices into GPU
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Set up view and projection matrices - using player-relative coordinates
    glm::mat4 proj = glm::perspective(
        glm::radians(70.0f), 
        static_cast<float>(settings.getWidth()) / settings.getHeight(), 
        0.1f, // Increased near plane for better depth precision
        1000.0f // Reduced far plane for better depth precision
    );
    
    // EARTH-SCALE FIX: Create a view matrix with player at origin
    float playerHeight = player.getHeight();
    glm::vec3 eyePos = glm::vec3(0, playerHeight, 0); // Player is at origin
    glm::vec3 lookAtPos = eyePos + player.cameraDirection;
    
    // Calculate orthogonal up vector to ensure proper orientation
    glm::vec3 viewDir = glm::normalize(lookAtPos - eyePos);
    glm::vec3 rightDir = glm::normalize(glm::cross(viewDir, player.up));
    glm::vec3 upDir = glm::normalize(glm::cross(rightDir, viewDir));
    
    // Create view matrix with player at origin
    glm::mat4 view = glm::lookAt(eyePos, lookAtPos, upDir);
    
    // No need for model matrix (identity) - all coordinates are already player-relative
    glm::mat4 model = glm::mat4(1.0f);
    
    // Apply the transformation matrices
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "proj"), 1, GL_FALSE, &proj[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    
    // Set highlight color (bright yellow for visibility)
    glUniform4f(glGetUniformLocation(shaderProgram, "highlightColor"), 1.0f, 1.0f, 0.0f, 1.0f);

    // Draw the highlight wireframe
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, vertices.size() / 3);

    // Restore GL state
    glEnable(GL_CULL_FACE);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void VoxelHighlightUI::loadShader() {
    // Vertex shader for the highlight wireframe - with player-relative coordinates
    const char* vertSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 pos;
        uniform mat4 model, view, proj;
        void main() {
            gl_Position = proj * view * model * vec4(pos, 1.0);
        }
    )";
    
    // Fragment shader with configurable highlight color
    const char* fragSrc = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec4 highlightColor;
        void main() {
            FragColor = highlightColor; // Use configurable highlight color
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