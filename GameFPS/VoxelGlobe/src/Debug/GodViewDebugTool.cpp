// ./src/Debug/GodViewDebugTool.cpp
#include "Debug/GodViewDebugTool.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Debug/DebugManager.hpp"
#include "Debug/Logger.hpp"
#include "Debug/Profiler.hpp"
#include "Utils/SphereUtils.hpp"
#include "imgui.h" // Added ImGui header
#include <vector>
#include <iostream>
#include <cmath>
#include <functional>
#include <sstream>
#include <unordered_map>

GodViewDebugTool::GodViewDebugTool(const World& world)
    : world(world), 
      active(false),  // Start inactive
      wireframeMode(false),
      visualizationType(0), // 0 = height map
      cameraPosition(0.0f, 0.0f, -30000.0f), // Start far away
      cameraTarget(0.0f, 0.0f, 0.0f),
      zoom(1.0f),
      rotationAngle(0.0f),
      indexCount(0),
      shadersLoaded(false)
{
    PROFILE_SCOPE("GodViewDebugTool::Constructor", LogCategory::RENDERING);
    
    try {
        // Generate OpenGL objects
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        
        // Load shaders
        if (!loadShaders()) {
            LOG_ERROR(LogCategory::RENDERING, "Failed to load shaders for God View Debug Tool");
            return;
        }
        
        // Generate initial globe mesh
        if (!generateGlobeMesh()) {
            LOG_ERROR(LogCategory::RENDERING, "Failed to generate globe mesh for God View Debug Tool");
            return;
        }
        
        shadersLoaded = true;
        LOG_INFO(LogCategory::RENDERING, "God View Debug Tool initialized successfully");
    } catch (const std::exception& e) {
        LOG_ERROR(LogCategory::RENDERING, "Exception in GodViewDebugTool constructor: " + std::string(e.what()));
    } catch (...) {
        LOG_ERROR(LogCategory::RENDERING, "Unknown exception in GodViewDebugTool constructor");
    }
}

GodViewDebugTool::~GodViewDebugTool() {
    PROFILE_SCOPE("GodViewDebugTool::Destructor", LogCategory::RENDERING);
    
    // Clean up OpenGL resources
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    
    if (shadersLoaded) {
        glDeleteProgram(shaderProgram);
    }
    
    LOG_INFO(LogCategory::RENDERING, "God View Debug Tool released");
}

bool GodViewDebugTool::loadShaders() {
    try {
        // Simple shader for rendering the globe
        const char* vertSrc = R"(
            #version 330 core
            layout(location = 0) in vec3 pos;
            layout(location = 1) in vec3 normal;
            layout(location = 2) in float height;
            
            out vec3 fragNormal;
            out float fragHeight;
            out vec3 fragWorldPos;
            
            uniform mat4 model, view, proj;
            uniform float surfaceRadius;
            uniform float heightScale;
            
            void main() {
                // Scale the position by the actual planet radius
                vec3 scaledPos = normalize(pos) * surfaceRadius;
                
                // Apply height exaggeration for better visualization
                // Use very minimal height variation for a mostly flat surface
                scaledPos += normalize(pos) * height * heightScale * 0.05;
                
                gl_Position = proj * view * model * vec4(scaledPos, 1.0);
                fragNormal = normal;
                fragHeight = height;
                fragWorldPos = scaledPos;
            }
        )";
        
        const char* fragSrc = R"(
            #version 330 core
            in vec3 fragNormal;
            in float fragHeight;
            in vec3 fragWorldPos;
            out vec4 FragColor;
            
            uniform int visualizationType;
            
            void main() {
                vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
                float diff = max(dot(normalize(fragNormal), lightDir), 0.2);
                
                // Visualization types
                if (visualizationType == 0) { // Terrain
                    // For flat surface, use consistent green (grass) color
                    vec3 grassColor = vec3(0.2, 0.7, 0.3);
                    FragColor = vec4(grassColor * diff, 1.0);
                }
                else if (visualizationType == 1) { // Biomes
                    // Simulate latitude-based biomes
                    float latitude = asin(normalize(fragWorldPos).y);
                    
                    // Snow at poles
                    if (abs(latitude) > 1.2) {
                        FragColor = vec4(0.95, 0.95, 0.95, 1.0) * diff;
                    }
                    // Tundra near poles
                    else if (abs(latitude) > 1.0) {
                        FragColor = vec4(0.7, 0.7, 0.7, 1.0) * diff;
                    }
                    // Temperate zones
                    else if (abs(latitude) > 0.5) {
                        FragColor = vec4(0.1, 0.6, 0.2, 1.0) * diff;
                    }
                    // Tropical zones
                    else {
                        FragColor = vec4(0.3, 0.8, 0.2, 1.0) * diff;
                    }
                }
                else if (visualizationType == 2) { // Blocks
                    // Uniform green for flat surface
                    FragColor = vec4(0.3, 0.7, 0.3, 1.0) * diff;
                }
                else { // Default wireframe color
                    FragColor = vec4(0.0, 1.0, 0.0, 1.0) * diff;
                }
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
            LOG_ERROR(LogCategory::RENDERING, std::string("God View Vertex Shader Error: ") + infoLog);
            glDeleteShader(vert);
            return false;
        }
        
        // Compile fragment shader
        GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(frag, 1, &fragSrc, NULL);
        glCompileShader(frag);
        glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(frag, 512, NULL, infoLog);
            LOG_ERROR(LogCategory::RENDERING, std::string("God View Fragment Shader Error: ") + infoLog);
            glDeleteShader(vert);
            glDeleteShader(frag);
            return false;
        }
        
        // Link program
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vert);
        glAttachShader(shaderProgram, frag);
        glLinkProgram(shaderProgram);
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
            LOG_ERROR(LogCategory::RENDERING, std::string("God View Shader Program Link Error: ") + infoLog);
            glDeleteShader(vert);
            glDeleteShader(frag);
            glDeleteProgram(shaderProgram);
            return false;
        }
        
        glDeleteShader(vert);
        glDeleteShader(frag);
        
        LOG_INFO(LogCategory::RENDERING, "God View shaders loaded successfully");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(LogCategory::RENDERING, "Exception during shader loading: " + std::string(e.what()));
        return false;
    } catch (...) {
        LOG_ERROR(LogCategory::RENDERING, "Unknown exception during shader loading");
        return false;
    }
}

