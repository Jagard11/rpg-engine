// src/rendering/gl_arena/gl_arena_direct_rendering.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include <QDebug>
#include <QOpenGLFramebufferObject>

// Global static buffers to prevent recreation
static QOpenGLBuffer s_vbo(QOpenGLBuffer::VertexBuffer);
static QOpenGLVertexArrayObject s_vao;
static bool s_buffersInitialized = false;

// Simplest possible quad drawing
void GLArenaWidget::drawCharacterQuad(QOpenGLTexture* texture, float x, float y, float z, float width, float height)
{
    if (!m_billboardProgram) {
        qWarning() << "Billboard program is null in drawCharacterQuad";
        return;
    }
    
    // Skip if no texture
    bool hasTexture = texture && texture->isCreated();
    if (!hasTexture) {
        return;
    }
    
    // Initialize static buffers if needed (only once)
    if (!s_buffersInitialized) {
        // Define the quad vertices - only position and texture coords
        float vertices[] = {
            -0.5f, -0.5f,  0.0f, 1.0f,  // Bottom left
             0.5f, -0.5f,  1.0f, 1.0f,  // Bottom right
             0.5f,  0.5f,  1.0f, 0.0f,  // Top right
            -0.5f,  0.5f,  0.0f, 0.0f   // Top left
        };
        
        // Create the VAO
        if (!s_vao.create()) {
            qWarning() << "Failed to create static VAO";
            return;
        }
        s_vao.bind();
        
        // Create and populate the VBO
        if (!s_vbo.create()) {
            qWarning() << "Failed to create static VBO";
            s_vao.release();
            return;
        }
        s_vbo.bind();
        s_vbo.allocate(vertices, sizeof(vertices));
        
        // Set up vertex attributes
        glEnableVertexAttribArray(0); // positions
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        
        glEnableVertexAttribArray(1); // texture coordinates
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        
        // Release bindings
        s_vbo.release();
        s_vao.release();
        
        // Mark as initialized
        s_buffersInitialized = true;
    }
    
    // Set uniforms (position, size)
    int posLoc = m_billboardProgram->uniformLocation("position");
    int sizeLoc = m_billboardProgram->uniformLocation("size");
    int texLoc = m_billboardProgram->uniformLocation("textureSampler");
    
    if (posLoc != -1)
        m_billboardProgram->setUniformValue(posLoc, QVector3D(x, y + height/2, z));
    if (sizeLoc != -1)
        m_billboardProgram->setUniformValue(sizeLoc, QVector2D(width, height));
    if (texLoc != -1)
        m_billboardProgram->setUniformValue(texLoc, 0);
    
    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    texture->bind();
    
    // Bind VAO and draw
    s_vao.bind();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    s_vao.release();
    
    // Release texture
    texture->release();
}

void GLArenaWidget::renderCharactersFallback() 
{
    // Extra guard against null objects
    if (!isValid() || !context() || !context()->isValid()) {
        return;
    }
    
    // Skip if shaders aren't available
    if (!m_billboardProgram || !m_billboardProgram->isLinked()) {
        return;
    }
    
    try {
        // Save current OpenGL state
        GLboolean depthTestEnabled;
        glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
        
        GLboolean blendEnabled;
        glGetBooleanv(GL_BLEND, &blendEnabled);
        
        GLint blendSrcRGB, blendDstRGB;
        glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRGB);
        glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRGB);
        
        // Set rendering state
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Bind shader
        m_billboardProgram->bind();
        
        // Set common uniforms
        m_billboardProgram->setUniformValue("view", m_viewMatrix);
        m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
        
        // Make a safe copy of the character map
        QMap<QString, CharacterSprite*> characterCopy = m_characterSprites;
        
        // Render each character
        for (auto it = characterCopy.constBegin(); it != characterCopy.constEnd(); ++it) {
            QString name = it.key();
            CharacterSprite* sprite = it.value();
            
            // Skip null sprites
            if (!sprite) {
                continue;
            }
            
            // Get texture safely
            QOpenGLTexture* texture = sprite->getTexture();
            if (!texture || !texture->isCreated()) {
                continue;
            }
            
            // Get position safely
            QVector3D position(0.0f, 0.0f, 0.0f);
            if (m_gameScene) {
                GameEntity entity = m_gameScene->getEntity(name);
                if (!entity.id.isEmpty()) {
                    position = entity.position;
                }
            }
            
            // Draw character
            drawCharacterQuad(texture, position.x(), position.y(), position.z(), 
                              sprite->width(), sprite->height());
        }
        
        // Cleanup
        m_billboardProgram->release();
        
        // Restore OpenGL state
        if (depthTestEnabled)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
            
        if (blendEnabled)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
            
        glBlendFunc(blendSrcRGB, blendDstRGB);
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in renderCharactersFallback:" << e.what();
        
        // Emergency cleanup
        if (m_billboardProgram && m_billboardProgram->isLinked()) {
            m_billboardProgram->release();
        }
        glEnable(GL_DEPTH_TEST);
    }
    catch (...) {
        qCritical() << "Unknown exception in renderCharactersFallback";
        
        // Emergency cleanup
        if (m_billboardProgram && m_billboardProgram->isLinked()) {
            m_billboardProgram->release();
        }
        glEnable(GL_DEPTH_TEST);
    }
}