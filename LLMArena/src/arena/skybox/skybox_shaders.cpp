// src/voxel/sky_system_shaders.cpp
#include "../../include/voxel/sky_system.h"
#include <QDebug>

void SkySystem::createShaders() {
    // Create skybox shader program
    m_skyboxShader = new QOpenGLShaderProgram();
    
    // Simple screen-space vertex shader
    const char* skyboxVertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        
        out vec2 vertPosition;
        
        void main() {
            vertPosition = position.xy;
            gl_Position = vec4(position, 1.0);
        }
    )";
    
    // Simple skybox fragment shader with constant color
    const char* skyboxFragmentShaderSource = R"(
        #version 330 core
        in vec2 vertPosition;
        
        uniform vec3 skyColor;
        
        out vec4 fragColor;
        
        void main() {
            // Simple gradient from skyColor to slightly lighter at horizon
            float height = (vertPosition.y + 1.0) * 0.5; // Normalize y to 0-1
            vec3 finalColor = mix(skyColor * 1.2, skyColor, height);
            
            fragColor = vec4(finalColor, 1.0);
        }
    )";
    
    // Celestial (sun/moon) vertex shader
    const char* celestialVertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec2 texCoord;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        out vec2 fragTexCoord;
        
        void main() {
            gl_Position = projection * view * model * vec4(position, 1.0);
            fragTexCoord = texCoord;
        }
    )";
    
    // Celestial fragment shader with opacity control
    const char* celestialFragmentShaderSource = R"(
        #version 330 core
        in vec2 fragTexCoord;
        
        uniform sampler2D textureSampler;
        uniform float opacity = 1.0; // Default to fully opaque
        
        out vec4 fragColor;
        
        void main() {
            vec4 texColor = texture(textureSampler, fragTexCoord);
            // Apply the opacity uniform to the texture's alpha channel
            fragColor = vec4(texColor.rgb, texColor.a * opacity);
        }
    )";
    
    // Compile and link skybox shader
    if (!m_skyboxShader->addShaderFromSourceCode(QOpenGLShader::Vertex, skyboxVertexShaderSource)) {
        qCritical() << "Failed to compile skybox vertex shader:" << m_skyboxShader->log();
    }
    
    if (!m_skyboxShader->addShaderFromSourceCode(QOpenGLShader::Fragment, skyboxFragmentShaderSource)) {
        qCritical() << "Failed to compile skybox fragment shader:" << m_skyboxShader->log();
    }
    
    if (!m_skyboxShader->link()) {
        qCritical() << "Failed to link skybox shader program:" << m_skyboxShader->log();
    }
    
    // Compile and link celestial shader
    m_celestialShader = new QOpenGLShaderProgram();
    
    if (!m_celestialShader->addShaderFromSourceCode(QOpenGLShader::Vertex, celestialVertexShaderSource)) {
        qCritical() << "Failed to compile celestial vertex shader:" << m_celestialShader->log();
    }
    
    if (!m_celestialShader->addShaderFromSourceCode(QOpenGLShader::Fragment, celestialFragmentShaderSource)) {
        qCritical() << "Failed to compile celestial fragment shader:" << m_celestialShader->log();
    }
    
    if (!m_celestialShader->link()) {
        qCritical() << "Failed to link celestial shader program:" << m_celestialShader->log();
    }
}