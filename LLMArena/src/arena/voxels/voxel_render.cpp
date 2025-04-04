// src/arena/voxels/voxel_render.cpp
// include/arena/voxels/voxel_renderer.h
#include "../../../include/arena/voxels/voxel_renderer.h"
#include "../../../include/arena/voxels/culling/view_frustum.h" // Include full definition before unique_ptr is used
#include <QOpenGLContext>
#include <QDebug>
#include <QPainter>
#include <QDir>
#include <algorithm>

// Constructor - keep this simple to avoid memory allocation during init
VoxelRenderer::VoxelRenderer(QObject* parent) 
    : QObject(parent),
      m_world(nullptr), 
      m_vertexBuffer(QOpenGLBuffer::VertexBuffer),
      m_indexBuffer(QOpenGLBuffer::IndexBuffer),
      m_shaderProgram(nullptr),
      m_voxelCount(0),
      m_maxVisibleChunks(256),
      m_frustumCullingEnabled(true),
      m_backfaceCullingEnabled(true) {
    
    // Initialize view frustum
    m_viewFrustum = std::make_unique<ViewFrustum>();
    
    // Initialize texture map
    m_textures["cobblestone"] = nullptr;
    m_textures["grass"] = nullptr;
    m_textures["dirt"] = nullptr;
    m_textures["default"] = nullptr;

    // Get performance settings reference
    m_perfSettings = PerformanceSettings::getInstance();
    
    // Connect to performance settings changes
    connect(m_perfSettings, &PerformanceSettings::settingsChanged, this, &VoxelRenderer::updateSettings);
    
    // Initialize settings from performance settings
    updateSettings();
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
            it.value() = nullptr;  // Set to nullptr after deletion
        }
    }
    
    // Clear container to avoid dangling pointers
    m_textures.clear();
    m_visibleVoxels.clear();
    
    // Delete shader program
    if (m_shaderProgram) {
        delete m_shaderProgram;
        m_shaderProgram = nullptr;
    }
    
    // The m_viewFrustum unique_ptr will be automatically cleaned up
}

void VoxelRenderer::initialize() {
    // Safety check
    if (!QOpenGLContext::currentContext()) {
        qCritical() << "No OpenGL context active during VoxelRenderer initialization";
        return;
    }
    
    try {
        // Initialize OpenGL functions
        initializeOpenGLFunctions();
        
        // Create shader program
        createShaders();
        
        // Create VAO
        if (!m_vao.isCreated()) {
            if (!m_vao.create()) {
                qWarning() << "Failed to create vertex array object";
            }
        }
        
        // Create buffers
        if (!m_vertexBuffer.isCreated()) {
            if (!m_vertexBuffer.create()) {
                qWarning() << "Failed to create vertex buffer";
            }
        }
        
        if (!m_indexBuffer.isCreated()) {
            if (!m_indexBuffer.create()) {
                qWarning() << "Failed to create index buffer";
            }
        }
        
        // Create cube geometry (will be instanced for each voxel)
        createCubeGeometry(1.0f); // 1-meter cube
        
        // Load textures
        loadTextures();
        
        qDebug() << "VoxelRenderer initialized successfully";
    } catch (const std::exception& e) {
        qCritical() << "Exception during VoxelRenderer initialization:" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception during VoxelRenderer initialization";
    }
}

