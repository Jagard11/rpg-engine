// ./src/Debug/GodViewDebugTool.cpp
#include "Debug/GodViewDebugTool.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Debug/DebugManager.hpp"
#include "Debug/Logger.hpp"
#include "Debug/Profiler.hpp"
#include "Utils/SphereUtils.hpp"
#include "imgui.h"
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
      shadersLoaded(false),
      visualizationMode(VisualizationMode::HYBRID),
      useAdaptiveResolution(true),
      adaptiveDetailFactor(1.0f),
      meshDirty(true),
      cameraPosition(0.0f, 0.0f, -30000.0f), // Start far away
      cameraTarget(0.0f, 0.0f, 0.0f),
      zoom(1.0f),
      rotationAngle(0.0f),
      indexCount(0),
      updateInProgress(false),
      vao(0), vbo(0), ebo(0), shaderProgram(0)     // Initialize OpenGL handles to 0
{
    PROFILE_SCOPE("GodViewDebugTool::Constructor", LogCategory::RENDERING);
    
    try {
        // Check if OpenGL is initialized by checking for errors
        // This is a simple way to see if we have a valid context
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            LOG_ERROR(LogCategory::RENDERING, "OpenGL error detected during GodViewDebugTool initialization: " + std::to_string(err));
            return;
        }
        
        glGenVertexArrays(1, &vao);
        if (glGetError() != GL_NO_ERROR || vao == 0) {
            LOG_ERROR(LogCategory::RENDERING, "Failed to generate VAO for God View Debug Tool");
            return;
        }
        
        glGenBuffers(1, &vbo);
        if (glGetError() != GL_NO_ERROR || vbo == 0) {
            LOG_ERROR(LogCategory::RENDERING, "Failed to generate VBO for God View Debug Tool");
            glDeleteVertexArrays(1, &vao);
            vao = 0;
            return;
        }
        
        glGenBuffers(1, &ebo);
        if (glGetError() != GL_NO_ERROR || ebo == 0) {
            LOG_ERROR(LogCategory::RENDERING, "Failed to generate EBO for God View Debug Tool");
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            vao = 0;
            vbo = 0;
            return;
        }
        
        // Load shaders
        if (!loadShaders()) {
            LOG_ERROR(LogCategory::RENDERING, "Failed to load shaders for God View Debug Tool");
            releaseResources();
            return;
        }
        
        // Generate initial globe mesh
        if (!generateGlobeMesh()) {
            LOG_ERROR(LogCategory::RENDERING, "Failed to generate globe mesh for God View Debug Tool");
            releaseResources();
            return;
        }
        
        shadersLoaded = true;
        LOG_INFO(LogCategory::RENDERING, "God View Debug Tool initialized successfully");
    } catch (const std::exception& e) {
        LOG_ERROR(LogCategory::RENDERING, "Exception in GodViewDebugTool constructor: " + std::string(e.what()));
        releaseResources();
    } catch (...) {
        LOG_ERROR(LogCategory::RENDERING, "Unknown exception in GodViewDebugTool constructor");
        releaseResources();
    }
}

// Helper method to clean up OpenGL resources
void GodViewDebugTool::releaseResources() {
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    
    if (ebo != 0) {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
    }
    
    if (shaderProgram != 0 && shadersLoaded) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
        shadersLoaded = false;
    }
}

GodViewDebugTool::~GodViewDebugTool() {
    PROFILE_SCOPE("GodViewDebugTool::Destructor", LogCategory::RENDERING);
    
    // Wait for any ongoing async operations to complete
    if (updateFuture.valid()) {
        try {
            updateFuture.wait();
        } catch (...) {
            // Ignore exceptions from async tasks during destruction
        }
    }
    
    releaseResources();
    LOG_INFO(LogCategory::RENDERING, "God View Debug Tool released");
}

