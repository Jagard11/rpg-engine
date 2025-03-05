// ./include/Graphics/ShaderManager.hpp
#ifndef SHADER_MANAGER_HPP
#define SHADER_MANAGER_HPP

#include <GL/glew.h>
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

/**
 * Centralized shader management system
 * Handles loading, compiling, and caching shader programs
 */
class ShaderManager {
public:
    // Singleton pattern
    static ShaderManager& getInstance() {
        static ShaderManager instance;
        return instance;
    }

    // Initialize standard shaders
    void initializeShaders() {
        loadDefaultShader();
        loadEdgeShader();
        loadFrustumShader();
        loadHighlightShader();
    }

    // Load shader from source strings
    GLuint loadShaderFromSource(const std::string& vertexSource, const std::string& fragmentSource, const std::string& name) {
        GLuint programId = compileShaderProgram(vertexSource, fragmentSource);
        shaderPrograms[name] = programId;
        return programId;
    }

    // Load shader from files
    GLuint loadShaderFromFiles(const std::string& vertexPath, const std::string& fragmentPath, const std::string& name) {
        std::string vertexSource = readFile(vertexPath);
        std::string fragmentSource = readFile(fragmentPath);
        return loadShaderFromSource(vertexSource, fragmentSource, name);
    }

    // Get a shader by name
    GLuint getShader(const std::string& name) {
        auto it = shaderPrograms.find(name);
        if (it != shaderPrograms.end()) {
            return it->second;
        }
        std::cerr << "Warning: Shader '" << name << "' not found, using default shader" << std::endl;
        return defaultShaderProgram;
    }

    // Set uniform value for a shader
    void setUniform(GLuint program, const std::string& name, int value) {
        glUniform1i(glGetUniformLocation(program, name.c_str()), value);
    }

    void setUniform(GLuint program, const std::string& name, float value) {
        glUniform1f(glGetUniformLocation(program, name.c_str()), value);
    }

    void setUniform(GLuint program, const std::string& name, const glm::vec3& value) {
        glUniform3fv(glGetUniformLocation(program, name.c_str()), 1, glm::value_ptr(value));
    }

    void setUniform(GLuint program, const std::string& name, const glm::vec4& value) {
        glUniform4fv(glGetUniformLocation(program, name.c_str()), 1, glm::value_ptr(value));
    }

    void setUniform(GLuint program, const std::string& name, const glm::mat4& value) {
        glUniformMatrix4fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
    }

    // Cleanup function
    void cleanup() {
        for (auto& pair : shaderPrograms) {
            glDeleteProgram(pair.second);
        }
        shaderPrograms.clear();
    }

    // Accessor methods for standard shaders
    GLuint getDefaultShader() const { return defaultShaderProgram; }
    GLuint getEdgeShader() const { return edgeShaderProgram; }
    GLuint getFrustumShader() const { return frustumShaderProgram; }
    GLuint getHighlightShader() const { return highlightShaderProgram; }

private:
    // Private constructor for singleton
    ShaderManager() 
        : defaultShaderProgram(0),
          edgeShaderProgram(0),
          frustumShaderProgram(0),
          highlightShaderProgram(0) {}

    // Prevent copying
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;

    // Read file content to string
    std::string readFile(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file) {
            std::cerr << "Failed to open file: " << filePath << std::endl;
            return "";
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    // Compile shader from source
    GLuint compileShader(const std::string& source, GLenum type) {
        GLuint shader = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLchar infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Shader compilation error (" 
                      << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") 
                      << "): " << infoLog << std::endl;
        }

        return shader;
    }

    // Compile full shader program
    GLuint compileShaderProgram(const std::string& vertexSource, const std::string& fragmentSource) {
        GLuint vertex = compileShader(vertexSource, GL_VERTEX_SHADER);
        GLuint fragment = compileShader(fragmentSource, GL_FRAGMENT_SHADER);
        
        GLuint program = glCreateProgram();
        glAttachShader(program, vertex);
        glAttachShader(program, fragment);
        glLinkProgram(program);
        
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            GLchar infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            std::cerr << "Shader program linking error: " << infoLog << std::endl;
        }
        
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        
        return program;
    }

