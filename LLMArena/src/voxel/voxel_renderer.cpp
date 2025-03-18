// src/voxel/voxel_renderer.cpp
#include "../../include/voxel/voxel_renderer.h"
#include <QDebug>

VoxelRenderer::VoxelRenderer(QObject* parent) 
    : QObject(parent),
      m_world(nullptr), 
      m_vertexBuffer(QOpenGLBuffer::VertexBuffer),
      m_indexBuffer(QOpenGLBuffer::IndexBuffer),
      m_shaderProgram(nullptr),
      m_voxelCount(0) {
}

VoxelRenderer::~VoxelRenderer() {
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
    
    delete m_shaderProgram;
}

void VoxelRenderer::initialize() {
    // Initialize OpenGL functions
    initializeOpenGLFunctions();
    
    // Create shader program
    createShaders();
    
    // Create VAO
    m_vao.create();
    
    // Create buffers
    m_vertexBuffer.create();
    m_indexBuffer.create();
    
    // Create cube geometry (will be instanced for each voxel)
    createCubeGeometry(1.0f); // 1-meter cube
}

void VoxelRenderer::setWorld(VoxelWorld* world) {
    if (m_world) {
        // Disconnect from old world
        disconnect(m_world, &VoxelWorld::worldChanged, this, &VoxelRenderer::updateRenderData);
    }
    
    m_world = world;
    
    if (m_world) {
        // Connect to new world
        connect(m_world, &VoxelWorld::worldChanged, this, &VoxelRenderer::updateRenderData);
        
        // Update rendering data
        updateRenderData();
    }
}

void VoxelRenderer::updateRenderData() {
    if (!m_world) return;
    
    // Clear existing data
    m_visibleVoxels.clear();
    
    // Get visible voxels from the world
    QVector<VoxelPos> visiblePositions = m_world->getVisibleVoxels();
    
    // Convert to render voxels
    for (const VoxelPos& pos : visiblePositions) {
        Voxel voxel = m_world->getVoxel(pos);
        
        RenderVoxel renderVoxel;
        renderVoxel.pos = pos;
        renderVoxel.color = voxel.color;
        
        m_visibleVoxels.append(renderVoxel);
    }
    
    m_voxelCount = m_visibleVoxels.size();
}

void VoxelRenderer::createCubeGeometry(float size) {
    // Bind VAO
    m_vao.bind();
    
    // Cube vertex positions (8 vertices)
    float halfSize = size / 2.0f;
    float vertices[] = {
        // Position (x, y, z) and Normal (nx, ny, nz)
        -halfSize, -halfSize, -halfSize, 0.0f, 0.0f, -1.0f, // 0: left-bottom-back
        halfSize, -halfSize, -halfSize, 0.0f, 0.0f, -1.0f,  // 1: right-bottom-back
        halfSize, halfSize, -halfSize, 0.0f, 0.0f, -1.0f,   // 2: right-top-back
        -halfSize, halfSize, -halfSize, 0.0f, 0.0f, -1.0f,  // 3: left-top-back
        
        -halfSize, -halfSize, halfSize, 0.0f, 0.0f, 1.0f,   // 4: left-bottom-front
        halfSize, -halfSize, halfSize, 0.0f, 0.0f, 1.0f,    // 5: right-bottom-front
        halfSize, halfSize, halfSize, 0.0f, 0.0f, 1.0f,     // 6: right-top-front
        -halfSize, halfSize, halfSize, 0.0f, 0.0f, 1.0f     // 7: left-top-front
    };
    
    // Cube indices (6 faces, 2 triangles per face, 3 vertices per triangle)
    unsigned int indices[] = {
        // Back face
        0, 1, 2, 2, 3, 0,
        // Front face
        4, 5, 6, 6, 7, 4,
        // Left face
        0, 3, 7, 7, 4, 0,
        // Right face
        1, 5, 6, 6, 2, 1,
        // Bottom face
        0, 4, 5, 5, 1, 0,
        // Top face
        3, 2, 6, 6, 7, 3
    };
    
    // Bind vertex buffer
    m_vertexBuffer.bind();
    m_vertexBuffer.allocate(vertices, sizeof(vertices));
    
    // Setup vertex attributes
    glEnableVertexAttribArray(0); // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    
    glEnableVertexAttribArray(1); // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 
                         reinterpret_cast<void*>(3 * sizeof(float)));
    
    // Bind index buffer
    m_indexBuffer.bind();
    m_indexBuffer.allocate(indices, sizeof(indices));
    
    // Unbind VAO and buffers
    m_vao.release();
    m_vertexBuffer.release();
    m_indexBuffer.release();
}

void VoxelRenderer::createShaders() {
    // Create shader program
    m_shaderProgram = new QOpenGLShaderProgram();
    
    // Vertex shader with instancing support
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec3 normal;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        // Instance data
        uniform vec3 voxelPosition;
        uniform vec4 voxelColor;
        
        out vec3 fragPos;
        out vec3 fragNormal;
        out vec4 fragColor;
        
        void main() {
            vec3 worldPos = position + voxelPosition;
            gl_Position = projection * view * vec4(worldPos, 1.0);
            
            fragPos = worldPos;
            fragNormal = normal;
            fragColor = voxelColor;
        }
    )";
    
    // Fragment shader with simple lighting
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 fragPos;
        in vec3 fragNormal;
        in vec4 fragColor;
        
        uniform vec3 lightPos;
        uniform vec3 viewPos;
        
        out vec4 outColor;
        
        void main() {
            // Ambient
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
            
            // Diffuse
            vec3 norm = normalize(fragNormal);
            vec3 lightDir = normalize(lightPos - fragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
            
            // Specular
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - fragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
            
            // Result
            vec3 result = (ambient + diffuse + specular) * fragColor.rgb;
            outColor = vec4(result, fragColor.a);
        }
    )";
    
    // Add shaders
    if (!m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qCritical() << "Failed to compile vertex shader:" << m_shaderProgram->log();
    }
    
    if (!m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qCritical() << "Failed to compile fragment shader:" << m_shaderProgram->log();
    }
    
    // Link
    if (!m_shaderProgram->link()) {
        qCritical() << "Failed to link shader program:" << m_shaderProgram->log();
    }
}

void VoxelRenderer::render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix) {
    if (!m_world || !m_shaderProgram) return;
    
    // Bind shader
    if (!m_shaderProgram->bind()) {
        qCritical() << "Failed to bind shader program";
        return;
    }
    
    // Set common uniforms
    m_shaderProgram->setUniformValue("view", viewMatrix);
    m_shaderProgram->setUniformValue("projection", projectionMatrix);
    
    // Setup lighting
    QVector3D lightPos(0.0f, 10.0f, 0.0f); // Light above center
    m_shaderProgram->setUniformValue("lightPos", lightPos);
    
    // Extract camera position from view matrix (inverse view matrix * origin)
    QMatrix4x4 invView = viewMatrix.inverted();
    QVector3D camPos = invView * QVector3D(0, 0, 0);
    m_shaderProgram->setUniformValue("viewPos", camPos);
    
    // Bind VAO
    m_vao.bind();
    
    // Draw each visible voxel
    for (const RenderVoxel& voxel : m_visibleVoxels) {
        // Set voxel-specific uniforms
        m_shaderProgram->setUniformValue("voxelPosition", voxel.pos.toWorldPos());
        m_shaderProgram->setUniformValue("voxelColor", QVector4D(
            voxel.color.redF(), voxel.color.greenF(), voxel.color.blueF(), voxel.color.alphaF()));
        
        // Draw cube
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    }
    
    // Unbind
    m_vao.release();
    m_shaderProgram->release();
}