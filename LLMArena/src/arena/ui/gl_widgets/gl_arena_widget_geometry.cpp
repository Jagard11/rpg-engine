// src/arena/ui/gl_widgets/gl_arena_widget_geometry.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include <QDebug>
#include <QImage>
#include <QDir>

// This file contains geometry initialization methods for GLArenaWidget

bool GLArenaWidget::initShaders() {
    // Create shader program for basic rendering
    m_billboardProgram = new QOpenGLShaderProgram();
    
    // Load shader source from resources
    if (!m_billboardProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/basic.vert")) {
        qCritical() << "Failed to load vertex shader:" << m_billboardProgram->log();
        return false;
    }
    
    if (!m_billboardProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/basic.frag")) {
        qCritical() << "Failed to load fragment shader:" << m_billboardProgram->log();
        return false;
    }
    
    // Link the shader program
    if (!m_billboardProgram->link()) {
        qCritical() << "Failed to link shader program:" << m_billboardProgram->log();
        return false;
    }
    
    qDebug() << "Shaders initialized successfully";
    return true;
}

void GLArenaWidget::initializeArena(double radius, double height) {
    if (!m_initialized) {
        qWarning() << "Cannot initialize arena: OpenGL not initialized";
        return;
    }
    
    // Store arena parameters
    m_arenaRadius = radius;
    m_wallHeight = height;
    
    // Create arena geometry
    createArena(radius, height);
    
    // Initialize voxel system
    if (m_voxelSystem) {
        m_voxelSystem->createDefaultWorld();
    }
    
    // Create player entity in the game scene
    if (m_playerController) {
        m_playerController->createPlayerEntity();
        m_playerController->startUpdates();
    }
}

void GLArenaWidget::loadCharacterSprite(const QString& characterName, const QString& texturePath) {
    // Skip if not initialized
    if (!m_initialized) {
        qWarning() << "Cannot load character sprite: OpenGL not initialized";
        return;
    }
    
    // Delete existing sprite if any
    if (m_characterSprites.contains(characterName)) {
        delete m_characterSprites[characterName];
        m_characterSprites.remove(characterName);
    }
    
    try {
        // Create a new sprite
        CharacterSprite* sprite = new CharacterSprite();
        
        // Get character information for sizing
        double width = 1.0;
        double height = 2.0;
        double depth = 0.2;
        
        // Find character data from manager if available
        if (m_characterManager) {
            // Use default dimensions since getCollisionGeometry might not be available
            // Look for a better way to get character dimensions in the future
            width = 1.0;  // Default width
            height = 2.0; // Default height
            depth = 0.2;  // Default depth
        }
        
        // Initialize the sprite
        sprite->init(context(), texturePath, width, height, depth);
        
        // Add to sprite map
        m_characterSprites[characterName] = sprite;
        
        qDebug() << "Loaded character sprite for" << characterName << "with dimensions:" 
                 << width << "x" << height << "x" << depth;
        
        // Add character to game scene if it doesn't exist
        if (m_gameScene) {
            GameEntity entity;
            entity.id = characterName;
            entity.type = "character";
            entity.position = QVector3D(0, height / 2, 0); // Position at origin initially
            entity.dimensions = QVector3D(width, height, depth);
            entity.spritePath = texturePath;
            entity.isStatic = false;
            
            try {
                m_gameScene->addEntity(entity);
            } catch (const std::exception& e) {
                qWarning() << "Failed to add character entity to scene:" << e.what();
            }
        }
    } catch (const std::exception& e) {
        qWarning() << "Exception loading character sprite:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception loading character sprite";
    }
}

void GLArenaWidget::updateCharacterPosition(const QString& characterName, float x, float y, float z) {
    // Skip if not initialized
    if (!m_initialized) {
        return;
    }
    
    // Update character position in sprites
    if (m_characterSprites.contains(characterName)) {
        m_characterSprites[characterName]->updatePosition(x, y, z);
    }
    
    // Update character position in game scene
    if (m_gameScene) {
        try {
            m_gameScene->updateEntityPosition(characterName, QVector3D(x, y, z));
        } catch (const std::exception& e) {
            qWarning() << "Failed to update character position in scene:" << e.what();
        }
    }
    
    // Emit signal for position update
    emit characterPositionUpdated(characterName, x, y, z);
}

