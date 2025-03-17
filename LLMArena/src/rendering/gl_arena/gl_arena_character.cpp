// src/rendering/gl_arena/gl_arena_character.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include <QImage>
#include <QDebug>
#include <QPainter>
#include <QFileInfo>
#include <QOpenGLContext>
#include <stdexcept>

CharacterSprite::CharacterSprite()
    : m_texture(nullptr), m_width(1.0f), m_height(2.0f), m_depth(1.0f),
      m_vertexBuffer(QOpenGLBuffer::VertexBuffer), m_indexBuffer(QOpenGLBuffer::IndexBuffer)
{
}

CharacterSprite::~CharacterSprite()
{
    try {
        // Release all OpenGL resources in a safe way
        if (m_vertexBuffer.isCreated()) {
            m_vertexBuffer.destroy();
        }
        if (m_indexBuffer.isCreated()) {
            m_indexBuffer.destroy();
        }
        if (m_vao.isCreated()) {
            m_vao.destroy();
        }
        
        // Delete texture with a safety check - POTENTIAL BUG AREA
        if (m_texture) {
            // Only delete if it's created, otherwise just leak it to avoid crash
            if (m_texture->isCreated()) {
                // BUGFIX: Make sure to detach texture from any active context
                QOpenGLContext* ctx = QOpenGLContext::currentContext();
                if (ctx && ctx->isValid()) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                m_texture->destroy();
            }
            delete m_texture;
            m_texture = nullptr;
        }
    } catch (const std::exception& e) {
        qWarning() << "Exception in CharacterSprite destructor:" << e.what();
        // We can't do much in a destructor except prevent the app from crashing
    }
}

void CharacterSprite::init(QOpenGLContext* context, const QString& texturePath, 
                          double width, double height, double depth)
{
    qDebug() << "Initializing CharacterSprite with texture:" << texturePath;
    
    // Make sure we have a valid context
    if (!context || !context->isValid()) {
        qWarning() << "CharacterSprite init: Invalid OpenGL context";
        return;
    }
    
    // Initialize OpenGL functions with the provided context
    initializeOpenGLFunctions();
    
    m_width = width;
    m_height = height;
    m_depth = depth;
    
    // First delete any existing texture to prevent memory leaks
    if (m_texture) {
        // BUGFIX: Properly clean up old texture before creating a new one
        if (m_texture->isCreated()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            m_texture->destroy();
        }
        delete m_texture;
        m_texture = nullptr;
    }
    
    // Create a new texture
    m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    if (!m_texture) {
        qWarning() << "Failed to allocate texture object";
        return;
    }
    
    // Create a default image for cases where we can't load the requested texture
    QImage defaultImage(64, 128, QImage::Format_RGBA8888);
    defaultImage.fill(Qt::transparent);
    
    // Draw a simple figure for default texture
    QPainter painter(&defaultImage);
    painter.setPen(Qt::black);
    painter.setBrush(QColor(255, 0, 255)); // Pink for default
    painter.fillRect(QRect(0, 0, 64, 128), QColor(255, 0, 255));
    painter.setPen(Qt::white);
    painter.drawRect(16, 16, 32, 96);
    painter.end();
    
    // The actual image we'll use
    QImage textureImage = defaultImage;
    
    // Try to load the requested texture, but only if we have a valid texture path
    if (!texturePath.isEmpty()) {
        // BUGFIX: Use QFileInfo to confirm file exists first
        QFileInfo fileInfo(texturePath);
        if (fileInfo.exists() && fileInfo.isFile()) {
            // Try to load the image safely
            try {
                bool loaded = textureImage.load(texturePath);
                qDebug() << "Loaded image from path:" << texturePath << "success:" << loaded;
                
                if (!loaded || textureImage.isNull()) {
                    qDebug() << "Could not load texture, using default:" << texturePath;
                    textureImage = defaultImage; // Ensure we use the default image
                } else {
                    // Convert to a format suitable for textures if needed
                    if (textureImage.format() != QImage::Format_RGBA8888 && 
                        textureImage.format() != QImage::Format_RGB888) {
                        textureImage = textureImage.convertToFormat(QImage::Format_RGBA8888);
                    }
                }
            } catch (const std::exception& e) {
                qWarning() << "Exception loading image from path:" << e.what();
                textureImage = defaultImage; // Ensure we use the default image
            }
        } else {
            qWarning() << "Texture file does not exist:" << texturePath;
            textureImage = defaultImage;
        }
    }
    
    // BUGFIX: Additional safety checks before creating texture
    if (textureImage.isNull()) {
        qWarning() << "Texture image is null, using default";
        textureImage = defaultImage;
    }
    
    // IMPORTANT: Configure texture parameters BEFORE allocating storage
    // BUGFIX: Add error checking for texture creation
    try {
        if (!m_texture->create()) {
            qWarning() << "Failed to create OpenGL texture";
            return;
        }
        
        m_texture->setFormat(QOpenGLTexture::RGBA8_UNorm);
        m_texture->setMinificationFilter(QOpenGLTexture::Linear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);
        m_texture->setSize(textureImage.width(), textureImage.height());
        
        // Now we can safely set the data
        m_texture->setData(textureImage);
        
        qDebug() << "Successfully created texture" << m_texture->width() << "x" << m_texture->height();
    }
    catch (const std::exception& e) {
        qWarning() << "Exception creating texture:" << e.what();
        if (m_texture) {
            delete m_texture;
            m_texture = nullptr;
        }
        return;
    }
    
    // Create a simple quad for the billboard with a safety-first approach
    // Use a safer static array with explicit size to avoid potential memory issues
    static const float vertices[] = {
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, // Bottom left
         0.5f, -0.5f, 0.0f,  1.0f, 1.0f, // Bottom right
         0.5f,  0.5f, 0.0f,  1.0f, 0.0f, // Top right
        -0.5f,  0.5f, 0.0f,  0.0f, 0.0f  // Top left
    };
    
    static const GLuint indices[] = {
        0, 1, 2,    // First triangle
        2, 3, 0     // Second triangle
    };
    
    // Make sure we have a valid texture before creating buffers
    if (!m_texture->isCreated()) {
        qWarning() << "Cannot create billboard without a valid texture";
        return;
    }
    
    // Initialize OpenGL buffers with try-catch for safety
    try {
        // Clean up any existing buffers
        if (m_vertexBuffer.isCreated()) {
            m_vertexBuffer.destroy();
        }
        if (m_indexBuffer.isCreated()) {
            m_indexBuffer.destroy();
        }
        if (m_vao.isCreated()) {
            m_vao.destroy();
        }
        
        // VAO must be created and bound first
        if (!m_vao.create()) {
            qWarning() << "Failed to create vertex array object";
            return;
        }
        m_vao.bind();
        
        // Create and set up vertex buffer
        if (!m_vertexBuffer.create()) {
            qWarning() << "Failed to create vertex buffer";
            m_vao.release();
            return;
        }
        m_vertexBuffer.bind();
        m_vertexBuffer.allocate(vertices, sizeof(vertices));
        
        // Create and set up index buffer
        if (!m_indexBuffer.create()) {
            qWarning() << "Failed to create index buffer";
            m_vertexBuffer.release();
            m_vao.release();
            return;
        }
        m_indexBuffer.bind();
        m_indexBuffer.allocate(indices, sizeof(indices));
        
        // Position attribute - using our own QOpenGLFunctions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
        
        // Texture coordinate attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
        
        // Clean up
        m_indexBuffer.release();
        m_vertexBuffer.release();
        m_vao.release();
        
        qDebug() << "Successfully initialized billboard for sprite with texture dimensions:" 
                 << textureImage.width() << "x" << textureImage.height();
    } catch (const std::exception& e) {
        qWarning() << "Exception during buffer setup:" << e.what();
        
        // Clean up resources safely
        if (m_indexBuffer.isCreated()) {
            m_indexBuffer.destroy();
        }
        if (m_vertexBuffer.isCreated()) {
            m_vertexBuffer.destroy();
        }
        if (m_vao.isCreated()) {
            m_vao.release();
            m_vao.destroy();
        }
    }
}

