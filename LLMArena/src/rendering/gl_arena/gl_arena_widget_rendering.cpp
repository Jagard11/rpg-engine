// src/rendering/gl_arena/gl_arena_widget_rendering.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include <QDebug>
#include <stdexcept>
#include <cmath>

void GLArenaWidget::paintGL()
{
    if (!m_initialized) {
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        qInfo() << "Widget not initialized yet - nothing to render";
        return;
    }
    
    try {
        // Make sure player controller exists
        if (!m_playerController) {
            qWarning() << "No player controller in paintGL";
            return;
        }
        
        // Clear the buffer before rendering
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Update view matrix based on player position and rotation
        QVector3D playerPos = m_playerController->getPosition();
        float playerRot = m_playerController->getRotation();
        
        m_viewMatrix.setToIdentity();
        
        // Position camera at player's eye level (1.6m above position)
        QVector3D eyePos = playerPos + QVector3D(0.0f, 1.6f, 0.0f);
        
        // Calculate look direction based on player rotation
        QVector3D lookDir(cos(playerRot), 0.0f, sin(playerRot));
        QVector3D lookAt = eyePos + lookDir;
        
        m_viewMatrix.lookAt(eyePos, lookAt, QVector3D(0.0f, 1.0f, 0.0f));
        
        // Render voxel world and sky - with null check and try/catch
        if (m_voxelSystem) {
            try {
                m_voxelSystem->render(m_viewMatrix, m_projectionMatrix);
            } catch (const std::exception& e) {
                qCritical() << "Exception rendering voxel system:" << e.what();
            } catch (...) {
                qCritical() << "Unknown exception rendering voxel system";
            }
        }
        
        // Try using the absolute fallback rendering approach
        renderCharactersFallback();
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in GLArenaWidget::paintGL:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown exception in GLArenaWidget::paintGL";
    }
}

void GLArenaWidget::renderCharacters()
{
    qWarning() << "Old character rendering method called - this is deprecated";
    renderCharactersSimple(); // Use the new, simpler method
}

void GLArenaWidget::renderCharactersSimple()
{
    qDebug() << "Rendering characters with simple method";
    
    // Safety check for OpenGL context
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (!ctx || !ctx->isValid()) {
        qWarning() << "Invalid OpenGL context in renderCharactersSimple";
        return;
    }
    
    // Check if we have any characters to render
    if (m_characterSprites.isEmpty()) {
        qDebug() << "No characters to render";
        return;
    }
    
    // Check if shader program is available
    if (!m_billboardProgram) {
        qWarning() << "No billboard shader program available";
        return;
    }
    
    if (!m_billboardProgram->isLinked()) {
        qWarning() << "Billboard shader program not linked";
        return;
    }
    
    qDebug() << "Rendering" << m_characterSprites.size() << "characters (simple method)";
    
    try {
        // Simplest rendering approach to avoid OpenGL state issues
        
        // Enable required OpenGL features
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Bind shader program - exit if binding fails
        if (!m_billboardProgram->bind()) {
            qWarning() << "Failed to bind billboard shader program";
            return;
        }
        
        // Set common uniforms that don't change per character
        m_billboardProgram->setUniformValue("view", m_viewMatrix);
        m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
        m_billboardProgram->setUniformValue("textureSampler", 0); // Use texture unit 0
        
        // Create a copy of character names to iterate safely
        QStringList characterNames = m_characterSprites.keys();
        
        // Process each character - return on any fatal error
        for (const QString& charName : characterNames) {
            qDebug() << "Rendering character:" << charName;
            
            // Skip if character sprite was removed
            if (!m_characterSprites.contains(charName)) {
                qWarning() << "Character" << charName << "was removed during rendering";
                continue;
            }
            
            // Get character sprite
            CharacterSprite* sprite = m_characterSprites[charName];
            if (!sprite) {
                qWarning() << "Null sprite for character" << charName;
                continue;
            }
            
            // Skip characters with invalid resources
            if (!sprite->hasValidTexture()) {
                qWarning() << "Character" << charName << "has invalid texture";
                continue;
            }
            
            if (!sprite->hasValidVAO()) {
                qWarning() << "Character" << charName << "has invalid VAO";
                continue;
            }
            
            // Get position (default to origin if not found)
            QVector3D charPosition(0.0f, 0.0f, 0.0f);
            
            // Try to get position from game scene
            if (m_gameScene) {
                try {
                    GameEntity entity = m_gameScene->getEntity(charName);
                    if (!entity.id.isEmpty()) {
                        charPosition = entity.position;
                        qDebug() << "Using entity position for" << charName << ":"
                                 << charPosition.x() << charPosition.y() << charPosition.z();
                    } else {
                        qDebug() << "Entity not found in game scene for" << charName;
                    }
                } catch (...) {
                    qWarning() << "Exception getting entity position for" << charName;
                }
            }
            
            // Update position uniform
            m_billboardProgram->setUniformValue("position", charPosition);
            
            // Set size - use fixed values if we can't get from sprite
            float spriteWidth = 1.0f;
            float spriteHeight = 2.0f;
            try {
                spriteWidth = sprite->width();
                spriteHeight = sprite->height();
            } catch (...) {
                qWarning() << "Exception getting sprite dimensions for" << charName;
            }
            
            m_billboardProgram->setUniformValue("size", QVector2D(spriteWidth, spriteHeight));
            
            // Bind texture (carefully)
            QOpenGLTexture* texture = sprite->getTexture();
            if (texture && texture->isCreated()) {
                glActiveTexture(GL_TEXTURE0);
                texture->bind();
                
                // Bind VAO (carefully)
                QOpenGLVertexArrayObject* vao = sprite->getVAO();
                if (vao && vao->isCreated()) {
                    vao->bind();
                    
                    // Draw quad (safely)
                    try {
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
                    } catch (...) {
                        qWarning() << "Exception during glDrawElements for" << charName;
                    }
                    
                    // Unbind VAO
                    vao->release();
                }
                
                // Unbind texture
                texture->release();
            }
        }
        
        // Unbind shader
        m_billboardProgram->release();
        
        // Disable blend when done
        glDisable(GL_BLEND);
        
    } catch (const std::exception& e) {
        qCritical() << "Exception in renderCharactersSimple:" << e.what();
        
        // Clean up OpenGL state in case of exception
        if (m_billboardProgram && m_billboardProgram->isLinked()) {
            m_billboardProgram->release();
        }
        glDisable(GL_BLEND);
    } catch (...) {
        qCritical() << "Unknown exception in renderCharactersSimple";
        
        // Clean up OpenGL state in case of exception
        if (m_billboardProgram && m_billboardProgram->isLinked()) {
            m_billboardProgram->release();
        }
        glDisable(GL_BLEND);
    }
}