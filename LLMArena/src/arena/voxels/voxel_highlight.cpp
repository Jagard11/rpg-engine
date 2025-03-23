// src/arena/voxels/voxel_highlight.cpp
#include "../../../include/arena/ui/voxel_highlight_renderer.h"
#include <QOpenGLContext>
#include <QDebug>

VoxelHighlightRenderer::VoxelHighlightRenderer(QObject* parent)
    : QObject(parent),
      m_shader(nullptr),
      m_highlightFace(-1)
{
}

VoxelHighlightRenderer::~VoxelHighlightRenderer()
{
    // Check if we have a valid OpenGL context before cleaning up
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (!ctx || !ctx->isValid()) {
        qWarning() << "No valid OpenGL context in VoxelHighlightRenderer destructor";
        // Just delete the shader pointer, don't try to destroy OpenGL resources
        delete m_shader;
        return;
    }
    
    // Clean up OpenGL resources safely
    try {
        if (m_vao.isCreated()) {
            m_vao.destroy();
        }
        
        if (m_vbo.isCreated()) {
            m_vbo.destroy();
        }
        
        if (m_ibo.isCreated()) {
            m_ibo.destroy();
        }
        
        // Clean up face VAOs and VBOs
        for (auto vao : m_faceVAOs) {
            if (vao && vao->isCreated()) {
                vao->destroy();
                delete vao;
            }
        }
        m_faceVAOs.clear();
        
        for (auto vbo : m_faceVBOs) {
            if (vbo && vbo->isCreated()) {
                vbo->destroy();
                delete vbo;
            }
        }
        m_faceVBOs.clear();
        
        delete m_shader;
    } catch (const std::exception& e) {
        qWarning() << "Exception in VoxelHighlightRenderer destructor:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception in VoxelHighlightRenderer destructor";
    }
}

void VoxelHighlightRenderer::initialize()
{
    // Verify we have a valid OpenGL context
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (!ctx || !ctx->isValid()) {
        qWarning() << "No valid OpenGL context in VoxelHighlightRenderer::initialize";
        return;
    }
    
    try {
        // Initialize OpenGL functions
        initializeOpenGLFunctions();
        
        // Create shader program
        createShaders();
        
        // Create wireframe cube geometry
        createWireframeCubeGeometry();
        
        // Create face highlight geometry
        createFaceHighlightGeometry();
    } catch (const std::exception& e) {
        qWarning() << "Exception in VoxelHighlightRenderer::initialize:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception in VoxelHighlightRenderer::initialize";
    }
}

void VoxelHighlightRenderer::createShaders()
{
    // Clean up previous shader if it exists
    if (m_shader) {
        delete m_shader;
        m_shader = nullptr;
    }
    
    // Create shader program for highlight outlines
    m_shader = new QOpenGLShaderProgram();
    if (!m_shader) {
        qWarning() << "Failed to allocate shader program for voxel highlight";
        return;
    }
    
    // Simple vertex shader for wireframe highlight
    const char* vertexShaderSource = R"(
        #version 120
        attribute vec3 position;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main() {
            gl_Position = projection * view * model * vec4(position, 1.0);
        }
    )";
    
    // Simple fragment shader for wireframe highlight
    const char* fragmentShaderSource = R"(
        #version 120
        uniform vec4 highlightColor;
        
        void main() {
            gl_FragColor = highlightColor;
        }
    )";
    
    // Compile shaders with error checking
    bool success = true;
    
    if (!m_shader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qWarning() << "Failed to compile voxel highlight vertex shader:" << m_shader->log();
        success = false;
    }
    
    if (success && !m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qWarning() << "Failed to compile voxel highlight fragment shader:" << m_shader->log();
        success = false;
    }
    
    // Link shader program
    if (success && !m_shader->link()) {
        qWarning() << "Failed to link voxel highlight shader program:" << m_shader->log();
        success = false;
    }
    
    if (!success) {
        delete m_shader;
        m_shader = nullptr;
    }
}