void CharacterSprite::updatePosition(float x, float y, float z)
{
    m_position = QVector3D(x, y, z);
}

void CharacterSprite::render(QOpenGLShaderProgram* program, QMatrix4x4& viewMatrix, QMatrix4x4& projectionMatrix)
{
    // More extensive safety checks
    if (!program) {
        qWarning() << "Cannot render CharacterSprite: shader program is null";
        return;
    }
    
    if (!m_texture || !m_texture->isCreated()) {
        qWarning() << "Cannot render CharacterSprite: texture is not valid";
        return;
    }
    
    if (!m_vao.isCreated() || !m_vertexBuffer.isCreated() || !m_indexBuffer.isCreated()) {
        qWarning() << "Cannot render CharacterSprite: buffers not properly initialized";
        return;
    }
    
    // Track binding states
    bool programBound = false;
    bool vaoIsBound = false;
    bool textureBound = false;
    
    try {
        // Bind the shader program
        if (!program->bind()) {
            qWarning() << "Failed to bind shader program";
            return;
        }
        programBound = true;
        
        // Set up camera right and up vectors for billboarding
        QMatrix4x4 view = viewMatrix;
        QVector3D right(view(0, 0), view(1, 0), view(2, 0));
        QVector3D up(view(0, 1), view(1, 1), view(2, 1));
        
        // Set uniform values - check each one is valid in the shader
        if (program->uniformLocation("model") != -1)
            program->setUniformValue("model", QMatrix4x4()); // Identity matrix
        
        if (program->uniformLocation("view") != -1)
            program->setUniformValue("view", viewMatrix);
        
        if (program->uniformLocation("projection") != -1)
            program->setUniformValue("projection", projectionMatrix);
        
        if (program->uniformLocation("cameraRight") != -1)
            program->setUniformValue("cameraRight", right);
        
        if (program->uniformLocation("cameraUp") != -1)
            program->setUniformValue("cameraUp", up);
        
        if (program->uniformLocation("billboardPos") != -1)
            program->setUniformValue("billboardPos", m_position);
        
        if (program->uniformLocation("billboardSize") != -1)
            program->setUniformValue("billboardSize", QVector2D(m_width, m_height));
        
        // BUGFIX: Ensure we're binding to texture unit 0
        glActiveTexture(GL_TEXTURE0);
        
        // Bind texture (bind doesn't return a value, it's void)
        m_texture->bind(0);
        textureBound = true;
        
        // Add error check after binding
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            qWarning() << "Failed to bind texture, GL error:" << err;
            program->release();
            return;
        }
        
        if (program->uniformLocation("textureSampler") != -1)
            program->setUniformValue("textureSampler", 0);
        
        // Bind VAO and draw
        m_vao.bind();
        vaoIsBound = true;
        
        // Draw the billboard quad
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        
        // Release resources in reverse order
        m_vao.release();
        vaoIsBound = false;
        
        m_texture->release();
        textureBound = false;
        
        program->release();
        programBound = false;
    } 
    catch (const std::exception& e) {
        qWarning() << "Exception in CharacterSprite::render:" << e.what();
    }
    
    // Ensure all resources are released even if there was an exception
    if (vaoIsBound) m_vao.release();
    if (textureBound && m_texture) m_texture->release();
    if (programBound && program) program->release();
}

