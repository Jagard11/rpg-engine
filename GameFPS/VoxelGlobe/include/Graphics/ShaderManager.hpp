// ./include/Graphics/ShaderManager.hpp
#ifndef SHADER_MANAGER_HPP
#define SHADER_MANAGER_HPP

#include <GL/glew.h>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <iostream>

class ShaderManager {
public:
    // Singleton instance accessor
    static ShaderManager& getInstance() {
        static ShaderManager instance;
        return instance;
    }
    
    // Create a shader program from vertex and fragment shader source
    GLuint createShaderProgram(const std::string& vertexSource, const std::string& fragmentSource) {
        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
        
        if (vertexShader == 0 || fragmentShader == 0) {
            std::cerr << "Failed to compile shaders" << std::endl;
            return 0;
        }
        
        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            GLchar infoLog[512];
            glGetProgramInfoLog(program, 512, NULL, infoLog);
            std::cerr << "Shader program linking failed: " << infoLog << std::endl;
            glDeleteProgram(program);
            return 0;
        }
        
        // Clean up shaders
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        
        return program;
    }
    
    // Create named shader program
    GLuint createNamedShaderProgram(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource) {
        GLuint program = createShaderProgram(vertexSource, fragmentSource);
        if (program > 0) {
            shaderPrograms[name] = program;
        }
        return program;
    }
    
    // Get a named shader program
    GLuint getShaderProgram(const std::string& name) {
        auto it = shaderPrograms.find(name);
        if (it != shaderPrograms.end()) {
            return it->second;
        }
        std::cerr << "Shader program '" << name << "' not found" << std::endl;
        return 0;
    }
    
    // Delete all shader programs
    void cleanup() {
        for (auto& pair : shaderPrograms) {
            glDeleteProgram(pair.second);
        }
        shaderPrograms.clear();
    }
    
    // Utility functions for setting uniforms
    void setUniform(GLuint program, const std::string& name, int value) {
        glUniform1i(glGetUniformLocation(program, name.c_str()), value);
    }
    
    void setUniform(GLuint program, const std::string& name, float value) {
        glUniform1f(glGetUniformLocation(program, name.c_str()), value);
    }
    
    void setUniform(GLuint program, const std::string& name, const glm::vec3& value) {
        glUniform3fv(glGetUniformLocation(program, name.c_str()), 1, &value[0]);
    }
    
    void setUniform(GLuint program, const std::string& name, const glm::mat4& value) {
        glUniformMatrix4fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE, &value[0][0]);
    }
    
    // Convenience functions for common shaders
    GLuint createBasicShader() {
        const char* vertexSource = R"(
            #version 330 core
            layout(location = 0) in vec3 pos;
            layout(location = 1) in vec2 uv;
            out vec2 TexCoord;
            uniform mat4 model, view, proj;
            uniform vec3 localOriginOffset;
            
            void main() {
                vec3 worldPos = (model * vec4(pos, 1.0)).xyz - localOriginOffset;
                gl_Position = proj * view * vec4(worldPos, 1.0);
                TexCoord = uv;
            }
        )";
        
        const char* fragmentSource = R"(
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
        
        return createNamedShaderProgram("basic", vertexSource, fragmentSource);
    }
    
    GLuint createEdgeShader() {
        const char* vertexSource = R"(
            #version 330 core
            layout(location = 0) in vec3 pos;
            uniform mat4 view, proj;
            uniform vec3 localOriginOffset;
            
            void main() {
                vec3 worldPos = pos - localOriginOffset;
                gl_Position = proj * view * vec4(worldPos, 1.0);
            }
        )";
        
        const char* fragmentSource = R"(
            #version 330 core
            out vec4 FragColor;
            void main() {
                FragColor = vec4(1.0, 0.0, 0.0, 1.0);
            }
        )";
        
        return createNamedShaderProgram("edge", vertexSource, fragmentSource);
    }
    
    GLuint createHighlightShader() {
        const char* vertexSource = R"(
            #version 330 core
            layout(location = 0) in vec3 pos;
            uniform mat4 model, view, proj;
            void main() {
                gl_Position = proj * view * model * vec4(pos, 1.0);
            }
        )";
        
        const char* fragmentSource = R"(
            #version 330 core
            out vec4 FragColor;
            void main() {
                FragColor = vec4(1.0, 1.0, 1.0, 1.0); // White highlight
            }
        )";
        
        return createNamedShaderProgram("highlight", vertexSource, fragmentSource);
    }

private:
    // Private constructor for singleton
    ShaderManager() {}
    
    // Prevent copying
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;
    
    // Compile individual shader
    GLuint compileShader(GLenum type, const std::string& source) {
        GLuint shader = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLchar infoLog[512];
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cerr << "Shader compilation failed: " << infoLog << std::endl;
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }
    
    // Storage for named shader programs
    std::unordered_map<std::string, GLuint> shaderPrograms;
};

#endif // SHADER_MANAGER_HPP