void VoxelRenderer::loadTextures() {
    QString resourcePath = QDir::currentPath() + "/resources/";
    qDebug() << "Looking for textures in:" << resourcePath;
    
    // Load cobblestone texture
    QImage cobblestoneImg(resourcePath + "cobblestone.png");
    if (cobblestoneImg.isNull()) {
        qWarning() << "Failed to load cobblestone texture from" << resourcePath + "cobblestone.png";
        createDefaultTexture("cobblestone");
    } else {
        qDebug() << "Successfully loaded cobblestone texture";
        createTexture("cobblestone", cobblestoneImg);
    }
    
    // Load grass texture
    QImage grassImg(resourcePath + "grass.png");
    if (grassImg.isNull()) {
        qWarning() << "Failed to load grass texture from" << resourcePath + "grass.png";
        createDefaultTexture("grass");
    } else {
        qDebug() << "Successfully loaded grass texture";
        createTexture("grass", grassImg);
    }
    
    // Load dirt texture
    QImage dirtImg(resourcePath + "dirt.png");
    if (dirtImg.isNull()) {
        qWarning() << "Failed to load dirt texture from" << resourcePath + "dirt.png";
        createDefaultTexture("dirt");
    } else {
        qDebug() << "Successfully loaded dirt texture";
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
    m_textures[name] = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_textures[name]->setMinificationFilter(QOpenGLTexture::NearestMipMapNearest);
    m_textures[name]->setMagnificationFilter(QOpenGLTexture::Nearest);
    m_textures[name]->setWrapMode(QOpenGLTexture::ClampToEdge);
    
    // Ensure proper format
    QImage textureImage;
    if (image.format() != QImage::Format_RGBA8888) {
        textureImage = image.convertToFormat(QImage::Format_RGBA8888);
    } else {
        textureImage = image;
    }
    
    // Fix image orientation for OpenGL
    textureImage = textureImage.mirrored();
    
    // Create and allocate texture
    if (!m_textures[name]->create()) {
        qWarning() << "Failed to create texture" << name;
        return;
    }
    
    m_textures[name]->setSize(textureImage.width(), textureImage.height());
    m_textures[name]->setFormat(QOpenGLTexture::RGBA8_UNorm);
    m_textures[name]->allocateStorage();
    
    // Upload the texture data
    m_textures[name]->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, textureImage.constBits());
    
    // Generate mipmaps
    m_textures[name]->generateMipMaps();
    
    qDebug() << "Created texture" << name << "with size" << textureImage.width() << "x" << textureImage.height();
}

void VoxelRenderer::createDefaultTexture(const QString& name) {
    // Create a simple texture for fallback
    QImage defaultImg(16, 16, QImage::Format_RGBA8888);
    defaultImg.fill(Qt::transparent);
    
    QPainter painter(&defaultImg);
    painter.setPen(Qt::NoPen);
    
    // Fill with appropriate color based on material type
    QColor baseColor;
    if (name == "cobblestone") {
        baseColor = QColor(128, 128, 128);
    } else if (name == "grass") {
        baseColor = QColor(0, 128, 0);
    } else if (name == "dirt") {
        baseColor = QColor(139, 69, 19);
    } else {
        baseColor = QColor(255, 0, 255); // Pink for default unknown
    }
    
    // Fill the entire image with the base color
    painter.fillRect(0, 0, 16, 16, baseColor);
    
    // Add some texture details
    QColor detailColor = baseColor.darker(120);
    
    // Draw some noise/pattern
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            // Add some random dots/details
            if ((x + y) % 3 == 0) {
                painter.fillRect(x, y, 1, 1, detailColor);
            }
        }
    }
    
    // If it's cobblestone, draw a grid pattern
    if (name == "cobblestone") {
        painter.setPen(QPen(QColor(100, 100, 100), 1));
        painter.drawLine(0, 4, 16, 4);
        painter.drawLine(0, 11, 16, 11);
        painter.drawLine(4, 0, 4, 16);
        painter.drawLine(11, 0, 11, 16);
    }
    
    // If it's grass, add some blades
    if (name == "grass") {
        painter.setPen(QPen(QColor(0, 180, 0), 1));
        painter.drawLine(2, 0, 2, 5);
        painter.drawLine(7, 0, 7, 7);
        painter.drawLine(12, 0, 12, 6);
    }
    
    painter.end();
    
    createTexture(name, defaultImg);
    
    qDebug() << "Created default texture for" << name;
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
        
        // Skip air voxels (invisible)
        if (voxel.type == VoxelType::Air) continue;
        
        // Create render voxel
        RenderVoxel renderVoxel;
        renderVoxel.pos = pos;
        renderVoxel.color = voxel.color;
        renderVoxel.type = voxel.type;
        
        // Calculate world position to detect chunk boundaries
        QVector3D worldPos = pos.toWorldPos();
        ChunkCoordinate chunkCoord = ChunkCoordinate::fromWorldPosition(worldPos);
        
        // Calculate local coordinates within the chunk
        int localX = static_cast<int>(worldPos.x()) % ChunkCoordinate::CHUNK_SIZE;
        int localY = static_cast<int>(worldPos.y()) % ChunkCoordinate::CHUNK_SIZE;
        int localZ = static_cast<int>(worldPos.z()) % ChunkCoordinate::CHUNK_SIZE;
        
        // Ensure correct handling of negative coordinates
        if (localX < 0) localX += ChunkCoordinate::CHUNK_SIZE;
        if (localY < 0) localY += ChunkCoordinate::CHUNK_SIZE;
        if (localZ < 0) localZ += ChunkCoordinate::CHUNK_SIZE;
        
        // Check if voxel is at a chunk boundary (within 1 voxel of edge)
        bool isChunkBoundary = 
            localX == 0 || localX == ChunkCoordinate::CHUNK_SIZE - 1 ||
            localY == 0 || localY == ChunkCoordinate::CHUNK_SIZE - 1 ||
            localZ == 0 || localZ == ChunkCoordinate::CHUNK_SIZE - 1;
        
        // Always add boundary voxels to ensure seamless rendering
        if (isChunkBoundary) {
            renderVoxel.isBoundary = true;
        }
        
        m_visibleVoxels.push_back(renderVoxel);
    }
    
    m_voxelCount = m_visibleVoxels.size();
}

