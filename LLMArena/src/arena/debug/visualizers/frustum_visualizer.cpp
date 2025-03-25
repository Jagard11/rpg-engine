// src/arena/debug/visualizers/frustum_visualizer.cpp
#include "../../../../include/arena/debug/visualizers/frustum_visualizer.h"
#include "../../../../include/arena/voxels/culling/view_frustum.h"
#include <QDebug>

FrustumVisualizer::FrustumVisualizer(QObject* parent)
    : QObject(parent),
      m_enabled(false),
      m_shader(nullptr),
      m_vertexBuffer(QOpenGLBuffer::VertexBuffer),
      m_indexBuffer(QOpenGLBuffer::IndexBuffer)
{
    // Initialize frustum points (8 corners of the frustum)
    m_frustumPoints.resize(8);
    
    // Create view frustum object
    m_viewFrustum = std::make_unique<ViewFrustum>();
}

FrustumVisualizer::~FrustumVisualizer()
{
    // Clean up OpenGL resources
    if (m_vertexBuffer.isCreated()) {
        m_vertexBuffer.destroy();
    }
    
    if (m_indexBuffer.isCreated()) {
        m_indexBuffer.destroy();
    }
    
    if (m_vao.isCreated()) {
        m_vao.destroy();
    }
    
    delete m_shader;
}

void FrustumVisualizer::initialize() {
    // Initialize OpenGL resources
    initializeOpenGLFunctions();
    
    // Create shader program
    createShaders();
    
    // Create frustum geometry
    createFrustumGeometry();
    
    qDebug() << "FrustumVisualizer fully initialized";
}

void FrustumVisualizer::render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix)
{
    if (!m_enabled) {
        return;
    }
    
    // Check if we have OpenGL functions initialized
    static bool openGLInitialized = false;
    if (!openGLInitialized) {
        try {
            initializeOpenGLFunctions();
            // Now create shaders and other OpenGL resources
            createShaders();
            createFrustumGeometry();
            openGLInitialized = true;
            qDebug() << "Initialized OpenGL functions for frustum visualizer";
        } catch (const std::exception& e) {
            qWarning() << "Failed to initialize OpenGL for frustum visualizer:" << e.what();
            return;
        } catch (...) {
            qWarning() << "Unknown error initializing OpenGL for frustum visualizer";
            return;
        }
    }
    
    // Check if we have a valid shader and geometry
    if (!m_shader || !m_shader->isLinked() || !m_vao.isCreated()) {
        qWarning() << "Frustum visualizer not properly initialized";
        return;
    }

    // Check if we have a valid current context
    if (!QOpenGLContext::currentContext()) {
        qWarning() << "No valid OpenGL context in frustum visualizer render";
        return;
    }

    // Compute the inverse view-projection matrix to transform NDC corners to world space
    QMatrix4x4 viewProjection = projectionMatrix * viewMatrix;
    QMatrix4x4 viewProjectionInverse = viewProjection.inverted();
    
    // Update frustum geometry
    updateFrustumGeometry(viewProjectionInverse);
    
    // Save OpenGL state
    GLboolean oldDepthTest;
    glGetBooleanv(GL_DEPTH_TEST, &oldDepthTest);
    
    GLboolean oldLineSmooth;
    glGetBooleanv(GL_LINE_SMOOTH, &oldLineSmooth);
    
    GLfloat oldLineWidth;
    glGetFloatv(GL_LINE_WIDTH, &oldLineWidth);
    
    // Enable required OpenGL state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    // Enable line smoothing and increase width for better visibility
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(5.0f); // Even thicker lines
    
    // Bind shader
    m_shader->bind();
    
    // Set shader uniforms
    m_shader->setUniformValue("viewMatrix", viewMatrix);
    m_shader->setUniformValue("projectionMatrix", projectionMatrix);
    
    // Brighter color for better visibility
    m_shader->setUniformValue("color", QVector4D(1.0f, 0.1f, 0.1f, 1.0f)); // Bright red
    
    // Bind VAO
    m_vao.bind();
    
    // Update vertex buffer with new frustum points
    m_vertexBuffer.bind();
    m_vertexBuffer.write(0, m_frustumPoints.constData(), m_frustumPoints.size() * sizeof(QVector3D));
    m_vertexBuffer.release();
    
    // Draw frustum lines with reduced depth test for better visibility
    GLboolean oldDepthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &oldDepthMask);
    glDepthMask(GL_FALSE); // Don't write to depth buffer
    
    // Draw with blend enabled for semi-transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw frustum lines
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);
    
    // Restore depth mask
    glDepthMask(oldDepthMask);
    
    // Unbind VAO
    m_vao.release();
    
    // Unbind shader
    m_shader->release();
    
    // Restore OpenGL state
    if (!oldDepthTest) {
        glDisable(GL_DEPTH_TEST);
    }
    
    if (!oldLineSmooth) {
        glDisable(GL_LINE_SMOOTH);
    }
    
    glLineWidth(oldLineWidth);
    
    // Disable blend if it wasn't enabled before
    GLboolean oldBlend;
    glGetBooleanv(GL_BLEND, &oldBlend);
    if (!oldBlend) {
        glDisable(GL_BLEND);
    }

    // Log that the frustum was rendered (but less frequently to reduce spam)
    static int logCounter = 0;
    if (logCounter++ % 60 == 0) {
        qDebug() << "Frustum visualizer rendered successfully";
    }
}

