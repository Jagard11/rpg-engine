// ./src/Debug/GodViewDebugTool.cpp
#include "Debug/GodViewDebugTool.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Debug/DebugManager.hpp"
#include "Debug/Logger.hpp"
#include "Debug/Profiler.hpp"
#include "Utils/SphereUtils.hpp"
#include <vector>
#include <iostream>
#include <cmath>
#include <functional>

GodViewDebugTool::GodViewDebugTool(const World& world)
    : world(world), 
      active(false),
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
            
            uniform mat4 model, view, proj;
            uniform float surfaceRadius;
            uniform float heightScale;
            
            void main() {
                // Scale the position by the actual planet radius
                vec3 scaledPos = normalize(pos) * surfaceRadius;
                
                // Apply height exaggeration for better visualization
                // Scale the height to show the -5km to +15km range
                scaledPos += normalize(pos) * height * heightScale;
                
                gl_Position = proj * view * model * vec4(scaledPos, 1.0);
                fragNormal = normal;
                fragHeight = height;
            }
        )";
        
        const char* fragSrc = R"(
            #version 330 core
            in vec3 fragNormal;
            in float fragHeight;
            out vec4 FragColor;
            
            uniform int visualizationType;
            
            void main() {
                vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
                float diff = max(dot(normalize(fragNormal), lightDir), 0.2);
                
                // Visualization types
                if (visualizationType == 0) { // Height map
                    // Blue to brown to white gradient based on height
                    vec3 waterColor = vec3(0.0, 0.0, 0.7);
                    vec3 landColor = vec3(0.5, 0.3, 0.0);
                    vec3 mountainColor = vec3(1.0, 1.0, 1.0);
                    
                    vec3 baseColor;
                    if (fragHeight < 0.0) {
                        // Water (blend from deep to shallow)
                        float t = (fragHeight + 1.0) / 1.0; // -1 to 0 mapped to 0 to 1
                        baseColor = mix(vec3(0.0, 0.0, 0.5), waterColor, t);
                    } else if (fragHeight < 0.6) {
                        // Land (blend from beaches to grassland to forests)
                        float t = fragHeight / 0.6; // 0 to 0.6 mapped to 0 to 1
                        baseColor = mix(vec3(0.9, 0.8, 0.5), landColor, t);
                    } else {
                        // Mountains (blend from rock to snow)
                        float t = (fragHeight - 0.6) / 0.4; // 0.6 to 1.0 mapped to 0 to 1
                        baseColor = mix(landColor, mountainColor, t);
                    }
                    
                    FragColor = vec4(baseColor * diff, 1.0);
                } else if (visualizationType == 1) { // Biomes
                    // Simple biome visualization based on height and latitude
                    float latitude = asin(fragNormal.y) / 3.14159 * 2.0; // -1 to 1
                    
                    // Polar
                    if (abs(latitude) > 0.7) {
                        FragColor = vec4(1.0, 1.0, 1.0, 1.0) * diff; // Snow
                    }
                    // Temperate
                    else if (abs(latitude) > 0.4) {
                        if (fragHeight < 0.0) {
                            FragColor = vec4(0.0, 0.0, 0.7, 1.0) * diff; // Ocean
                        } else if (fragHeight < 0.3) {
                            FragColor = vec4(0.0, 0.6, 0.0, 1.0) * diff; // Forest
                        } else {
                            FragColor = vec4(0.5, 0.5, 0.5, 1.0) * diff; // Mountains
                        }
                    }
                    // Tropical
                    else {
                        if (fragHeight < 0.0) {
                            FragColor = vec4(0.0, 0.2, 0.8, 1.0) * diff; // Ocean
                        } else if (fragHeight < 0.2) {
                            FragColor = vec4(1.0, 0.9, 0.6, 1.0) * diff; // Beach/Desert
                        } else if (fragHeight < 0.5) {
                            FragColor = vec4(0.0, 0.8, 0.0, 1.0) * diff; // Jungle
                        } else {
                            FragColor = vec4(0.5, 0.5, 0.5, 1.0) * diff; // Mountains
                        }
                    }
                } else if (visualizationType == 2) { // Block density
                    // Visualize areas with more blocks as brighter
                    FragColor = vec4(0.0, fragHeight * 2.0, 0.0, 1.0) * diff;
                } else {         // Default wireframe color
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

float GodViewDebugTool::generateHeight(const glm::vec3& pos) {
    // Get surface radius
    double surfaceR = SphereUtils::getSurfaceRadiusMeters();
    
    // Convert position to world coordinates (assuming pos is normalized)
    double worldX = pos.x * surfaceR;
    double worldY = pos.y * surfaceR;
    double worldZ = pos.z * surfaceR;
    
    // Use distance to generate a basic height value
    double dist = sqrt(worldX*worldX + worldY*worldY + worldZ*worldZ);
    
    // Calculate height relative to surface (-1 to +1 range)
    float relativeHeight = static_cast<float>((dist - surfaceR) / 5000.0); // 5km scale
    
    // Add some noise based on position
    float noiseValue = (sin(pos.x * 10.0f) * cos(pos.z * 10.0f) + cos(pos.y * 8.0f)) * 0.1f;
    
    // Base height on latitude (distance from equator) for continental shapes
    float latitude = asin(pos.y) / 3.14159f;
    float latitudeEffect = 0.0f;
    
    // Create continents
    if (abs(latitude) < 0.6f) {
        // Noise creates continental shapes
        float continentalNoise = sin(pos.x * 3.0f) * cos(pos.z * 2.0f) + cos(pos.y * 5.0f);
        
        if (continentalNoise > 0.1f) {
            latitudeEffect = 0.3f; // Continent
        } else {
            latitudeEffect = -0.2f; // Ocean
        }
    } else {
        // Polar regions - create ice caps
        latitudeEffect = 0.3f + 0.1f * (abs(latitude) - 0.6f) / 0.4f;
    }
    
    // Combine all height factors
    float finalHeight = relativeHeight + noiseValue + latitudeEffect;
    
    // Clamp to reasonable range
    return glm::clamp(finalHeight, -1.0f, 1.5f);
}

void GodViewDebugTool::updateHeightData() {
    // This would update the height data based on current world state
    // For now, just logging that it was called
    LOG_DEBUG(LogCategory::RENDERING, "God View height data update requested");
}

bool GodViewDebugTool::generateGlobeMesh() {
    try {
        // Create a low-poly sphere to represent the globe
        // We'll subdivide an icosahedron for better uniformity than a UV sphere
        
        // Resolution of the globe mesh - lower for better performance
        const int resolution = 3; // Reduced from 4 to 3 subdivisions for performance
        
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
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "proj"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    
    // Set visualization type
    glUniform1i(glGetUniformLocation(shaderProgram, "visualizationType"), visualizationType);
    
    // Set surface radius (in kilometers)
    glUniform1f(glGetUniformLocation(shaderProgram, "surfaceRadius"), 
                static_cast<float>(SphereUtils::getSurfaceRadiusMeters() / 1000.0));
    
    // Set height scale for better visualization
    glUniform1f(glGetUniformLocation(shaderProgram, "heightScale"), 3000.0f);
    
    // Draw the globe
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
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