void VoxelRenderer::render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix) {
    try {
        if (!m_world || !m_shaderProgram) return;
        
        // Combine view and projection matrices (for frustum extraction)
        QMatrix4x4 viewProjection = projectionMatrix * viewMatrix;
        
        // Update view frustum for culling
        if (m_frustumCullingEnabled) {
            m_viewFrustum->update(viewProjection);
        }
        
        // Extract camera position from view matrix (inverse view matrix * origin)
        QMatrix4x4 invView = viewMatrix.inverted();
        QVector3D camPos = invView * QVector3D(0, 0, 0);
        
        // Performance monitor
        static int debugFrameCounter = 0;
        int visibleVoxels = 0;
        int culledVoxels = 0;
        
        // Enable or disable backface culling
        if (m_backfaceCullingEnabled) {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        } else {
            glDisable(GL_CULL_FACE);
        }
        
        // Bind shader
        if (!m_shaderProgram->bind()) {
            qCritical() << "Failed to bind shader program";
            return;
        }
        
        // Set common uniforms
        m_shaderProgram->setUniformValue("view", viewMatrix);
        m_shaderProgram->setUniformValue("projection", projectionMatrix);
        
        // Set camera position for lighting
        m_shaderProgram->setUniformValue("viewPos", camPos);
        
        // Setup lighting
        QVector3D lightPos(0.0f, 10.0f, 0.0f); // Light above center
        m_shaderProgram->setUniformValue("lightPos", lightPos);
        
        // Bind VAO
        m_vao.bind();
        
        // Enable texture unit 0
        glActiveTexture(GL_TEXTURE0);
        m_shaderProgram->setUniformValue("textureSampler", 0);
        
        // Track current bound texture to avoid redundant binds
        GLuint currentTexture = 0;
        
        // Draw each visible voxel
        for (const RenderVoxel& voxel : m_visibleVoxels) {
            // Get world position
            QVector3D worldPos = voxel.pos.toWorldPos();
            
            // Skip if outside frustum - but always render boundary voxels
            // to ensure seamless terrain at chunk boundaries
            if (m_frustumCullingEnabled && !voxel.isBoundary) {
                // Create a bounding sphere for the voxel
                float radius = 0.866f; // Radius of bounding sphere for a unit cube
                
                // Check if voxel is inside the view frustum
                if (!m_viewFrustum->isSphereInside(worldPos, radius)) {
                    culledVoxels++;
                    continue;
                }
            }
            
            visibleVoxels++;
            
            // Set voxel-specific uniforms
            m_shaderProgram->setUniformValue("model", QMatrix4x4());
            m_shaderProgram->setUniformValue("voxelPosition", worldPos);
            m_shaderProgram->setUniformValue("voxelColor", QVector4D(
                voxel.color.redF(), voxel.color.greenF(), voxel.color.blueF(), voxel.color.alphaF()));
            
            // Set texture based on voxel type
            QOpenGLTexture* texture = nullptr;
            bool useTexture = true;
            
            switch (voxel.type) {
                case VoxelType::Cobblestone:
                    texture = m_textures.value("cobblestone", nullptr);
                    break;
                case VoxelType::Grass:
                    texture = m_textures.value("grass", nullptr);
                    break;
                case VoxelType::Dirt:
                    texture = m_textures.value("dirt", nullptr);
                    break;
                case VoxelType::Solid:
                default:
                    texture = m_textures.value("default", nullptr);
                    useTexture = false; // Use color only for generic solids
                    break;
            }
            
            // Bind texture if available and different from current
            if (texture && texture->isCreated()) {
                // Only rebind if it's a different texture
                if (texture->textureId() != currentTexture) {
                    texture->bind();
                    currentTexture = texture->textureId();
                }
                m_shaderProgram->setUniformValue("useTexture", useTexture);
            } else {
                // If no valid texture, use default color
                m_shaderProgram->setUniformValue("useTexture", false);
            }
            
            // Draw cube
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
        }
        
        // Debug output every few seconds
        if (m_frustumCullingEnabled && debugFrameCounter++ % 300 == 0) {
            qDebug() << "Frustum culling stats: Visible:" << visibleVoxels 
                    << "Culled:" << culledVoxels 
                    << "Total:" << (visibleVoxels + culledVoxels)
                    << "Boundary voxels:" << std::count_if(m_visibleVoxels.begin(), m_visibleVoxels.end(), 
                                                         [](const RenderVoxel& v) { return v.isBoundary; });
        }
        
        // Unbind any bound texture
        if (currentTexture != 0) {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        
        // Unbind VAO and shader
        m_vao.release();
        m_shaderProgram->release();
        
        // Disable culling when done
        if (m_backfaceCullingEnabled) {
            glDisable(GL_CULL_FACE);
        }
    } catch (const std::exception& e) {
        qCritical() << "Exception in render function:" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception in render function";
    }
}

void VoxelRenderer::setMaxVisibleChunks(int maxChunks) {
    m_maxVisibleChunks = maxChunks;
}

void VoxelRenderer::setFrustumCullingEnabled(bool enabled) {
    m_frustumCullingEnabled = enabled;
}

void VoxelRenderer::setBackfaceCullingEnabled(bool enabled) {
    m_backfaceCullingEnabled = enabled;
}

void VoxelRenderer::createShaders() {
    // Clean up existing shader if any
    if (m_shaderProgram) {
        delete m_shaderProgram;
        m_shaderProgram = nullptr;
    }
    
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
            
            // Specular (disabled for non-reflective voxels)
            float specularStrength = 0.0;
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
    
    // Cube indices (6 faces, 2 triangles per face, 3 vertices per triangle)
    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,       // Front face
        4, 5, 6, 6, 7, 4,       // Back face
        8, 9, 10, 10, 11, 8,    // Left face
        12, 13, 14, 14, 15, 12, // Right face
        16, 17, 18, 18, 19, 16, // Bottom face
        20, 21, 22, 22, 23, 20  // Top face
    };

    // Bind index buffer
    m_indexBuffer.bind();
    m_indexBuffer.allocate(indices, sizeof(indices));
    
    // Unbind VAO and buffers
    m_vao.release();
    m_vertexBuffer.release();
    m_indexBuffer.release();
}