void FrustumVisualizer::setEnabled(bool enabled)
{
    m_enabled = enabled;
    qDebug() << "Frustum visualizer enabled:" << m_enabled;
}

bool FrustumVisualizer::isEnabled() const
{
    return m_enabled;
}

void FrustumVisualizer::createShaders()
{
    // Create shader program
    m_shader = new QOpenGLShaderProgram();
    
    // Vertex shader
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        
        uniform mat4 viewMatrix;
        uniform mat4 projectionMatrix;
        
        void main() {
            gl_Position = projectionMatrix * viewMatrix * vec4(position, 1.0);
        }
    )";
    
    // Fragment shader
    const char* fragmentShaderSource = R"(
        #version 330 core
        uniform vec4 color;
        
        out vec4 fragColor;
        
        void main() {
            fragColor = color;
        }
    )";
    
    // Compile and link shaders
    if (!m_shader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qCritical() << "Failed to compile frustum visualizer vertex shader:" << m_shader->log();
    }
    
    if (!m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qCritical() << "Failed to compile frustum visualizer fragment shader:" << m_shader->log();
    }
    
    if (!m_shader->link()) {
        qCritical() << "Failed to link frustum visualizer shader program:" << m_shader->log();
    }

    qDebug() << "Frustum visualizer shaders created and linked";
}

void FrustumVisualizer::createFrustumGeometry()
{
    // Create and bind VAO
    m_vao.create();
    m_vao.bind();
    
    // Create and bind vertex buffer
    m_vertexBuffer.create();
    m_vertexBuffer.bind();
    
    // Allocate space for 8 vertices (frustum corners)
    m_vertexBuffer.allocate(8 * sizeof(QVector3D));
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), nullptr);
    
    // Define indices for the 12 lines of the frustum box
    const GLuint indices[] = {
        // Near face
        0, 1, 1, 3, 3, 2, 2, 0,
        // Far face
        4, 5, 5, 7, 7, 6, 6, 4,
        // Connecting lines
        0, 4, 1, 5, 2, 6, 3, 7
    };
    
    // Create and bind index buffer
    m_indexBuffer.create();
    m_indexBuffer.bind();
    m_indexBuffer.allocate(indices, sizeof(indices));
    
    // Unbind buffers
    m_indexBuffer.release();
    m_vertexBuffer.release();
    m_vao.release();

    qDebug() << "Frustum visualizer geometry created";
}

void FrustumVisualizer::updateFrustumGeometry(const QMatrix4x4& viewProjectionInverse)
{
    // Define normalized device coordinates (NDC) for the 8 frustum corners
    // NDC corners arranged as:
    //    near        far
    // 0 --- 1    4 --- 5
    // |     |    |     |
    // 2 --- 3    6 --- 7
    const QVector4D ndcCorners[] = {
        // Near face (z=-1)
        QVector4D(-1, -1, -1, 1), // Bottom left
        QVector4D( 1, -1, -1, 1), // Bottom right
        QVector4D(-1,  1, -1, 1), // Top left
        QVector4D( 1,  1, -1, 1), // Top right
        // Far face (z=1)
        QVector4D(-1, -1,  1, 1), // Bottom left
        QVector4D( 1, -1,  1, 1), // Bottom right
        QVector4D(-1,  1,  1, 1), // Top left
        QVector4D( 1,  1,  1, 1)  // Top right
    };
    
    // Transform NDC corners to world space
    for (int i = 0; i < 8; ++i) {
        QVector4D worldPos = viewProjectionInverse * ndcCorners[i];
        
        // Perform perspective division
        if (worldPos.w() != 0.0f) {
            worldPos /= worldPos.w();
        }
        
        m_frustumPoints[i] = worldPos.toVector3D();
    }

    // Log the updated frustum corners
    qDebug() << "Frustum corners updated: Near-Bottom-Left:" << m_frustumPoints[0]
             << "Far-Top-Right:" << m_frustumPoints[7];
}