void VoxelHighlightRenderer::createWireframeCubeGeometry()
{
    // Check if we have a valid shader
    if (!m_shader || !m_shader->isLinked()) {
        qWarning() << "Cannot create wireframe geometry: shader not linked";
        return;
    }
    
    // Clean up existing objects if they exist
    if (m_vao.isCreated()) {
        m_vao.destroy();
    }
    
    if (m_vbo.isCreated()) {
        m_vbo.destroy();
    }
    
    if (m_ibo.isCreated()) {
        m_ibo.destroy();
    }
    
    // Create wireframe cube geometry with slightly expanded size
    const float size = 1.02f; // Slightly larger than the actual voxel
    const float halfSize = size / 2.0f;
    
    // Vertices for 8 corners of the cube
    float vertices[] = {
        // Front face corners
        -halfSize, -halfSize,  halfSize,
         halfSize, -halfSize,  halfSize,
         halfSize,  halfSize,  halfSize,
        -halfSize,  halfSize,  halfSize,
        
        // Back face corners
        -halfSize, -halfSize, -halfSize,
         halfSize, -halfSize, -halfSize,
         halfSize,  halfSize, -halfSize,
        -halfSize,  halfSize, -halfSize
    };
    
    // Indices for 12 lines (edges of the cube)
    unsigned int indices[] = {
        // Front face
        0, 1, 1, 2, 2, 3, 3, 0,
        // Back face
        4, 5, 5, 6, 6, 7, 7, 4,
        // Connecting lines
        0, 4, 1, 5, 2, 6, 3, 7
    };
    
    // Create and bind VAO
    if (!m_vao.create()) {
        qWarning() << "Failed to create VAO for wireframe cube";
        return;
    }
    m_vao.bind();
    
    // Create and bind VBO
    if (!m_vbo.create()) {
        qWarning() << "Failed to create VBO for wireframe cube";
        m_vao.release();
        return;
    }
    m_vbo.bind();
    m_vbo.allocate(vertices, sizeof(vertices));
    
    // Set up vertex attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    
    // Create and bind IBO
    if (!m_ibo.create()) {
        qWarning() << "Failed to create IBO for wireframe cube";
        m_vbo.release();
        m_vao.release();
        return;
    }
    m_ibo.bind();
    m_ibo.allocate(indices, sizeof(indices));
    
    // Record index count for rendering
    m_wireframeIndexCount = sizeof(indices) / sizeof(unsigned int);
    
    // Release bindings
    m_ibo.release();
    m_vbo.release();
    m_vao.release();
}

void VoxelHighlightRenderer::createFaceHighlightGeometry()
{
    // Check if we have a valid shader
    if (!m_shader || !m_shader->isLinked()) {
        qWarning() << "Cannot create face highlight geometry: shader not linked";
        return;
    }
    
    // Clean up existing face VAOs and VBOs
    for (auto vao : m_faceVAOs) {
        if (vao && vao->isCreated()) {
            vao->destroy();
            delete vao;
        }
    }
    m_faceVAOs.clear();
    
    for (auto vbo : m_faceVBOs) {
        if (vbo && vbo->isCreated()) {
            vbo->destroy();
            delete vbo;
        }
    }
    m_faceVBOs.clear();
    
    // Create geometry for highlighting individual faces
    // Each face is a quad made of 2 triangles (6 vertices)
    
    // Define the 6 faces of a cube
    // Each face has 4 vertices for a quad
    // The positions are for a cube of size 1.02 (slightly larger than voxel)
    const float size = 1.02f;
    const float halfSize = size / 2.0f;
    
    // Face data: each face has 4 vertices for a quad
    // Format: x, y, z (4 vertices per face, 6 faces total)
    float faceVertices[6][12] = {
        // +X face (right)
        {
            halfSize, -halfSize, -halfSize,  // bottom-back
            halfSize, -halfSize,  halfSize,  // bottom-front
            halfSize,  halfSize,  halfSize,  // top-front
            halfSize,  halfSize, -halfSize   // top-back
        },
        // -X face (left)
        {
            -halfSize, -halfSize,  halfSize,  // bottom-front
            -halfSize, -halfSize, -halfSize,  // bottom-back
            -halfSize,  halfSize, -halfSize,  // top-back
            -halfSize,  halfSize,  halfSize   // top-front
        },
        // +Y face (top)
        {
            -halfSize,  halfSize,  halfSize,  // front-left
            -halfSize,  halfSize, -halfSize,  // back-left
             halfSize,  halfSize, -halfSize,  // back-right
             halfSize,  halfSize,  halfSize   // front-right
        },
        // -Y face (bottom)
        {
            -halfSize, -halfSize, -halfSize,  // back-left
            -halfSize, -halfSize,  halfSize,  // front-left
             halfSize, -halfSize,  halfSize,  // front-right
             halfSize, -halfSize, -halfSize   // back-right
        },
        // +Z face (front)
        {
            -halfSize, -halfSize,  halfSize,  // bottom-left
             halfSize, -halfSize,  halfSize,  // bottom-right
             halfSize,  halfSize,  halfSize,  // top-right
            -halfSize,  halfSize,  halfSize   // top-left
        },
        // -Z face (back)
        {
             halfSize, -halfSize, -halfSize,  // bottom-right
            -halfSize, -halfSize, -halfSize,  // bottom-left
            -halfSize,  halfSize, -halfSize,  // top-left
             halfSize,  halfSize, -halfSize   // top-right
        }
    };
    
    // Create VAOs and VBOs for each face
    for (int i = 0; i < 6; i++) {
        QOpenGLVertexArrayObject* vao = new QOpenGLVertexArrayObject();
        QOpenGLBuffer* vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        
        if (!vao->create()) {
            qWarning() << "Failed to create VAO for face highlight" << i;
            delete vao;
            delete vbo;
            continue;
        }
        
        vao->bind();
        
        if (!vbo->create()) {
            qWarning() << "Failed to create VBO for face highlight" << i;
            vao->release();
            delete vao;
            delete vbo;
            continue;
        }
        
        vbo->bind();
        vbo->allocate(faceVertices[i], sizeof(float) * 12);
        
        // Set up vertex attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        
        vbo->release();
        vao->release();
        
        m_faceVAOs.append(vao);
        m_faceVBOs.append(vbo);
    }
}