bool GodViewDebugTool::loadShaders() {
    try {
        // Enhanced shader for rendering the globe with multiple visualization modes
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
                scaledPos += normalize(pos) * height * heightScale;
                
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
            
            uniform int visualizationType;     // 0=terrain, 1=biomes, 2=blocks
            uniform int visualizationMode;     // 0=procedural, 1=actual, 2=hybrid
            
            // Color helpers
            vec3 heightToColor(float height) {
                // Map height to color gradient
                // Normalized height in [0,1] where 0.5 is sea level
                float normalizedHeight = height * 0.5 + 0.5;
                
                if (normalizedHeight < 0.3) {
                    // Deep water (dark blue to medium blue)
                    return mix(vec3(0.0, 0.0, 0.5), vec3(0.0, 0.0, 0.8), normalizedHeight / 0.3);
                } else if (normalizedHeight < 0.5) {
                    // Shallow water (medium blue to light blue)
                    return mix(vec3(0.0, 0.0, 0.8), vec3(0.0, 0.5, 1.0), (normalizedHeight - 0.3) / 0.2);
                } else if (normalizedHeight < 0.55) {
                    // Beach/shore (light blue to sand)
                    return mix(vec3(0.0, 0.5, 1.0), vec3(0.95, 0.95, 0.7), (normalizedHeight - 0.5) / 0.05);
                } else if (normalizedHeight < 0.7) {
                    // Lowlands (sand to grass green)
                    return mix(vec3(0.95, 0.95, 0.7), vec3(0.2, 0.6, 0.1), (normalizedHeight - 0.55) / 0.15);
                } else if (normalizedHeight < 0.85) {
                    // Highlands (grass green to mountain brown)
                    return mix(vec3(0.2, 0.6, 0.1), vec3(0.5, 0.3, 0.1), (normalizedHeight - 0.7) / 0.15);
                } else {
                    // Mountains/peaks (mountain brown to snow white)
                    return mix(vec3(0.5, 0.3, 0.1), vec3(1.0, 1.0, 1.0), (normalizedHeight - 0.85) / 0.15);
                }
            }
            
            vec3 biomeColor(vec3 worldPos) {
                // Calculate latitude and longitude
                float latitude = asin(normalize(worldPos).y);
                float longitude = atan(worldPos.z, worldPos.x);
                
                // Normalize to [0,1] range
                float lat01 = latitude / 3.14159 + 0.5;
                
                // Ice caps at poles
                if (lat01 < 0.1 || lat01 > 0.9) {
                    return vec3(0.95, 0.95, 0.95); // Snow white
                }
                // Tundra
                else if (lat01 < 0.2 || lat01 > 0.8) {
                    return vec3(0.7, 0.7, 0.7); // Light gray
                }
                // Temperate zones
                else if (lat01 < 0.3 || lat01 > 0.7) {
                    return vec3(0.1, 0.5, 0.1); // Forest green
                }
                // Subtropical
                else if (lat01 < 0.4 || lat01 > 0.6) {
                    return vec3(0.2, 0.7, 0.1); // Vibrant green
                }
                // Tropical zone
                else {
                    // Add some variation based on longitude
                    float longVar = (sin(longitude * 5.0) + 1.0) * 0.5;
                    if (longVar > 0.6) {
                        return vec3(0.95, 0.9, 0.6); // Desert tan
                    } else {
                        return vec3(0.0, 0.8, 0.0); // Tropical green
                    }
                }
            }
            
            vec3 blockTypeColor() {
                // Determine if this point likely has actual data
                bool isActualData = (visualizationMode == 1) || 
                                   (visualizationMode == 2 && fragHeight > 0.01);
                
                if (isActualData) {
                    // Brighter colors for actual data
                    if (fragHeight < 0.33) {
                        return vec3(0.6, 0.3, 0.1); // Dirt (brighter)
                    } else if (fragHeight < 0.67) {
                        return vec3(0.0, 0.8, 0.0); // Grass (brighter)
                    } else {
                        return vec3(0.9, 0.9, 0.2); // Other block (bright yellow)
                    }
                } else {
                    // Default procedural colors (more muted)
                    if (fragHeight < 0.33) {
                        return vec3(0.4, 0.2, 0.1); // Dirt
                    } else if (fragHeight < 0.67) {
                        return vec3(0.0, 0.5, 0.0); // Grass
                    } else {
                        return vec3(0.7, 0.7, 0.7); // Other (light gray)
                    }
                }
            }
            
            void main() {
                vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
                float diff = max(dot(normalize(fragNormal), lightDir), 0.2);
                vec3 baseColor;
                
                // Choose base color based on visualization type
                if (visualizationType == 0) {
                    // Terrain height map
                    baseColor = heightToColor(fragHeight);
                    
                    // If we're in hybrid mode, subtly highlight player-modified areas
                    if (visualizationMode == 2 && fragHeight > 0.01) {
                        baseColor = mix(baseColor, vec3(1.0, 1.0, 0.0), 0.2);
                    }
                }
                else if (visualizationType == 1) {
                    // Biomes
                    baseColor = biomeColor(fragWorldPos);
                    
                    // If we're in hybrid mode, subtly highlight player-modified areas
                    if (visualizationMode == 2 && fragHeight > 0.01) {
                        baseColor = mix(baseColor, vec3(1.0, 1.0, 0.0), 0.2);
                    }
                }
                else if (visualizationType == 2) {
                    // Block types
                    baseColor = blockTypeColor();
                }
                else {
                    // Default fallback
                    baseColor = vec3(0.0, 1.0, 0.0);
                }
                
                // Apply lighting
                FragColor = vec4(baseColor * diff, 1.0);
            }
        )";
        
        // Compile vertex shader
        GLuint vert = glCreateShader(GL_VERTEX_SHADER);
        if (vert == 0) {
            LOG_ERROR(LogCategory::RENDERING, "Failed to create vertex shader");
            return false;
        }
        
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
        if (frag == 0) {
            LOG_ERROR(LogCategory::RENDERING, "Failed to create fragment shader");
            glDeleteShader(vert);
            return false;
        }
        
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
        if (shaderProgram == 0) {
            LOG_ERROR(LogCategory::RENDERING, "Failed to create shader program");
            glDeleteShader(vert);
            glDeleteShader(frag);
            return false;
        }
        
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
            shaderProgram = 0;
            return false;
        }
        
        glDeleteShader(vert);
        glDeleteShader(frag);
        
        LOG_INFO(LogCategory::RENDERING, "Enhanced God View shaders loaded successfully");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(LogCategory::RENDERING, "Exception during shader loading: " + std::string(e.what()));
        return false;
    } catch (...) {
        LOG_ERROR(LogCategory::RENDERING, "Unknown exception during shader loading");
        return false;
    }
}

