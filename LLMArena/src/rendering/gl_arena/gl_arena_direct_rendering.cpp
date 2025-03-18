// src/rendering/gl_arena/gl_arena_direct_rendering.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include <QDebug>

// Draw a quad directly without using VAOs or IBOs
void GLArenaWidget::drawCharacterQuad(QOpenGLTexture* texture, float x, float y, float z, float width, float height)
{
    if (!m_billboardProgram || !texture) {
        return;
    }
    
    qDebug() << "Drawing character quad at" << x << y << z << "with size" << width << height;
    
    // Set shader uniforms
    m_billboardProgram->setUniformValue("position", QVector3D(x, y, z));
    m_billboardProgram->setUniformValue("size", QVector2D(width, height));
    
    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    texture->bind();
    
    // Draw a simple quad (two triangles) using glDrawArrays
    float vertices[] = {
        // Position (2D) + TexCoord
        -0.5f, -0.5f,  0.0f, 1.0f,  // Bottom left
         0.5f, -0.5f,  1.0f, 1.0f,  // Bottom right
         0.5f,  0.5f,  1.0f, 0.0f,  // Top right
        -0.5f,  0.5f,  0.0f, 0.0f   // Top left
    };
    
    // Simple direct rendering without VAO
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), vertices);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), vertices + 2);
    
    // Draw as triangle fan
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    // Cleanup
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    
    // Unbind texture
    texture->release();
}

void GLArenaWidget::renderCharactersFallback() 
{
    qDebug() << "Using ABSOLUTE FALLBACK character rendering method";
    
    // Check for shader program
    if (!m_billboardProgram || !m_billboardProgram->isLinked()) {
        qWarning() << "No valid shader for fallback rendering";
        return;
    }
    
    // Bind shader program
    if (!m_billboardProgram->bind()) {
        qWarning() << "Failed to bind shader for fallback rendering";
        return;
    }
    
    // Set common uniforms
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    m_billboardProgram->setUniformValue("textureSampler", 0);
    
    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Iterate through characters
    for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
        QString name = it.key();
        CharacterSprite* sprite = it.value();
        
        if (!sprite) {
            continue;
        }
        
        // Get texture directly and safely
        QOpenGLTexture* texture = sprite->getTexture();
        if (!texture || !texture->isCreated()) {
            qWarning() << "No valid texture for" << name;
            continue;
        }
        
        // Get position
        float x = 0.0f, y = 0.0f, z = 0.0f;
        try {
            if (m_gameScene) {
                GameEntity entity = m_gameScene->getEntity(name);
                if (!entity.id.isEmpty()) {
                    x = entity.position.x();
                    y = entity.position.y();
                    z = entity.position.z();
                }
            }
        } catch (...) {
            qWarning() << "Failed to get position for" << name;
        }
        
        // Draw quad directly
        try {
            drawCharacterQuad(texture, x, y, z, sprite->width(), sprite->height());
        } catch (const std::exception& e) {
            qWarning() << "Exception drawing quad for" << name << ":" << e.what();
        } catch (...) {
            qWarning() << "Unknown exception drawing quad for" << name;
        }
    }
    
    // Unbind shader
    m_billboardProgram->release();
    
    // Disable blending
    glDisable(GL_BLEND);
}