    // Default shader for standard rendering
    void loadDefaultShader() {
        const char* vertSrc = R"(
            #version 330 core
            layout(location = 0) in vec3 pos;
            layout(location = 1) in vec2 uv;
            out vec2 TexCoord;
            uniform mat4 model, view, proj;
            uniform vec3 localOriginOffset;
            
            void main() {
                // Apply model transform and origin offset
                vec3 worldPos = (model * vec4(pos, 1.0)).xyz;
                vec3 localPos = worldPos - localOriginOffset;
                
                // Standard transformation pipeline
                gl_Position = proj * view * vec4(localPos, 1.0);
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

        GLuint vert = compileShader(vertSrc, GL_VERTEX_SHADER);
        GLuint frag = compileShader(fragSrc, GL_FRAGMENT_SHADER);
        
        defaultShaderProgram = glCreateProgram();
        glAttachShader(defaultShaderProgram, vert);
        glAttachShader(defaultShaderProgram, frag);
        glLinkProgram(defaultShaderProgram);
        
        GLint success;
        GLchar infoLog[512];
        glGetProgramiv(defaultShaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(defaultShaderProgram, 512, NULL, infoLog);
            std::cerr << "Default Shader Program Link Error: " << infoLog << std::endl;
        }
        
        glDeleteShader(vert);
        glDeleteShader(frag);
        
        shaderPrograms["default"] = defaultShaderProgram;
    }

    // Edge shader for rendering block outlines
    void loadEdgeShader() {
        const char* vertSrc = R"(
            #version 330 core
            layout(location = 0) in vec3 pos;
            uniform mat4 view, proj;
            uniform vec3 localOriginOffset;
            
            void main() {
                // Apply the local origin offset for better precision
                vec3 worldPos = pos - localOriginOffset;
                gl_Position = proj * view * vec4(worldPos, 1.0);
            }
        )";
        
        const char* fragSrc = R"(
            #version 330 core
            out vec4 FragColor;
            uniform vec4 edgeColor;
            
            void main() {
                FragColor = edgeColor;
            }
        )";
        
        GLuint vert = compileShader(vertSrc, GL_VERTEX_SHADER);
        GLuint frag = compileShader(fragSrc, GL_FRAGMENT_SHADER);
        
        edgeShaderProgram = glCreateProgram();
        glAttachShader(edgeShaderProgram, vert);
        glAttachShader(edgeShaderProgram, frag);
        glLinkProgram(edgeShaderProgram);
        
        GLint success;
        GLchar infoLog[512];
        glGetProgramiv(edgeShaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(edgeShaderProgram, 512, NULL, infoLog);
            std::cerr << "Edge Shader Program Link Error: " << infoLog << std::endl;
        }
        
        glDeleteShader(vert);
        glDeleteShader(frag);
        