// Check if mesh needs to be updated
bool GodViewDebugTool::needsMeshUpdate() const {
    return meshDirty;
}

float GodViewDebugTool::generateHeight(const glm::vec3& pos) {
    // Safety check for zero vector
    if (glm::length(pos) < 0.001f) {
        return 0.0f;
    }
    
    // Create a normalized direction to get the position on the unit sphere
    glm::dvec3 direction = glm::normalize(glm::dvec3(pos.x, pos.y, pos.z));
    
    // Use the enhanced sampling method that combines procedural and actual data
    double heightVariation = sampleHeight(direction);
    
    // Return height normalized to a reasonable range for visualization
    // We divide by 1000 to get a value in the [0,1] range (assuming max height is 1000m)
    return static_cast<float>(heightVariation / 1000.0f);
}

double GodViewDebugTool::sampleHeight(const glm::dvec3& direction) {
    // Convert 3D direction to a 2D key for caching
    // We use lat/lon type coordinates derived from the direction
    int resolution = 1000; // Higher = more detailed cache
    glm::ivec2 cacheKey(
        static_cast<int>(atan2(direction.z, direction.x) * resolution),
        static_cast<int>(asin(direction.y) * resolution)
    );
    
    // Check cache first
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        auto it = heightSampleCache.find(cacheKey);
        if (it != heightSampleCache.end()) {
            // If cache entry is recent enough, use it
            if (it->second.isActual || (visualizationMode == VisualizationMode::PROCEDURAL)) {
                return it->second.height;
            }
        }
    }
    
    // Sample based on visualization mode
    double height = 0.0;
    bool isActual = false;
    
    switch (visualizationMode) {
        case VisualizationMode::ACTUAL:
            height = sampleFromChunks(direction);
            isActual = true;
            break;
            
        case VisualizationMode::PROCEDURAL:
            height = sampleProcedural(direction);
            isActual = false;
            break;
            
        case VisualizationMode::HYBRID:
            // Try actual first, fall back to procedural
            height = sampleFromChunks(direction);
            if (height != 0.0) {
                isActual = true;
            } else {
                height = sampleProcedural(direction);
                isActual = false;
            }
            break;
    }
    
    // Cache the result
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        heightSampleCache[cacheKey] = {height, isActual, glfwGetTime()};
    }
    
    return height;
}