// Implementation of GLArenaWidget character methods

void GLArenaWidget::loadCharacterSprite(const QString& characterName, const QString& texturePath)
{
    if (!m_initialized) {
        qWarning() << "Cannot load character sprite: renderer not initialized";
        return;
    }
    
    qDebug() << "Loading character sprite:" << characterName << "path:" << texturePath;
    
    // Get character dimensions - with safety checks
    CharacterCollisionGeometry geometry;
    geometry.width = 1.0;  // Default values
    geometry.height = 2.0;
    geometry.depth = 1.0;
    
    try {
        if (m_characterManager) {
            CharacterAppearance appearance = m_characterManager->loadCharacterAppearance(characterName);
            geometry = appearance.collision;
        } else {
            qWarning() << "Character manager is null, using default geometry";
        }
    } catch (const std::exception& e) {
        qWarning() << "Error loading character appearance:" << e.what();
        // Continue with default geometry
    }
    
    // Create or update character sprite with robust error handling
    try {
        // Make sure current context is the GLWidget's context
        makeCurrent();
        
        // Check if the context is valid
        if (!context() || !context()->isValid()) {
            qWarning() << "Invalid OpenGL context when trying to load sprite";
            return;
        }
        
        // BUGFIX: Add synchronization point before manipulating OpenGL objects
        glFinish();
        
        // Remove any existing sprite for this character
        if (m_characterSprites.contains(characterName)) {
            try {
                delete m_characterSprites[characterName];
                m_characterSprites.remove(characterName);
            } catch (const std::exception& e) {
                qWarning() << "Exception removing old sprite:" << e.what();
                // Continue with creating new sprite
            }
        }
        
        // Check if texture file exists to avoid trying to load non-existent files
        QString finalTexturePath = texturePath;
        if (!texturePath.isEmpty()) {
            QFileInfo fileInfo(texturePath);
            if (!fileInfo.exists() || !fileInfo.isFile()) {
                qWarning() << "Texture file does not exist:" << texturePath;
                finalTexturePath = ""; // This will cause the default texture to be used
            }
        }
        
        // Create new sprite with a valid context
        CharacterSprite* sprite = new CharacterSprite();
        if (!sprite) {
            qWarning() << "Failed to allocate CharacterSprite";
            doneCurrent();
            return;
        }
        
        // Initialize the sprite
        sprite->init(context(), finalTexturePath, geometry.width, geometry.height, geometry.depth);
        m_characterSprites[characterName] = sprite;
        
        // Position at origin initially
        QVector3D initialPos(0.0f, 0.0f, 0.0f);
        sprite->updatePosition(initialPos.x(), initialPos.y(), initialPos.z());
        
        // BUGFIX: Add synchronization point before releasing context
        glFinish();
        
        // Make sure we release the context before emitting signals or updating
        doneCurrent();
        
        // Emit position update signal (must be done after doneCurrent)
        emit characterPositionUpdated(characterName, initialPos.x(), initialPos.y(), initialPos.z());
        
        // Request a repaint
        update();
    } catch (const std::exception& e) {
        qWarning() << "Exception during character sprite loading:" << e.what();
        
        // Ensure we release the context
        if (context() && context()->isValid()) {
            doneCurrent();
        }
    }
}

void GLArenaWidget::updateCharacterPosition(const QString& characterName, float x, float y, float z)
{
    if (!m_characterSprites.contains(characterName)) return;
    
    CharacterSprite* sprite = m_characterSprites[characterName];
    if (sprite) {
        sprite->updatePosition(x, y, z);
        emit characterPositionUpdated(characterName, x, y, z);
        update();
    }
}