void VoxelHighlightRenderer::render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix, 
                                   const QVector3D& position, int highlightFace)
{
    // Validate input parameters
    if (!qIsFinite(position.x()) || !qIsFinite(position.y()) || !qIsFinite(position.z())) {
        qWarning() << "Invalid position for voxel highlight";
        return;
    }
    
    if (highlightFace < -1 || highlightFace >= 6) {
        qWarning() << "Invalid face index for voxel highlight:" << highlightFace;
        return;
    }
    
    // Check for valid shader and VAO
    if (!m_shader || !m_shader->isLinked() || !m_vao.isCreated()) {
        // Silently return to avoid spam
        return;
    }
    
    // Update highlight face
    m_highlightFace = highlightFace;
    
    try {
        // Bind shader
        if (!m_shader->bind()) {
            qWarning() << "Failed to bind shader for voxel highlight";
            return;
        }
        
        // Set view and projection matrices
        m_shader->setUniformValue("view", viewMatrix);
        m_shader->setUniformValue("projection", projectionMatrix);
        
        // Set model matrix for the voxel position
        QMatrix4x4 modelMatrix;
        modelMatrix.setToIdentity();
        modelMatrix.translate(position);
        m_shader->setUniformValue("model", modelMatrix);
        
        // Store OpenGL state
        glDisable(GL_CULL_FACE);
        GLboolean depthWriteEnabled;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteEnabled);
        glDepthMask(GL_FALSE); // Disable depth writing for the highlight
        
        // Set line width for wireframe
        glLineWidth(2.0f);
        
        if (m_highlightFace < 0) {
            // Render full wireframe cube when no specific face is highlighted
            // Make sure VAO and IBO are properly bound
            m_vao.bind();
            m_ibo.bind();
            
            // Set wireframe color (white)
            m_shader->setUniformValue("highlightColor", QVector4D(1.0f, 1.0f, 1.0f, 1.0f));
            
            // Draw wireframe
            glDrawElements(GL_LINES, m_wireframeIndexCount, GL_UNSIGNED_INT, nullptr);
            
            // Unbind
            m_ibo.release();
            m_vao.release();
        } else if (m_highlightFace >= 0 && m_highlightFace < m_faceVAOs.size()) {
            // Draw wireframe first
            m_vao.bind();
            m_ibo.bind();
            
            // Set wireframe color (white)
            m_shader->setUniformValue("highlightColor", QVector4D(1.0f, 1.0f, 1.0f, 1.0f));
            
            // Draw wireframe
            glDrawElements(GL_LINES, m_wireframeIndexCount, GL_UNSIGNED_INT, nullptr);
            
            // Release wireframe bindings
            m_ibo.release();
            m_vao.release();
            
            // Make sure the face VAO exists and is created
            QOpenGLVertexArrayObject* faceVAO = m_faceVAOs[m_highlightFace];
            if (!faceVAO || !faceVAO->isCreated()) {
                qWarning() << "Invalid face VAO for face" << m_highlightFace;
                m_shader->release();
                return;
            }
            
            // Set face highlight color (semi-transparent white)
            m_shader->setUniformValue("highlightColor", QVector4D(1.0f, 1.0f, 1.0f, 0.3f));
            
            // Enable blending for transparent face
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            // Bind the appropriate face VAO
            faceVAO->bind();
            
            // Draw highlighted face
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            
            // Unbind the face VAO
            faceVAO->release();
        }
        
        // Restore OpenGL state
        if (depthWriteEnabled) {
            glDepthMask(GL_TRUE);
        }
        
        // Unbind shader
        m_shader->release();
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in VoxelHighlightRenderer::render:" << e.what();
        if (m_shader && m_shader->isLinked()) {
            m_shader->release();
        }
    }
    catch (...) {
        qWarning() << "Unknown exception in VoxelHighlightRenderer::render";
        if (m_shader && m_shader->isLinked()) {
            m_shader->release();
        }
    }
}