        shaderPrograms["edge"] = edgeShaderProgram;
    }

    // Frustum shader for Earth-scale voxel rendering
    void loadFrustumShader() {
        const char* vertSrc = R"(
            #version 330 core
            
            // Input vertex attributes
            layout(location = 0) in vec3 blockPos;      // Block position in chunk
            layout(location = 1) in vec2 texCoord;      // Texture coordinates
            layout(location = 2) in int faceIndex;      // Which face of the block (0-5)
            layout(location = 3) in int blockType;      // Type of block (for texture)
            
            // Output to fragment shader
            out vec2 fragTexCoord;
            out float fragBlockType;
            
            // Uniform variables
            uniform mat4 model;             // Model matrix for the chunk
            uniform mat4 view;              // View matrix
            uniform mat4 proj;              // Projection matrix
            uniform vec3 chunkOffset;       // Chunk position in world
            uniform vec3 originOffset;      // Origin rebasing offset
            uniform vec3 playerPos;         // Player's absolute position
            uniform float planetRadius;     // Radius of the planet in meters
            
            // Constants
            const float VOXEL_SIZE = 1.0;  // Base size of a voxel at surface
            
            // Outputs vertex data for a frustum face given block data
            vec3 calculateFrustumVertex(vec3 localBlockPos, int vertexIndex, int face) {
                // Convert local position to world position
                vec3 worldPos = localBlockPos + chunkOffset - originOffset;
                
                // Calculate block center
                vec3 blockCenter = worldPos + vec3(0.5);
                
                // Get direction from planet center to block center
                // Since originOffset centers around player, we can use
                // player position to get the exact direction from center
                vec3 dirToCenter = normalize(playerPos);
                vec3 dirFromCenter = -dirToCenter;
                
                // Distance from center (more accurate to use player as reference)
                float playerDistFromCenter = length(playerPos);
                float blockDistEst = playerDistFromCenter + dot(worldPos, dirFromCenter);
                
                // Calculate tapering factor based on distance from center
                // The closer to center, the more the block tapers
                float distRatio = blockDistEst / planetRadius;
                float taperFactor = (blockDistEst - VOXEL_SIZE) / blockDistEst;
                
                // Ensure tapering factor is reasonable
                taperFactor = max(0.5, min(1.0, taperFactor));
                
                // Calculate orthogonal axes for the block faces
                // This ensures the frustum is oriented correctly relative to planet center
                vec3 up = normalize(blockCenter);
                vec3 right = normalize(cross(
                    abs(up.y) > 0.99 ? vec3(1,0,0) : vec3(0,1,0), 
                    up
                ));
                vec3 forward = normalize(cross(right, up));
                
                // Vertex offsets for a unit cube (will be transformed into frustum)
                vec3 vertexOffset;
                float topSize = VOXEL_SIZE;
                float bottomSize = VOXEL_SIZE * taperFactor;
                
                // Determine vertex position based on vertex index and face
                // The 8 vertices of a cube, adjusted for frustum shape
                switch(vertexIndex) {
                    // Top face vertices (further from center)
                    case 0: vertexOffset = up * 0.5 - right * (topSize/2) - forward * (topSize/2); break;
                    case 1: vertexOffset = up * 0.5 + right * (topSize/2) - forward * (topSize/2); break;
                    case 2: vertexOffset = up * 0.5 + right * (topSize/2) + forward * (topSize/2); break;
                    case 3: vertexOffset = up * 0.5 - right * (topSize/2) + forward * (topSize/2); break;
                    
                    // Bottom face vertices (closer to center)
                    case 4: vertexOffset = -up * 0.5 - right * (bottomSize/2) - forward * (bottomSize/2); break;
                    case 5: vertexOffset = -up * 0.5 + right * (bottomSize/2) - forward * (bottomSize/2); break;
                    case 6: vertexOffset = -up * 0.5 + right * (bottomSize/2) + forward * (bottomSize/2); break;
                    case 7: vertexOffset = -up * 0.5 - right * (bottomSize/2) + forward * (bottomSize/2); break;
                }
                
                // Apply the offset to the block center
                return blockCenter + vertexOffset;
            }
            
            void main() {
                // Determine which vertices to use based on the face
                int v1, v2, v3, v4;
                switch(faceIndex) {
                    case 0: // +X face
                        v1 = 1; v2 = 2; v3 = 6; v4 = 5;
                        break;
                    case 1: // -X face
                        v1 = 0; v2 = 4; v3 = 7; v4 = 3;
                        break;
                    case 2: // +Y face (top)
                        v1 = 0; v2 = 1; v3 = 2; v4 = 3;
                        break;
                    case 3: // -Y face (bottom)
                        v1 = 4; v2 = 5; v3 = 6; v4 = 7;
                        break;
                    case 4: // +Z face
                        v1 = 3; v2 = 2; v3 = 6; v4 = 7;
                        break;
                    case 5: // -Z face
                        v1 = 0; v2 = 1; v3 = 5; v4 = 4;
                        break;
                }
                
                // Determine which vertex of the face we're rendering
                int vertexIndex;
                if (gl_VertexID % 4 == 0) vertexIndex = v1;
                else if (gl_VertexID % 4 == 1) vertexIndex = v2;
                else if (gl_VertexID % 4 == 2) vertexIndex = v3;
                else vertexIndex = v4;
                
                // Calculate the frustum vertex position
                vec3 worldPos = calculateFrustumVertex(blockPos, vertexIndex, faceIndex);
                
                // Apply transformations
                gl_Position = proj * view * vec4(worldPos, 1.0);
                
                // Pass texture coordinates to fragment shader
                fragTexCoord = texCoord;
                fragBlockType = float(blockType);
            }
        )";
        
        const char* fragSrc = R"(
            #version 330 core
            
            in vec2 fragTexCoord;
            in float fragBlockType;
            
            out vec4 FragColor;
            
            uniform sampler2D texAtlas;
            uniform bool useFaceColors;
            
            void main() {
                if (useFaceColors) {
                    // Debug mode with face colors
                    int type = int(fragBlockType);
                    vec4 color;
                    
                    if (type == 1) color = vec4(0.5, 0.3, 0.0, 1.0); // Dirt
                    else if (type == 2) color = vec4(0.0, 0.8, 0.1, 1.0); // Grass
                    else color = vec4(0.8, 0.8, 0.8, 1.0); // Default
                    
                    FragColor = color;
                } else {
                    // Calculate UV coordinates in atlas based on block type
                    int type = int(fragBlockType);
                    float atlas_x = mod(float(type), 4.0) * 0.25;
                    float atlas_y = floor(float(type) / 4.0) * 0.25;
                    
                    // Get final texture coordinates
                    vec2 atlasCoord = vec2(
                        atlas_x + fragTexCoord.x * 0.25,
                        atlas_y + fragTexCoord.y * 0.25
                    );
                    
                    FragColor = texture(texAtlas, atlasCoord);
                }
            }
        )";
        
        GLuint vert = compileShader(vertSrc, GL_VERTEX_SHADER);
        GLuint frag = compileShader(fragSrc, GL_FRAGMENT_SHADER);
        
        frustumShaderProgram = glCreateProgram();
        glAttachShader(frustumShaderProgram, vert);
        glAttachShader(frustumShaderProgram, frag);
        glLinkProgram(frustumShaderProgram);
        
        GLint success;
        GLchar infoLog[512];
        glGetProgramiv(frustumShaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(frustumShaderProgram, 512, NULL, infoLog);
            std::cerr << "Frustum Shader Program Link Error: " << infoLog << std::endl;
        }
        
        glDeleteShader(vert);
        glDeleteShader(frag);
        
        shaderPrograms["frustum"] = frustumShaderProgram;
    }

    // Highlight shader for selected blocks
    void loadHighlightShader() {
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
            uniform vec4 highlightColor;
            void main() {
                FragColor = highlightColor;
            }
        )";
        
        GLuint vert = compileShader(vertSrc, GL_VERTEX_SHADER);
        GLuint frag = compileShader(fragSrc, GL_FRAGMENT_SHADER);
        
        highlightShaderProgram = glCreateProgram();
        glAttachShader(highlightShaderProgram, vert);
        glAttachShader(highlightShaderProgram, frag);
        glLinkProgram(highlightShaderProgram);
        
        GLint success;
        GLchar infoLog[512];
        glGetProgramiv(highlightShaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(highlightShaderProgram, 512, NULL, infoLog);
            std::cerr << "Highlight Shader Program Link Error: " << infoLog << std::endl;
        }
        
        glDeleteShader(vert);
        glDeleteShader(frag);
        
        shaderPrograms["highlight"] = highlightShaderProgram;
    }

    // Shader program storage
    std::unordered_map<std::string, GLuint> shaderPrograms;

    // Standard shader program IDs
    GLuint defaultShaderProgram;
    GLuint edgeShaderProgram;
    GLuint frustumShaderProgram;
    GLuint highlightShaderProgram;
};

#endif // SHADER_MANAGER_HPP