// Generate height data that represents the actual world
float GodViewDebugTool::generateHeight(const glm::vec3& pos) {
    // Get surface radius
    double surfaceR = SphereUtils::getSurfaceRadiusMeters();
    
    // For a flat surface, we set a constant height value of 0.0
    // This creates a perfectly spherical planet as it's supposed to be
    // The height value is only used for visualization in shaders
    return 0.0f;
}

void GodViewDebugTool::updateHeightData() {
    // Nothing to update since we use a flat surface
    LOG_DEBUG(LogCategory::RENDERING, "God View height data update requested");
}

bool GodViewDebugTool::generateGlobeMesh() {
    try {
        // Create a higher-resolution sphere to represent the globe
        // We'll subdivide an icosahedron for better uniformity than a UV sphere
        
        // Resolution of the globe mesh - higher for better detail
        const int resolution = 6; // Increased from 3 for better resolution
        
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
        
        // Start with icosahedron vertices
        const float X = 0.525731f;
        const float Z = 0.850651f;
        
        std::vector<glm::vec3> positions = {
            {-X, 0.0f, Z}, {X, 0.0f, Z}, {-X, 0.0f, -Z}, {X, 0.0f, -Z},
            {0.0f, Z, X}, {0.0f, Z, -X}, {0.0f, -Z, X}, {0.0f, -Z, -X},
            {Z, X, 0.0f}, {-Z, X, 0.0f}, {Z, -X, 0.0f}, {-Z, -X, 0.0f}
        };
        
        // Icosahedron faces (20 triangles)
        std::vector<std::vector<int>> faces = {
            {0, 4, 1}, {0, 9, 4}, {9, 5, 4}, {4, 5, 8}, {4, 8, 1},
            {8, 10, 1}, {8, 3, 10}, {5, 3, 8}, {5, 2, 3}, {2, 7, 3},
            {7, 10, 3}, {7, 6, 10}, {7, 11, 6}, {11, 0, 6}, {0, 1, 6},
            {6, 1, 10}, {9, 0, 11}, {9, 11, 2}, {9, 2, 5}, {7, 2, 11}
        };
        
        // Map to store unique vertices and their indices
        std::unordered_map<std::string, unsigned int> vertexMap;
        
        // Function to get unique vertex index or add it if it doesn't exist
        auto getVertexIndex = [&](const glm::vec3& pos, float height) -> unsigned int {
            // Create a unique key for the vertex
            std::stringstream ss;
            ss << pos.x << "," << pos.y << "," << pos.z;
            std::string key = ss.str();
            
            auto it = vertexMap.find(key);
            if (it != vertexMap.end()) {
                return it->second;
            }
            
            // Add new vertex
            unsigned int index = vertices.size() / 7;
            
            // Position
            vertices.push_back(pos.x);
            vertices.push_back(pos.y);
            vertices.push_back(pos.z);
            
            // Normal (same as position for unit sphere)
            vertices.push_back(pos.x);
            vertices.push_back(pos.y);
            vertices.push_back(pos.z);
            
            // Height
            vertices.push_back(height);
            
            vertexMap[key] = index;
            return index;
        };
        
        // Function to subdivide a triangle
        std::function<void(const glm::vec3&, const glm::vec3&, const glm::vec3&, int)> subdivide;
        subdivide = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, int depth) {
            if (depth == 0) {
                // Generate height values based on position
                float heightA = generateHeight(a);
                float heightB = generateHeight(b);
                float heightC = generateHeight(c);
                
                // Get indices for the three vertices
                unsigned int idxA = getVertexIndex(a, heightA);
                unsigned int idxB = getVertexIndex(b, heightB);
                unsigned int idxC = getVertexIndex(c, heightC);
                
                // Add indices for the triangle
                indices.push_back(idxA);
                indices.push_back(idxB);
                indices.push_back(idxC);
                return;
            }
            
            // Calculate midpoints and normalize to keep on unit sphere
            glm::vec3 ab = glm::normalize(glm::mix(a, b, 0.5f));
            glm::vec3 bc = glm::normalize(glm::mix(b, c, 0.5f));
            glm::vec3 ca = glm::normalize(glm::mix(c, a, 0.5f));
            
            // Subdivide the 4 new triangles
            subdivide(a, ab, ca, depth - 1);
            subdivide(ab, b, bc, depth - 1);
            subdivide(ca, bc, c, depth - 1);
            subdivide(ab, bc, ca, depth - 1);
        };
        
        // Subdivide all faces of the icosahedron
        for (const auto& face : faces) {
            glm::vec3 a = positions[face[0]];
            glm::vec3 b = positions[face[1]];
            glm::vec3 c = positions[face[2]];
            subdivide(a, b, c, resolution);
        }
        
        // Verify we have vertices and indices
        if (vertices.empty() || indices.empty()) {
            LOG_ERROR(LogCategory::RENDERING, "Generated empty globe mesh");
            return false;
        }
        
        // Upload to GPU
        glBindVertexArray(vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // Height attribute
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        glBindVertexArray(0);
        
        // Store index count for rendering
        indexCount = indices.size();
        
        LOG_INFO(LogCategory::RENDERING, "God View globe mesh generated with " + 
            std::to_string(vertices.size() / 7) + " vertices and " + 
            std::to_string(indices.size() / 3) + " triangles");
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(LogCategory::RENDERING, "Exception in globe mesh generation: " + std::string(e.what()));
        return false;
    } catch (...) {
        LOG_ERROR(LogCategory::RENDERING, "Unknown exception in globe mesh generation");
        return false;
    }
}

