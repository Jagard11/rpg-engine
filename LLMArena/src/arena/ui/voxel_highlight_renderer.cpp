// src/arena/ui/voxel_highlight_renderer.cpp
#include "../../../include/arena/ui/voxel_highlight_renderer.h"
#include <QDebug>

VoxelHighlightRenderer::VoxelHighlightRenderer(QObject* parent)
    : QObject(parent),
      m_shader(nullptr),
      m_wireframeIndexCount(0),
      m_highlightFace(-1)
{
}

VoxelHighlightRenderer::~VoxelHighlightRenderer()
{
    // Clean up shader program
    delete m_shader;
    
    // Clean up face VAOs and VBOs
    for (auto vao : m_faceVAOs) {
        if (vao && vao->isCreated()) {
            vao->destroy();
            delete vao;
        }
    }
    
    for (auto vbo : m_faceVBOs) {
        if (vbo && vbo->isCreated()) {
            vbo->destroy();
            delete vbo;
        }
    }
    
    // Clean up main VAO and buffers
    if (m_vao.isCreated()) {
        m_vao.destroy();
    }
    
    if (m_vbo.isCreated()) {
        m_vbo.destroy();
    }
    
    if (m_ibo.isCreated()) {
        m_ibo.destroy();
    }
}

void VoxelHighlightRenderer::initialize()
{
    // Initialize OpenGL functions
    initializeOpenGLFunctions();
    
    // Create shaders
    createShaders();
    
    // Create wireframe cube geometry
    createWireframeCubeGeometry();
    
    // Create face highlight geometry
    createFaceHighlightGeometry();
}

void VoxelHighlightRenderer::render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix, 
                                   const QVector3D& position, int highlightFace)
{
    // Skip if no shader or geometry
    if (!m_shader || !m_vao.isCreated()) {
        return;
    }
    
    // Cache the highlight face
    m_highlightFace = highlightFace;
    
    // Get OpenGL state
    GLboolean oldDepthTest;
    glGetBooleanv(GL_DEPTH_TEST, &oldDepthTest);
    
    GLboolean oldBlend;
    glGetBooleanv(GL_BLEND, &oldBlend);
    
    GLint oldCullFace;
    glGetIntegerv(GL_CULL_FACE, &oldCullFace);
    
    // Set up OpenGL state for rendering
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    
    // Bind shader
    m_shader->bind();
    
    // Set up common uniforms
    m_shader->setUniformValue("viewMatrix", viewMatrix);
    m_shader->setUniformValue("projectionMatrix", projectionMatrix);
    
    // Create model matrix for positioning
    QMatrix4x4 modelMatrix;
    modelMatrix.setToIdentity();
    modelMatrix.translate(position);
    
    // Render wireframe cube
    m_shader->setUniformValue("modelMatrix", modelMatrix);
    m_shader->setUniformValue("highlightColor", QVector4D(1.0f, 1.0f, 1.0f, 0.5f)); // White, semi-transparent
    
    m_vao.bind();
    glDrawElements(GL_LINES, m_wireframeIndexCount, GL_UNSIGNED_INT, nullptr);
    m_vao.release();
    
    // Render highlighted face if specified
    if (highlightFace >= 0 && highlightFace < m_faceVAOs.size()) {
        m_shader->setUniformValue("modelMatrix", modelMatrix);
        m_shader->setUniformValue("highlightColor", QVector4D(1.0f, 1.0f, 0.0f, 0.5f)); // Yellow, semi-transparent
        
        m_faceVAOs[highlightFace]->bind();
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        m_faceVAOs[highlightFace]->release();
    }
    
    // Unbind shader
    m_shader->release();
    
    // Restore OpenGL state
    if (!oldDepthTest) {
        glDisable(GL_DEPTH_TEST);
    }
    
    if (!oldBlend) {
        glDisable(GL_BLEND);
    }
    
    if (oldCullFace) {
        glEnable(GL_CULL_FACE);
    }
}

void VoxelHighlightRenderer::createShaders()
{
    // Create shader program
    m_shader = new QOpenGLShaderProgram();
    
    // Vertex shader
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        
        uniform mat4 modelMatrix;
        uniform mat4 viewMatrix;
        uniform mat4 projectionMatrix;
        
        void main() {
            gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);
        }
    )";
    
    // Fragment shader
    const char* fragmentShaderSource = R"(
        #version 330 core
        uniform vec4 highlightColor;
        
        out vec4 fragColor;
        
        void main() {
            fragColor = highlightColor;
        }
    )";
    
    // Compile shaders
    if (!m_shader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qCritical() << "Failed to compile highlight vertex shader:" << m_shader->log();
    }
    
    if (!m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qCritical() << "Failed to compile highlight fragment shader:" << m_shader->log();
    }
    
    // Link shader program
    if (!m_shader->link()) {
        qCritical() << "Failed to link highlight shader program:" << m_shader->log();
    }
}