double GodViewDebugTool::sampleFromChunks(const glm::dvec3& direction) {
    // Scale to the actual planet radius to get the position in world space
    double surfaceR = SphereUtils::getSurfaceRadiusMeters();
    glm::dvec3 surfacePos = direction * surfaceR;
    
    // Perform raycast from far above toward center to find first solid block
    glm::dvec3 rayStart = direction * (surfaceR + 1000.0); // Start 1km above surface
    glm::dvec3 rayEnd = direction * (surfaceR - 1000.0);   // End 1km below surface
    
    // Calculate ray direction and length
    glm::dvec3 rayDir = glm::normalize(rayEnd - rayStart);
    double rayLength = glm::distance(rayStart, rayEnd);
    
    // Sample along ray at intervals
    const int samples = 100;
    for (int i = 0; i < samples; i++) {
        double t = static_cast<double>(i) / samples;
        glm::dvec3 samplePos = rayStart + rayDir * (t * rayLength);
        
        // Convert to int coordinates
        int x = static_cast<int>(floor(samplePos.x));
        int y = static_cast<int>(floor(samplePos.y));
        int z = static_cast<int>(floor(samplePos.z));
        
        // Get block at this position
        Block block = world.getBlock(x, y, z);
        
        // If we hit a non-air block, calculate height above/below surface
        if (block.type != BlockType::AIR) {
            double distFromCenter = glm::length(samplePos);
            return distFromCenter - surfaceR; // Height relative to surface
        }
    }
    
    // If no solid block found, return 0
    return 0.0;
}

double GodViewDebugTool::sampleProcedural(const glm::dvec3& direction) {
    // Scale to the actual planet radius to get the position in world space
    double surfaceR = SphereUtils::getSurfaceRadiusMeters();
    glm::dvec3 worldPos = direction * surfaceR;
    
    // Use SphereUtils to get procedural height at this position
    try {
        return SphereUtils::getHeightVariation(worldPos);
    } catch (...) {
        // Handle any exceptions during height calculation
        return 0.0;
    }
}

