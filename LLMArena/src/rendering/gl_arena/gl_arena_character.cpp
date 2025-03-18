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
    qDebug() << "CharacterSprite constructor called";
}

CharacterSprite::~CharacterSprite()
{
    // Safe cleanup with explicit error handling
    try {
        // Only operate on OpenGL objects if we have a valid context
        QOpenGLContext* ctx = QOpenGLContext::currentContext();
        if (!ctx || !ctx->isValid()) {
            qWarning() << "No valid OpenGL context in CharacterSprite destructor";
            return;
        }
        
        // Release OpenGL resources in a safe way
        if (m_vertexBuffer.isCreated()) {
            qDebug() << "Destroying vertex buffer";
            m_vertexBuffer.destroy();
        }
        
        if (m_indexBuffer.isCreated()) {
            qDebug() << "Destroying index buffer";
            m_indexBuffer.destroy();
        }
        
        if (m_vao.isCreated()) {
            qDebug() << "Destroying VAO";
            m_vao.destroy();
        }
        
        // Only delete texture if it exists
        if (m_texture) {
            // Only destroy if it's created to avoid crash
            if (m_texture->isCreated()) {
                qDebug() << "Destroying texture";
                // Ensure we're not bound to any texture unit
                QOpenGLFunctions* f = ctx->functions();
                if (f) {
                    f->glActiveTexture(GL_TEXTURE0);
                    f->glBindTexture(GL_TEXTURE_2D, 0);
                }
                
                // Now safe to destroy texture
                m_texture->destroy();
            }
            
            // Delete the texture object itself
            qDebug() << "Deleting texture object";
            delete m_texture;
            m_texture = nullptr;
        }
    } catch (const std::exception& e) {
        qWarning() << "Exception in CharacterSprite destructor:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception in CharacterSprite destructor";
    }
}

void CharacterSprite::init(QOpenGLContext* context, const QString& texturePath, 
                          double width, double height, double depth)
{
    qDebug() << "Initializing CharacterSprite with texture:" << texturePath;
    
    // Bail out if no context
    if (!context || !context->isValid()) {
        qWarning() << "CharacterSprite init: Invalid OpenGL context";
        return;
    }
    
    // Initialize OpenGL functions with the provided context
    initializeOpenGLFunctions();
    
    m_width = width;
    m_height = height;
    m_depth = depth;
    
    // Clean up any existing texture first
    if (m_texture) {
        if (m_texture->isCreated()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            m_texture->destroy();
        }
        delete m_texture;
        m_texture = nullptr;
    }
    
    try {
        // Start with default texture
        QImage textureImage(128, 256, QImage::Format_RGBA8888);
        textureImage.fill(QColor(255, 0, 255)); // Pink for default
        
        // Add text to default texture
        QPainter painter(&textureImage);
        painter.setPen(Qt::white);
        painter.drawText(10, 128, "DEFAULT");
        painter.end();
        
        // Try to load the requested texture if path is valid
        if (!texturePath.isEmpty()) {
            QFileInfo fileInfo(texturePath);
            if (fileInfo.exists() && fileInfo.isFile()) {
                QImage loadedImage;
                if (loadedImage.load(texturePath)) {
                    qDebug() << "Successfully loaded image from" << texturePath;
                    // Convert to RGBA format if needed
                    if (loadedImage.format() != QImage::Format_RGBA8888) {
                        textureImage = loadedImage.convertToFormat(QImage::Format_RGBA8888);
                    } else {
                        textureImage = loadedImage;
                    }
                } else {
                    qWarning() << "Failed to load texture from" << texturePath;
                }
            } else {
                qWarning() << "Texture file does not exist:" << texturePath;
            }
        }
        
        // Create texture object
        m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        
        // Set up texture parameters
        m_texture->setFormat(QOpenGLTexture::RGBA8_UNorm);
        m_texture->setMinificationFilter(QOpenGLTexture::Linear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);
        
        // Create and upload the texture
        if (!m_texture->create()) {
            qWarning() << "Failed to create OpenGL texture";
            delete m_texture;
            m_texture = nullptr;
            return;
        }
        
        m_texture->setData(textureImage);
        
        qDebug() << "Successfully created texture" << textureImage.width() << "x" << textureImage.height();
        
        // Now set up the vertex/index data for billboard quad
        // Create quad buffer data - simplified with minimal data
        float vertices[] = {
            -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, // Bottom left
             0.5f, -0.5f, 0.0f,  1.0f, 1.0f, // Bottom right
             0.5f,  0.5f, 0.0f,  1.0f, 0.0f, // Top right
            -0.5f,  0.5f, 0.0f,  0.0f, 0.0f  // Top left
        };
        
        const GLuint indices[] = {
            0, 1, 2,    // First triangle
            2, 3, 0     // Second triangle
        };
        
        // Clean up existing buffers
        if (m_vertexBuffer.isCreated()) {
            m_vertexBuffer.destroy();
        }
        if (m_indexBuffer.isCreated()) {
            m_indexBuffer.destroy();
        }
        if (m_vao.isCreated()) {
            m_vao.destroy();
        }
        
        // Create VAO
        if (!m_vao.create()) {
            qWarning() << "Failed to create VAO";
            return;
        }
        m_vao.bind();
        
        // Create VBO
        if (!m_vertexBuffer.create()) {
            qWarning() << "Failed to create VBO";
            m_vao.release();
            return;
        }
        m_vertexBuffer.bind();
        m_vertexBuffer.allocate(vertices, sizeof(vertices));
        
        // Create IBO
        if (!m_indexBuffer.create()) {
            qWarning() << "Failed to create IBO";
            m_vertexBuffer.release();
            m_vao.release();
            return;
        }
        m_indexBuffer.bind();
        m_indexBuffer.allocate(indices, sizeof(indices));
        
        // Set up vertex attributes
        glEnableVertexAttribArray(0); // Position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
        
        glEnableVertexAttribArray(1); // Texture coordinates
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 
                            reinterpret_cast<void*>(3 * sizeof(float)));
        
        // Cleanup
        m_indexBuffer.release();
        m_vertexBuffer.release();
        m_vao.release();
        
        qDebug() << "Successfully initialized billboard geometry";
    } catch (const std::exception& e) {
        qWarning() << "Exception in CharacterSprite::init:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception in CharacterSprite::init";
    }
}

void CharacterSprite::updatePosition(float x, float y, float z)
{
    m_position = QVector3D(x, y, z);
}

void CharacterSprite::render(QOpenGLShaderProgram* program, QMatrix4x4& viewMatrix, QMatrix4x4& projectionMatrix)
{
    // This method is no longer used, as rendering is handled directly in GLArenaWidget::renderCharacters
    // Kept for API compatibility
    qDebug() << "CharacterSprite::render called but is deprecated";
    
    // Skip render if shader, texture or VAO are invalid
    if (!program || !program->isLinked()) {
        qWarning() << "Invalid shader program in CharacterSprite::render";
        return;
    }
    
    if (!m_texture || !m_texture->isCreated()) {
        qWarning() << "Invalid texture in CharacterSprite::render";
        return;
    }
    
    if (!m_vao.isCreated()) {
        qWarning() << "Invalid VAO in CharacterSprite::render";
        return;
    }
    
    qDebug() << "CharacterSprite::render - this method is no longer used";
}