void GLArenaWidget::onPlayerPositionChanged(const QVector3D& position) {
    // Update player position in game scene
    if (m_gameScene) {
        try {
            m_gameScene->updateEntityPosition("player", position);
        } catch (const std::exception& e) {
            qWarning() << "Failed to update player position in scene:" << e.what();
        }
    }
    
    // Update voxel streaming if voxel system is available
    if (m_voxelSystem) {
        try {
            m_voxelSystem->streamChunksAroundPlayer(position);
        } catch (const std::exception& e) {
            qWarning() << "Failed to update voxel streaming:" << e.what();
        }
    }
    
    // Emit signal for position update
    emit playerPositionUpdated(position.x(), position.y(), position.z());
}

void GLArenaWidget::onPlayerRotationChanged(float rotation) {
    // Currently, just update the view matrix, which is done in paintGL
    update(); // Request a repaint to update view matrix
}

void GLArenaWidget::onPlayerPitchChanged(float pitch) {
    // Currently, just update the view matrix, which is done in paintGL
    update(); // Request a repaint to update view matrix
}

// Character sprite initialization methods

CharacterSprite::CharacterSprite() : 
    m_texture(nullptr), 
    m_position(0, 0, 0),
    m_width(1.0f),
    m_height(2.0f),
    m_depth(0.2f)
{
    // Initialize buffers
    m_vertexBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_indexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
}

CharacterSprite::~CharacterSprite() {
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
    
    delete m_texture;
}

void CharacterSprite::init(QOpenGLContext* context, const QString& texturePath, 
                          double width, double height, double depth) {
    initializeOpenGLFunctions();
    
    // Store dimensions
    m_width = width;
    m_height = height;
    m_depth = depth;
    
    // Create texture
    QImage image;
    if (!texturePath.isEmpty() && QFile::exists(texturePath)) {
        if (!image.load(texturePath)) {
            qWarning() << "Failed to load character texture:" << texturePath;
            // Use default texture
            image = QImage(16, 32, QImage::Format_RGBA8888);
            image.fill(Qt::magenta);
        }
    } else {
        // Create default texture
        image = QImage(16, 32, QImage::Format_RGBA8888);
        image.fill(Qt::magenta);
    }
    
    // Create OpenGL texture
    m_texture = new QOpenGLTexture(image);
    m_texture->setMinificationFilter(QOpenGLTexture::Nearest);
    m_texture->setMagnificationFilter(QOpenGLTexture::Nearest);
    m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);
    
    // Create VAO
    m_vao.create();
    m_vao.bind();
    
    // Create VBO
    m_vertexBuffer.create();
    m_vertexBuffer.bind();
    
    // Define vertices for a simple quad
    // Position (3) + TexCoord (2)
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f
    };
    
    m_vertexBuffer.allocate(vertices, sizeof(vertices));
    
    // Create IBO
    m_indexBuffer.create();
    m_indexBuffer.bind();
    
    // Define indices for quad (two triangles)
    GLuint indices[] = {
        0, 1, 2,  // First triangle
        0, 2, 3   // Second triangle
    };
    
    m_indexBuffer.allocate(indices, sizeof(indices));
    
    // Set vertex attributes
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), nullptr);
    
    // Texture coordinates
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 
                         reinterpret_cast<void*>(3 * sizeof(GLfloat)));
    
    // Unbind
    m_indexBuffer.release();
    m_vertexBuffer.release();
    m_vao.release();
}

void CharacterSprite::render(QOpenGLShaderProgram* program, QMatrix4x4& viewMatrix, QMatrix4x4& projectionMatrix) {
    if (!program || !program->isLinked() || !m_texture || !m_texture->isCreated() || !m_vao.isCreated()) {
        return;
    }
    
    // Compute billboard orientation vectors
    QVector3D right = QVector3D::crossProduct(QVector3D(0, 1, 0), 
                                             QVector3D(viewMatrix(0, 2), viewMatrix(1, 2), viewMatrix(2, 2))).normalized();
    QVector3D up(0, 1, 0);
    
    // Bind shader program
    program->bind();
    
    // Set uniforms
    program->setUniformValue("view", viewMatrix);
    program->setUniformValue("projection", projectionMatrix);
    program->setUniformValue("cameraRight", right);
    program->setUniformValue("cameraUp", up);
    program->setUniformValue("billboardPos", m_position);
    program->setUniformValue("billboardSize", QVector2D(m_width, m_height));
    
    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    m_texture->bind();
    program->setUniformValue("textureSampler", 0);
    
    // Bind VAO and draw
    m_vao.bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    m_vao.release();
    
    // Unbind texture
    m_texture->release();
    
    // Unbind shader program
    program->release();
}

void CharacterSprite::updatePosition(float x, float y, float z) {
    m_position = QVector3D(x, y, z);
}