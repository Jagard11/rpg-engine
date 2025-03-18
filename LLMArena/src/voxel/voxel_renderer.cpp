// src/voxel/voxel_renderer.cpp
#include "../../include/voxel/voxel_renderer.h"
#include <QDebug>
#include <QDir>
#include <QMap>
#include <QPainter>

VoxelRenderer::VoxelRenderer(QObject* parent) 
    : QObject(parent),
      m_world(nullptr), 
      m_vertexBuffer(QOpenGLBuffer::VertexBuffer),
      m_indexBuffer(QOpenGLBuffer::IndexBuffer),
      m_shaderProgram(nullptr),
      m_voxelCount(0) {
    
    // Initialize texture map
    m_textures["cobblestone"] = nullptr;
    m_textures["grass"] = nullptr;
    m_textures["dirt"] = nullptr;
    m_textures["default"] = nullptr;
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
    
    // Clean up textures
    for (auto it = m_textures.begin(); it != m_textures.end(); ++it) {
        if (it.value()) {
            it.value()->destroy();
            delete it.value();
        }
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
    
    // Load textures
    loadTextures();
}

void VoxelRenderer::loadTextures() {
    QString resourcePath = QDir::currentPath() + "/resources/";
    
    // Load cobblestone texture
    QImage cobblestoneImg(resourcePath + "cobblestone.png");
    if (cobblestoneImg.isNull()) {
        qWarning() << "Failed to load cobblestone texture";
        createDefaultTexture("cobblestone");
    } else {
        createTexture("cobblestone", cobblestoneImg);
    }
    
    // Load grass texture
    QImage grassImg(resourcePath + "grass.png");
    if (grassImg.isNull()) {
        qWarning() << "Failed to load grass texture";
        createDefaultTexture("grass");
    } else {
        createTexture("grass", grassImg);
    }
    
    // Load dirt texture
    QImage dirtImg(resourcePath + "dirt.png");
    if (dirtImg.isNull()) {
        qWarning() << "Failed to load dirt texture";
        createDefaultTexture("dirt");
    } else {
        createTexture("dirt", dirtImg);
    }
    
    // Create default texture
    createDefaultTexture("default");
}

void VoxelRenderer::createTexture(const QString& name, const QImage& image) {
    // Clean up existing texture
    if (m_textures[name]) {
        m_textures[name]->destroy();
        delete m_textures[name];
    }
    
    // Create new texture
    m_textures[name] = new QOpenGLTexture(image);
    m_textures[name]->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    m_textures[name]->setMagnificationFilter(QOpenGLTexture::Linear);
    m_textures[name]->setWrapMode(QOpenGLTexture::Repeat);
}

void VoxelRenderer::createDefaultTexture(const QString& name) {
    // Create a simple checkered pattern texture
    QImage defaultImg(16, 16, QImage::Format_RGBA8888);
    defaultImg.fill(Qt::transparent);
    
    QPainter painter(&defaultImg);
    painter.setPen(Qt::NoPen);
    
    // Draw checkerboard pattern
    if (name == "cobblestone") {
        painter.setBrush(QColor(128, 128, 128));
    } else if (name == "grass") {
        painter.setBrush(QColor(0, 128, 0));
    } else if (name == "dirt") {
        painter.setBrush(QColor(139, 69, 19));
    } else {
        painter.setBrush(QColor(255, 0, 255)); // Pink for default unknown
    }
    
    painter.drawRect(0, 0, 8, 8);
    painter.drawRect(8, 8, 8, 8);
    
    if (name == "cobblestone") {
        painter.setBrush(QColor(100, 100, 100));
    } else if (name == "grass") {
        painter.setBrush(QColor(0, 100, 0));
    } else if (name == "dirt") {
        painter.setBrush(QColor(101, 67, 33));
    } else {
        painter.setBrush(QColor(200, 0, 200));
    }
    
    painter.drawRect(0, 8, 8, 8);
    painter.drawRect(8, 0, 8, 8);
    
    createTexture(name, defaultImg);
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
        renderVoxel.type = voxel.type;
        
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
        // Position (x, y, z), Normal (nx, ny, nz), TexCoord (u, v)
        // Front face
        -halfSize, -halfSize,  halfSize,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, // bottom-left
         halfSize, -halfSize,  halfSize,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f, // bottom-right
         halfSize,  halfSize,  halfSize,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f, // top-right
        -halfSize,  halfSize,  halfSize,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, // top-left
        
        // Back face
        -halfSize, -halfSize, -halfSize,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f, // bottom-left
        -halfSize,  halfSize, -halfSize,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f, // top-left
         halfSize,  halfSize, -halfSize,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f, // top-right
         halfSize, -halfSize, -halfSize,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, // bottom-right
        
        // Left face
        -halfSize,  halfSize,  halfSize, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // top-right
        -halfSize,  halfSize, -halfSize, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // top-left
        -halfSize, -halfSize, -halfSize, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // bottom-left
        -halfSize, -halfSize,  halfSize, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // bottom-right
        
        // Right face
         halfSize,  halfSize,  halfSize,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // top-left
         halfSize, -halfSize,  halfSize,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // bottom-left
         halfSize, -halfSize, -halfSize,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // bottom-right
         halfSize,  halfSize, -halfSize,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // top-right
        
        // Bottom face
        -halfSize, -halfSize, -halfSize,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f, // top-right
         halfSize, -halfSize, -halfSize,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f, // top-left
         halfSize, -halfSize,  halfSize,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f, // bottom-left
        -halfSize, -halfSize,  halfSize,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f, // bottom-right
        
        // Top face
        -halfSize,  halfSize, -halfSize,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f, // top-left
        -halfSize,  halfSize,  halfSize,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // bottom-left
         halfSize,  halfSize,  halfSize,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // bottom-right
         halfSize,  halfSize, -halfSize,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f  // top-right
    };
    
    // Cube indices (6 faces, 2 triangles per face, 3 vertices per triangle)
    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,       // Front face
        4, 5, 6, 6, 7, 4,       // Back face
        8, 9, 10, 10, 11, 8,    // Left face
        12, 13, 14, 14, 15, 12, // Right face
        16, 17, 18, 18, 19, 16, // Bottom face
        20, 21, 22, 22, 23, 20  // Top face
    };
    
    // Bind vertex buffer
    m_vertexBuffer.bind();
    m_vertexBuffer.allocate(vertices, sizeof(vertices));
    
    // Setup vertex attributes
    glEnableVertexAttribArray(0); // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    
    glEnableVertexAttribArray(1); // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 
                         reinterpret_cast<void*>(3 * sizeof(float)));
    
    glEnableVertexAttribArray(2); // Texture coordinates
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 
                         reinterpret_cast<void*>(6 * sizeof(float)));
    
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
    
    // Vertex shader with instancing and texture support
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec3 normal;
        layout(location = 2) in vec2 texCoord;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        // Instance data
        uniform vec3 voxelPosition;
        uniform vec4 voxelColor;
        
        out vec3 fragPos;
        out vec3 fragNormal;
        out vec4 fragColor;
        out vec2 fragTexCoord;
        
        void main() {
            vec3 worldPos = position + voxelPosition;
            gl_Position = projection * view * vec4(worldPos, 1.0);
            
            fragPos = worldPos;
            fragNormal = normal;
            fragColor = voxelColor;
            fragTexCoord = texCoord;
        }
    )";
    
    // Fragment shader with simple lighting and texture
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 fragPos;
        in vec3 fragNormal;
        in vec4 fragColor;
        in vec2 fragTexCoord;
        
        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform sampler2D textureSampler;
        uniform bool useTexture;
        
        out vec4 outColor;
        
        void main() {
            // Material properties
            vec4 materialColor;
            if (useTexture) {
                materialColor = texture(textureSampler, fragTexCoord) * fragColor;
            } else {
                materialColor = fragColor;
            }
            
            // Ambient
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * materialColor.rgb;
            
            // Diffuse
            vec3 norm = normalize(fragNormal);
            vec3 lightDir = normalize(lightPos - fragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * materialColor.rgb;
            
            // Specular
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - fragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
            
            // Result
            vec3 result = (ambient + diffuse + specular);
            outColor = vec4(result, materialColor.a);
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
        
        // Select and bind texture based on voxel type
        QOpenGLTexture* texture = nullptr;
        bool useTexture = true;
        
        switch (voxel.type) {
            case VoxelType::Cobblestone:
                texture = m_textures["cobblestone"];
                break;
            case VoxelType::Grass:
                texture = m_textures["grass"];
                break;
            case VoxelType::Dirt:
                texture = m_textures["dirt"];
                break;
            case VoxelType::Solid:
            default:
                texture = m_textures["default"];
                useTexture = false; // Use color only for generic solids
                break;
        }
        
        // Bind texture if available
        if (texture && texture->isCreated()) {
            texture->bind();
            m_shaderProgram->setUniformValue("useTexture", useTexture);
        } else {
            m_shaderProgram->setUniformValue("useTexture", false);
        }
        
        // Draw cube
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
        
        // Unbind texture
        if (texture && texture->isCreated()) {
            texture->release();
        }
    }
    
    // Unbind
    m_vao.release();
    m_shaderProgram->release();
}