// src/arena/ui/gl_widgets/gl_arena_widget_shaders.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
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
    "uniform vec4 color;\n"
    "void main() {\n"
    "    vec4 texColor = texture2D(textureSampler, fragTexCoord);\n"
    "    gl_FragColor = texColor * color;\n"
    "}\n";

bool GLArenaWidget::initShaders()
{
    // Clean up existing shader
    if (m_billboardProgram) {
        if (context() && context()->isValid()) {
            makeCurrent();
            delete m_billboardProgram;
            doneCurrent();
        } else {
            delete m_billboardProgram;
        }
        m_billboardProgram = nullptr;
    }
    
    // Check if we have a valid OpenGL context
    if (!context() || !context()->isValid()) {
        qWarning() << "No valid OpenGL context for shader initialization";
        return false;
    }
    
    // Try/catch for shader creation
    try {
        // Make sure we have the current context
        makeCurrent();
        
        // Create billboard shader program
        m_billboardProgram = new QOpenGLShaderProgram(this);
        
        if (!m_billboardProgram) {
            qWarning() << "Failed to allocate shader program";
            doneCurrent();
            return false;
        }
        
        // Compile billboard vertex shader
        if (!m_billboardProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, billboardVertexShaderSource)) {
            qWarning() << "Failed to compile billboard vertex shader:" << m_billboardProgram->log();
            delete m_billboardProgram;
            m_billboardProgram = nullptr;
            doneCurrent();
            return false;
        }
        
        // Compile billboard fragment shader
        if (!m_billboardProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, billboardFragmentShaderSource)) {
            qWarning() << "Failed to compile billboard fragment shader:" << m_billboardProgram->log();
            delete m_billboardProgram;
            m_billboardProgram = nullptr;
            doneCurrent();
            return false;
        }
        
        // Set attribute bindings before linking (important for some drivers)
        m_billboardProgram->bindAttributeLocation("vertexPosition", 0);
        m_billboardProgram->bindAttributeLocation("vertexTexCoord", 1);
        
        // Link shader program
        if (!m_billboardProgram->link()) {
            qWarning() << "Failed to link billboard shader program:" << m_billboardProgram->log();
            delete m_billboardProgram;
            m_billboardProgram = nullptr;
            doneCurrent();
            return false;
        }
        
        // Log success
        qDebug() << "Billboard shader program created and linked successfully";
        
        doneCurrent();
        return true;
    } 
    catch (const std::exception& e) {
        qCritical() << "Exception in initShaders:" << e.what();
        if (m_billboardProgram) {
            delete m_billboardProgram;
            m_billboardProgram = nullptr;
        }
        doneCurrent();
        return false;
    }
    catch (...) {
        qCritical() << "Unknown exception in initShaders";
        if (m_billboardProgram) {
            delete m_billboardProgram;
            m_billboardProgram = nullptr;
        }
        doneCurrent();
        return false;
    }
}