void GodViewDebugTool::render(const GraphicsSettings& settings) {
    if (!active || !shadersLoaded) {
        return;
    }
    
    PROFILE_SCOPE("GodViewDebugTool::render", LogCategory::RENDERING);
    
    // Safety check - don't try to render if OpenGL isn't ready
    GLint currentFBO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
    if (glGetError() != GL_NO_ERROR) {
        LOG_ERROR(LogCategory::RENDERING, "OpenGL error before GodViewDebugTool rendering");
        return;
    }
    
    // Store previous GL state
    GLboolean cullFace;
    glGetBooleanv(GL_CULL_FACE, &cullFace);
    
    // Set rendering state
    if (wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    
    // Disable culling for simpler rendering
    glDisable(GL_CULL_FACE);
    
    // Use shader
    glUseProgram(shaderProgram);
    
    // Set up view and projection
    float aspectRatio = static_cast<float>(settings.getWidth()) / settings.getHeight();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 1.0f, 1000000.0f);
    
    // Create view matrix based on camera settings
    glm::mat4 view = glm::lookAt(
        cameraPosition * zoom,
        cameraTarget,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    // Apply rotation
    glm::mat4 model = glm::rotate(
        glm::mat4(1.0f),
        glm::radians(rotationAngle),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    // Apply transforms
    GLint projLoc = glGetUniformLocation(shaderProgram, "proj");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    
    if (projLoc >= 0) glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    if (viewLoc >= 0) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    if (modelLoc >= 0) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    
    // Set visualization type
    GLint vizLoc = glGetUniformLocation(shaderProgram, "visualizationType");
    if (vizLoc >= 0) glUniform1i(vizLoc, visualizationType);
    
    // Set surface radius (in kilometers)
    GLint radiusLoc = glGetUniformLocation(shaderProgram, "surfaceRadius");
    if (radiusLoc >= 0) {
        float radius = static_cast<float>(SphereUtils::getSurfaceRadiusMeters() / 1000.0);
        glUniform1f(radiusLoc, radius);
    }
    
    // Set height scale for better visualization
    GLint heightLoc = glGetUniformLocation(shaderProgram, "heightScale");
    if (heightLoc >= 0) glUniform1f(heightLoc, 3000.0f);
    
    // Draw the globe
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Check for OpenGL errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        LOG_ERROR(LogCategory::RENDERING, "OpenGL error during God View rendering: " + std::to_string(err));
    }
    
    // Restore polygon mode
    if (wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    // Restore culling state
    if (cullFace) {
        glEnable(GL_CULL_FACE);
    }
}

// Setter methods
void GodViewDebugTool::setCameraPosition(const glm::vec3& position) {
    cameraPosition = position;
}

void GodViewDebugTool::setCameraTarget(const glm::vec3& target) {
    cameraTarget = target;
}

void GodViewDebugTool::setZoom(float zoomFactor) {
    zoom = zoomFactor;
}

void GodViewDebugTool::rotateView(float degrees) {
    rotationAngle = degrees;
}

void GodViewDebugTool::setWireframeMode(bool enabled) {
    wireframeMode = enabled;
}

void GodViewDebugTool::setVisualizationType(int type) {
    visualizationType = type;
}

void GodViewDebugTool::setActive(bool enabled) {
    active = enabled;
}

float GodViewDebugTool::getCurrentRotation() const {
    return rotationAngle;
}