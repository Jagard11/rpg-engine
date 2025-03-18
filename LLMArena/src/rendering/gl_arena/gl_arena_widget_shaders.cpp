// src/rendering/gl_arena/gl_arena_widget_shaders.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include <QDebug>

// Ultra simple shader that just draws textured quads - using the simplest possible GLSL
const char* billboardVertexShaderSource =
    "#version 120\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "uniform vec3 position;\n"
    "uniform vec2 size;\n"
    "attribute vec2 vertexPosition;\n"
    "attribute vec2 vertexTexCoord;\n"
    "varying vec2 fragTexCoord;\n"
    "void main() {\n"
    "    vec3 pos = vec3(vertexPosition.x * size.x, vertexPosition.y * size.y, 0.0) + position;\n"
    "    gl_Position = projection * view * vec4(pos, 1.0);\n"
    "    fragTexCoord = vertexTexCoord;\n"
    "}\n";

// Ultra simple fragment shader - using the simplest possible GLSL
const char* billboardFragmentShaderSource =
    "#version 120\n"
    "varying vec2 fragTexCoord;\n"
    "uniform sampler2D textureSampler;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(textureSampler, fragTexCoord);\n"
    "}\n";

bool GLArenaWidget::initShaders()
{
    qDebug() << "Initializing ultra simple billboard shader";
    
    // Clean up existing shader
    if (m_billboardProgram) {
        delete m_billboardProgram;
        m_billboardProgram = nullptr;
    }
    
    // Try/catch for shader creation
    try {
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
        
        // Set attribute bindings before linking (important for some drivers)
        m_billboardProgram->bindAttributeLocation("vertexPosition", 0);
        m_billboardProgram->bindAttributeLocation("vertexTexCoord", 1);
        
        qDebug() << "Linking shader program";
        if (!m_billboardProgram->link()) {
            qWarning() << "Failed to link billboard shader program:" << m_billboardProgram->log();
            return false;
        }
        
        qDebug() << "Billboard shader program linked successfully";
        
        // Verify and print uniform locations for debugging
        qDebug() << "Billboard Shader Uniforms:";
        QStringList uniforms = {"view", "projection", "position", "size", "textureSampler"};
        
        for (const QString &name : uniforms) {
            int loc = m_billboardProgram->uniformLocation(name);
            qDebug() << "  Uniform" << name << "location:" << loc;
        }
        
        return true;
    } 
    catch (const std::exception& e) {
        qCritical() << "Exception in initShaders:" << e.what();
        return false;
    }
    catch (...) {
        qCritical() << "Unknown exception in initShaders";
        return false;
    }
}