void VoxelRenderer::updateSettings() {
    if (!m_perfSettings) return;
    
    // Update settings from performance settings
    m_maxVisibleChunks = m_perfSettings->getMaxVisibleChunks();
    m_frustumCullingEnabled = m_perfSettings->isFrustumCullingEnabled();
    m_backfaceCullingEnabled = m_perfSettings->isBackfaceCullingEnabled();
    
    // Debug the frustum culling setting change
    if (m_frustumCullingEnabled && m_viewFrustum) { // Add null check
        qDebug() << "Frustum culling enabled - Debug info:";
        // Dump the first few voxel positions to help with debugging
        int count = std::min(5, (int)m_visibleVoxels.size());
        for (int i = 0; i < count; i++) {
            QVector3D worldPos = m_visibleVoxels[i].pos.toWorldPos();
            bool isInside = m_viewFrustum->isSphereInside(worldPos, 1.0f);
            qDebug() << "  Voxel" << i << "at" << worldPos << "inside frustum:" << isInside;
        }
    } else {
        qDebug() << "Frustum culling disabled";
    }
    
    // Add detailed debug output to show settings being applied
    qDebug() << "VoxelRenderer applying settings:";
    qDebug() << "  - Max Visible Chunks:" << m_maxVisibleChunks;
    qDebug() << "  - Frustum Culling:" << (m_frustumCullingEnabled ? "Enabled" : "Disabled");
    qDebug() << "  - Backface Culling:" << (m_backfaceCullingEnabled ? "Enabled" : "Disabled");
    qDebug() << "  - Occlusion Culling:" << (m_perfSettings->isOcclusionCullingEnabled() ? "Enabled" : "Disabled");
    
    // If world is set, trigger update
    if (m_world) {
        qDebug() << "  - Triggering world render data update";
        updateRenderData();
    }
}