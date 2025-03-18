// src/rendering/gl_arena/gl_arena_widget_shaders.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include <QDebug>

// Ultra simple shader that just draws textured quads
const char* billboardVertexShaderSource =
    "#version 330 core\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "uniform vec3 position;\n"
    "uniform vec2 size;\n"
    "layout(location = 0) in vec2 vertexPosition;\n"
    "layout(location = 1) in vec2 vertexTexCoord;\n"
    "out vec2 fragTexCoord;\n"
    "void main() {\n"
    "    vec3 worldPos = vec3(vertexPosition.x * size.x, vertexPosition.y * size.y, 0.0) + position;\n"
    "    gl_Position = projection * view * vec4(worldPos, 1.0);\n"
    "    fragTexCoord = vertexTexCoord;\n"
    "}\n";

// Ultra simple fragment shader
const char* billboardFragmentShaderSource =
    "#version 330 core\n"
    "in vec2 fragTexCoord;\n"
    "uniform sampler2D textureSampler;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    fragColor = texture(textureSampler, fragTexCoord);\n"
    "}\n";

bool GLArenaWidget::initShaders()
{
    qDebug() << "Initializing ultra simple billboard shader";
    
    // Clean up existing shader
    if (m_billboardProgram) {
        delete m_billboardProgram;
        m_billboardProgram = nullptr;
    }
    
    // Create billboard shader program
    m_billboardProgram = new QOpenGLShaderProgram(this);
    
    // Compile billboard shader
    qDebug() << "Compiling vertex shader";
    if (!m_billboardProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, billboardVertexShaderSource)) {
        qWarning() << "Failed to compile billboard vertex shader:" << m_billboardProgram->log();
        return false;
    }
    
    qDebug() << "Compiling fragment shader";
    if (!m_billboardProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, billboardFragmentShaderSource)) {
        qWarning() << "Failed to compile billboard fragment shader:" << m_billboardProgram->log();
        return false;
    }
    
    qDebug() << "Linking shader program";
    if (!m_billboardProgram->link()) {
        qWarning() << "Failed to link billboard shader program:" << m_billboardProgram->log();
        return false;
    }
    
    qDebug() << "Billboard shader program linked successfully";
    
    // Print uniform locations for debugging
    qDebug() << "Billboard Shader Uniforms:";
    QStringList uniforms = {"view", "projection", "position", "size", "textureSampler"};
    
    for (const QString &name : uniforms) {
        int loc = m_billboardProgram->uniformLocation(name);
        qDebug() << "  Uniform" << name << "location:" << loc;
    }
    
    return true;
}