bool GodViewDebugTool::generateGlobeMesh() {
    if (useAdaptiveResolution) {
        generateAdaptiveMesh();
        return true;
    }
    
    try {
        // Create a higher-resolution sphere to represent the globe
        // We'll subdivide an icosahedron for better uniformity than a UV sphere
        
        // Resolution of the globe mesh - lower for better stability
        const int resolution = 4; // Reduced from 6 for stability
        
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
            
            // Height - use a safe implementation
            float safeHeight = 0.0f;
            try {
                safeHeight = height;
                // Clamp height to reasonable range to avoid rendering issues
                safeHeight = std::max(0.0f, std::min(safeHeight, 1.0f));
            } catch (...) {
                // If anything goes wrong with height, use default
                safeHeight = 0.0f;
            }
            
            // Add height value
            vertices.push_back(safeHeight);
            
            vertexMap[key] = index;
            return index;
        };
        
        // Function to subdivide a triangle
        std::function<void(const glm::vec3&, const glm::vec3&, const glm::vec3&, int)> subdivide;
        subdivide = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, int depth) {
            if (depth == 0) {
                // Generate height values based on position
                float heightA = 0.0f;
                float heightB = 0.0f;
                float heightC = 0.0f;
                
                // Use try/catch for each height calculation
                try {
                    heightA = generateHeight(a);
                } catch (...) {}
                
                try {
                    heightB = generateHeight(b);
                } catch (...) {}
                
                try {
                    heightC = generateHeight(c);
                } catch (...) {}
                
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
        
        // Limit mesh size to prevent memory issues
        const size_t MAX_VERTICES = 100000;
        const size_t MAX_INDICES = 300000;
        
        if (vertices.size() > MAX_VERTICES) {
            LOG_WARNING(LogCategory::RENDERING, "Truncating globe mesh vertices: " + 
                std::to_string(vertices.size() / 7) + " to " + std::to_string(MAX_VERTICES / 7));
            vertices.resize(MAX_VERTICES);
        }
        
        if (indices.size() > MAX_INDICES) {
            LOG_WARNING(LogCategory::RENDERING, "Truncating globe mesh indices: " + 
                std::to_string(indices.size()) + " to " + std::to_string(MAX_INDICES));
            indices.resize(MAX_INDICES);
        }
        
        // Upload to GPU - with proper error checking
        glBindVertexArray(vao);
        if (glGetError() != GL_NO_ERROR) {
            LOG_ERROR(LogCategory::RENDERING, "Error binding VAO for globe mesh");
            return false;
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        if (glGetError() != GL_NO_ERROR) {
            LOG_ERROR(LogCategory::RENDERING, "Error binding VBO for globe mesh");
            return false;
        }
        
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        if (glGetError() != GL_NO_ERROR) {
            LOG_ERROR(LogCategory::RENDERING, "Error uploading vertex data for globe mesh");
            return false;
        }
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        if (glGetError() != GL_NO_ERROR) {
            LOG_ERROR(LogCategory::RENDERING, "Error binding EBO for globe mesh");
            return false;
        }
        
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        if (glGetError() != GL_NO_ERROR) {
            LOG_ERROR(LogCategory::RENDERING, "Error uploading index data for globe mesh");
            return false;
        }
        
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
        
        // Mark the mesh as clean
        meshDirty = false;
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(LogCategory::RENDERING, "Exception in globe mesh generation: " + std::string(e.what()));
        try {
            // Create a minimal fallback sphere
            createFallbackSphere();
            return true;
        } catch (...) {
            LOG_ERROR(LogCategory::RENDERING, "Failed to create fallback sphere");
            return false;
        }
    } catch (...) {
        LOG_ERROR(LogCategory::RENDERING, "Unknown exception in globe mesh generation");
        try {
            // Create a minimal fallback sphere
            createFallbackSphere();
            return true;
        } catch (...) {
            LOG_ERROR(LogCategory::RENDERING, "Failed to create fallback sphere");
            return false;
        }
    }
}

void GodViewDebugTool::generateAdaptiveMesh() {
    try {
        // Create a higher-resolution sphere with adaptive detail
        // We'll subdivide an icosahedron for better uniformity than a UV sphere
        
        // Base resolution of the globe mesh
        const int baseResolution = 4; // Base level subdivisions
        
        // Calculate detail direction - the direction the camera is looking at
        glm::dvec3 detailDir = glm::normalize(glm::dvec3(cameraTarget) - glm::dvec3(cameraPosition));
        
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
            
            // Height - use a safe implementation
            float safeHeight = 0.0f;
            try {
                safeHeight = height;
                // Clamp height to reasonable range to avoid rendering issues
                safeHeight = std::max(0.0f, std::min(safeHeight, 1.0f));
            } catch (...) {
                // If anything goes wrong with height, use default
                safeHeight = 0.0f;
            }
            
            // Add height value
            vertices.push_back(safeHeight);
            
            vertexMap[key] = index;
            return index;
        };
        
        // Function to determine subdivision level based on angle to detail direction
        auto getSubdivisionLevel = [&](const glm::vec3& faceNormal) -> int {
            // Convert to double precision for better accuracy
            glm::dvec3 normal(faceNormal.x, faceNormal.y, faceNormal.z);
            
            // Calculate angle between face normal and detail direction
            double angle = acos(glm::dot(normal, detailDir));
            
            // Map angle to subdivision level
            // 0 degrees = facing camera = max detail
            // 180 degrees = opposite side = min detail
            double angleFactor = angle / 3.14159265358979323846;
            
            // Base resolution + additional detail where needed
            int level = baseResolution;
            
            // Add more subdivisions for faces closer to detail direction
            if (angleFactor < 0.2) level += 2; // Very close to detail direction
            else if (angleFactor < 0.4) level += 1; // Somewhat close to detail direction
            
            // Apply adaptive detail factor
            level = static_cast<int>(level * adaptiveDetailFactor);
            
            // Ensure minimum and maximum levels
            return std::max(2, std::min(level, 6));
        };
        
        // Function to subdivide a triangle
        std::function<void(const glm::vec3&, const glm::vec3&, const glm::vec3&, int)> subdivide;
        subdivide = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, int depth) {
            if (depth == 0) {
                // Generate height values based on position
                float heightA = 0.0f;
                float heightB = 0.0f;
                float heightC = 0.0f;
                
                // Use try/catch for each height calculation
                try {
                    heightA = generateHeight(a);
                } catch (...) {}
                
                try {
                    heightB = generateHeight(b);
                } catch (...) {}
                
                try {
                    heightC = generateHeight(c);
                } catch (...) {}
                
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
        
        // Subdivide all faces of the icosahedron with adaptive resolution
        for (const auto& face : faces) {
            glm::vec3 a = positions[face[0]];
            glm::vec3 b = positions[face[1]];
            glm::vec3 c = positions[face[2]];
            
            // Calculate face normal (pointing outward from center)
            glm::vec3 faceNormal = glm::normalize(a + b + c);
            
            // Determine subdivision level based on view direction
            int subdivLevel = getSubdivisionLevel(faceNormal);
            
            // Subdivide this face
            subdivide(a, b, c, subdivLevel);
        }
        
        // Verify we have vertices and indices
        if (vertices.empty() || indices.empty()) {
            LOG_ERROR(LogCategory::RENDERING, "Generated empty adaptive globe mesh");
            return;
        }
        
        // Limit mesh size to prevent memory issues
        const size_t MAX_VERTICES = 200000;
        const size_t MAX_INDICES = 600000;
        
        if (vertices.size() > MAX_VERTICES) {
            LOG_WARNING(LogCategory::RENDERING, "Truncating adaptive globe mesh vertices: " + 
                std::to_string(vertices.size() / 7) + " to " + std::to_string(MAX_VERTICES / 7));
            vertices.resize(MAX_VERTICES);
        }
        
        if (indices.size() > MAX_INDICES) {
            LOG_WARNING(LogCategory::RENDERING, "Truncating adaptive globe mesh indices: " + 
                std::to_string(indices.size()) + " to " + std::to_string(MAX_INDICES));
            indices.resize(MAX_INDICES);
        }
        
        // Upload to GPU - with proper error checking
        glBindVertexArray(vao);
        if (glGetError() != GL_NO_ERROR) {
            LOG_ERROR(LogCategory::RENDERING, "Error binding VAO for adaptive globe mesh");
            return;
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        if (glGetError() != GL_NO_ERROR) {
            LOG_ERROR(LogCategory::RENDERING, "Error binding VBO for adaptive globe mesh");
            return;
        }
        
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        if (glGetError() != GL_NO_ERROR) {
            LOG_ERROR(LogCategory::RENDERING, "Error uploading vertex data for adaptive globe mesh");
            return;
        }
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        if (glGetError() != GL_NO_ERROR) {
            LOG_ERROR(LogCategory::RENDERING, "Error binding EBO for adaptive globe mesh");
            return;
        }
        
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        if (glGetError() != GL_NO_ERROR) {
            LOG_ERROR(LogCategory::RENDERING, "Error uploading index data for adaptive globe mesh");
            return;
        }
        
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
        
        LOG_INFO(LogCategory::RENDERING, "Adaptive God View globe mesh generated with " + 
            std::to_string(vertices.size() / 7) + " vertices and " + 
            std::to_string(indices.size() / 3) + " triangles");
            
        // Mark the mesh as clean
        meshDirty = false;
    } catch (const std::exception& e) {
        LOG_ERROR(LogCategory::RENDERING, "Exception in adaptive globe mesh generation: " + std::string(e.what()));
        try {
            // Create a minimal fallback sphere
            createFallbackSphere();
        } catch (...) {
            LOG_ERROR(LogCategory::RENDERING, "Failed to create fallback sphere");
        }
    } catch (...) {
        LOG_ERROR(LogCategory::RENDERING, "Unknown exception in adaptive globe mesh generation");
        try {
            // Create a minimal fallback sphere
            createFallbackSphere();
        } catch (...) {
            LOG_ERROR(LogCategory::RENDERING, "Failed to create fallback sphere");
        }
    }
}

// Create a simple fallback sphere with minimal detail if main generation fails
void GodViewDebugTool::createFallbackSphere() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Create a very simple sphere with just 12 points (an icosahedron)
    const float X = 0.525731f;
    const float Z = 0.850651f;
    
    // Basic vertices
    std::vector<glm::vec3> positions = {
        {-X, 0.0f, Z}, {X, 0.0f, Z}, {-X, 0.0f, -Z}, {X, 0.0f, -Z},
        {0.0f, Z, X}, {0.0f, Z, -X}, {0.0f, -Z, X}, {0.0f, -Z, -X},
        {Z, X, 0.0f}, {-Z, X, 0.0f}, {Z, -X, 0.0f}, {-Z, -X, 0.0f}
    };
    
    // Simple indices for triangles
    indices = {
        0, 4, 1, 0, 9, 4, 9, 5, 4, 4, 5, 8, 4, 8, 1,
        8, 10, 1, 8, 3, 10, 5, 3, 8, 5, 2, 3, 2, 7, 3,
        7, 10, 3, 7, 6, 10, 7, 11, 6, 11, 0, 6, 0, 1, 6,
        6, 1, 10, 9, 0, 11, 9, 11, 2, 9, 2, 5, 7, 2, 11
    };
    
    // Create vertex data
    for (const auto& pos : positions) {
        // Position
        vertices.push_back(pos.x);
        vertices.push_back(pos.y);
        vertices.push_back(pos.z);
        
        // Normal
        vertices.push_back(pos.x);
        vertices.push_back(pos.y);
        vertices.push_back(pos.z);
        
        // Height (constant 0.0 for fallback)
        vertices.push_back(0.0f);
    }
    
    // Upload to GPU
    glBindVertexArray(vao);
    if (glGetError() != GL_NO_ERROR) {
        LOG_ERROR(LogCategory::RENDERING, "Error binding VAO for fallback sphere");
        return;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    if (glGetError() != GL_NO_ERROR) {
        LOG_ERROR(LogCategory::RENDERING, "Error binding VBO for fallback sphere");
        return;
    }
    
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    if (glGetError() != GL_NO_ERROR) {
        LOG_ERROR(LogCategory::RENDERING, "Error uploading vertex data for fallback sphere");
        return;
    }
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    if (glGetError() != GL_NO_ERROR) {
        LOG_ERROR(LogCategory::RENDERING, "Error binding EBO for fallback sphere");
        return;
    }
    
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    if (glGetError() != GL_NO_ERROR) {
        LOG_ERROR(LogCategory::RENDERING, "Error uploading index data for fallback sphere");
        return;
    }
    
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
    
    LOG_INFO(LogCategory::RENDERING, "Created fallback sphere for God View");
    
    // Mark mesh as clean after fallback
    meshDirty = false;
}

void GodViewDebugTool::updateHeightData() {
    // If an update is already in progress, don't start another
    if (updateInProgress.load()) {
        return;
    }
    
    // Start an async update
    updateHeightDataAsync();
}

void GodViewDebugTool::updateHeightDataAsync() {
    // If an update is already in progress, don't start another
    if (updateInProgress.exchange(true)) {
        return;
    }
    
    // Clear any existing future
    if (updateFuture.valid()) {
        try {
            updateFuture.wait();
        } catch (...) {
            // Ignore any exceptions from previous update
        }
    }
    
    // Start a new update in the background
    updateFuture = std::async(std::launch::async, [this]() {
        try {
            // Clear the cache if it's getting too large
            {
                std::lock_guard<std::mutex> lock(cacheMutex);
                if (heightSampleCache.size() > 10000) {
                    LOG_INFO(LogCategory::RENDERING, "Clearing height cache (size: " + 
                             std::to_string(heightSampleCache.size()) + ")");
                    heightSampleCache.clear();
                }
            }
            
            // Need to regenerate mesh with updated height data
            if (needsMeshUpdate()) {
                // Signal main thread to regenerate mesh
                meshDirty = true;
            }
        } catch (const std::exception& e) {
            LOG_ERROR(LogCategory::RENDERING, "Exception in height data update: " + std::string(e.what()));
        } catch (...) {
            LOG_ERROR(LogCategory::RENDERING, "Unknown exception in height data update");
        }
        
        // Mark update as complete
        updateInProgress.store(false);
    });
}

void GodViewDebugTool::clearHeightCache() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    heightSampleCache.clear();
    LOG_INFO(LogCategory::RENDERING, "Height cache cleared");
    meshDirty = true;
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
    
    // Check if mesh needs update (e.g., after camera movement)
    static glm::vec3 lastCamPos = cameraPosition;
    static glm::vec3 lastCamTarget = cameraTarget;
    
    if (glm::distance(lastCamPos, cameraPosition) > 1000.0f || 
        glm::distance(lastCamTarget, cameraTarget) > 1000.0f) {
        // Camera moved significantly, might need to update adaptive mesh
        if (useAdaptiveResolution) {
            meshDirty = true;
        }
        
        lastCamPos = cameraPosition;
        lastCamTarget = cameraTarget;
    }
    
    // Check if we need to update the mesh
    if (meshDirty) {
        generateGlobeMesh();
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
    
    // Set visualization mode (for shader coloring)
    GLint vizModeLoc = glGetUniformLocation(shaderProgram, "visualizationMode");
    if (vizModeLoc >= 0) glUniform1i(vizModeLoc, static_cast<int>(visualizationMode));
    
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
    
    // Update height data in the background if needed
    if (!updateInProgress.load()) {
        updateHeightDataAsync();
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

void GodViewDebugTool::setVisualizationMode(VisualizationMode mode) {
    visualizationMode = mode;
    
    LOG_INFO(LogCategory::RENDERING, "Visualization mode set to: " + 
             std::string(mode == VisualizationMode::PROCEDURAL ? "Procedural" : 
                        (mode == VisualizationMode::ACTUAL ? "Actual" : "Hybrid")));
                        
    // Clear the cache and regenerate mesh when mode changes
    clearHeightCache();
    
    // Mark mesh as dirty to trigger regeneration
    meshDirty = true;
}

void GodViewDebugTool::setAdaptiveResolution(bool enabled) {
    if (useAdaptiveResolution != enabled) {
        useAdaptiveResolution = enabled;
        LOG_INFO(LogCategory::RENDERING, std::string("Adaptive resolution ") + 
                 (enabled ? "enabled" : "disabled"));
        
        // Mark mesh as dirty to trigger regeneration
        meshDirty = true;
    }
}

void GodViewDebugTool::setAdaptiveDetailFactor(float factor) {
    adaptiveDetailFactor = std::max(0.5f, std::min(factor, 2.0f));
    LOG_INFO(LogCategory::RENDERING, "Adaptive detail factor set to: " + 
             std::to_string(adaptiveDetailFactor));
    
    // Mark mesh as dirty to trigger regeneration with new detail factor
    if (useAdaptiveResolution) {
        meshDirty = true;
    }
}

void GodViewDebugTool::setActive(bool enabled) {
    active = enabled;
}

float GodViewDebugTool::getCurrentRotation() const {
    return rotationAngle;
}