void VoxelHighlightRenderer::createWireframeCubeGeometry()
{
    // Create a wireframe cube with dimensions 1.02 to ensure visibility around a voxel
    const float size = 1.02f;
    const float halfSize = size / 2.0f;
    
    // Cube vertices
    const float vertices[] = {
        // Front-bottom-left
        -halfSize, -halfSize, -halfSize,
        // Front-bottom-right
        halfSize, -halfSize, -halfSize,
        // Front-top-right
        halfSize, halfSize, -halfSize,
        // Front-top-left
        -halfSize, halfSize, -halfSize,
        // Back-bottom-left
        -halfSize, -halfSize, halfSize,
        // Back-bottom-right
        halfSize, -halfSize, halfSize,
        // Back-top-right
        halfSize, halfSize, halfSize,
        // Back-top-left
        -halfSize, halfSize, halfSize
    };
    
    // Wireframe indices - 12 lines, 2 vertices each
    const GLuint indices[] = {
        // Front face
        0, 1, 1, 2, 2, 3, 3, 0,
        // Back face
        4, 5, 5, 6, 6, 7, 7, 4,
        // Connecting lines
        0, 4, 1, 5, 2, 6, 3, 7
    };
    
    m_wireframeIndexCount = sizeof(indices) / sizeof(indices[0]);
    
    // Create VAO
    m_vao.create();
    m_vao.bind();
    
    // Create VBO for vertices
    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(vertices, sizeof(vertices));
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    
    // Create index buffer
    m_ibo.create();
    m_ibo.bind();
    m_ibo.allocate(indices, sizeof(indices));
    
    // Unbind
    m_ibo.release();
    m_vbo.release();
    m_vao.release();
}

void VoxelHighlightRenderer::createFaceHighlightGeometry()
{
    // Create a set of faces for highlighting
    const float size = 1.03f; // Slightly larger than 1 to avoid z-fighting
    const float halfSize = size / 2.0f;
    
    // Create 6 faces: +X, -X, +Y, -Y, +Z, -Z
    const float faceVertices[6][12] = {
        // +X face (right)
        {
            halfSize, -halfSize, -halfSize,
            halfSize, -halfSize, halfSize,
            halfSize, halfSize, halfSize,
            halfSize, halfSize, -halfSize
        },
        // -X face (left)
        {
            -halfSize, -halfSize, -halfSize,
            -halfSize, halfSize, -halfSize,
            -halfSize, halfSize, halfSize,
            -halfSize, -halfSize, halfSize
        },
        // +Y face (top)
        {
            -halfSize, halfSize, -halfSize,
            halfSize, halfSize, -halfSize,
            halfSize, halfSize, halfSize,
            -halfSize, halfSize, halfSize
        },
        // -Y face (bottom)
        {
            -halfSize, -halfSize, -halfSize,
            -halfSize, -halfSize, halfSize,
            halfSize, -halfSize, halfSize,
            halfSize, -halfSize, -halfSize
        },
        // +Z face (back)
        {
            -halfSize, -halfSize, halfSize,
            -halfSize, halfSize, halfSize,
            halfSize, halfSize, halfSize,
            halfSize, -halfSize, halfSize
        },
        // -Z face (front)
        {
            -halfSize, -halfSize, -halfSize,
            halfSize, -halfSize, -halfSize,
            halfSize, halfSize, -halfSize,
            -halfSize, halfSize, -halfSize
        }
    };
    
    // Create VAO and VBO for each face
    m_faceVAOs.resize(6);
    m_faceVBOs.resize(6);
    
    for (int i = 0; i < 6; i++) {
        // Create and bind VAO
        m_faceVAOs[i] = new QOpenGLVertexArrayObject();
        m_faceVAOs[i]->create();
        m_faceVAOs[i]->bind();
        
        // Create and bind VBO
        m_faceVBOs[i] = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        m_faceVBOs[i]->create();
        m_faceVBOs[i]->bind();
        m_faceVBOs[i]->allocate(faceVertices[i], sizeof(faceVertices[i]));
        
        // Set up vertex attributes
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        
        // Unbind
        m_faceVBOs[i]->release();
        m_faceVAOs[i]->release();
    }
}