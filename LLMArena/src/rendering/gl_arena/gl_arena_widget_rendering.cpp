// src/rendering/gl_arena/gl_arena_widget_rendering.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include <QDebug>
#include <stdexcept>
#include <cmath>  // Added for cos() and sin() functions

void GLArenaWidget::paintGL()
{
    if (!m_initialized) {
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return;
    }
    
    try {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Make sure player controller exists
        if (!m_playerController) {
            qWarning() << "No player controller in paintGL!";
            return;
        }
        
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
        
        // Render scene
        renderArena();
        
        // Render characters
        renderCharacters();
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in GLArenaWidget::paintGL:" << e.what();
    }
}

void GLArenaWidget::renderArena()
{
    if (!m_basicProgram || !m_gridProgram || !m_initialized) return;
    
    // Render floor
    m_basicProgram->bind();
    
    QMatrix4x4 modelMatrix;
    modelMatrix.setToIdentity();
    
    m_basicProgram->setUniformValue("model", modelMatrix);
    m_basicProgram->setUniformValue("view", m_viewMatrix);
    m_basicProgram->setUniformValue("projection", m_projectionMatrix);
    m_basicProgram->setUniformValue("objectColor", QVector3D(0.5f, 0.5f, 0.5f)); // Gray floor
    
    // Set lighting parameters
    QVector3D playerPos = m_playerController->getPosition();
    QVector3D eyePos = playerPos + QVector3D(0.0f, 1.6f, 0.0f);
    m_basicProgram->setUniformValue("lightPos", QVector3D(0.0f, 10.0f, 0.0f)); // Light above the arena
    m_basicProgram->setUniformValue("viewPos", eyePos);
    
    m_floorVAO.bind();
    glDrawElements(GL_TRIANGLES, m_floorIndexCount, GL_UNSIGNED_INT, nullptr);
    m_floorVAO.release();
    
    // Render walls
    for (int i = 0; i < m_walls.size(); i++) {
        const WallGeometry& wall = m_walls[i];
        
        // Set wall color
        m_basicProgram->setUniformValue("objectColor", QVector3D(0.7f, 0.7f, 0.7f)); // Light gray walls
        
        wall.vao->bind();
        glDrawElements(GL_TRIANGLES, wall.indexCount, GL_UNSIGNED_INT, nullptr);
        wall.vao->release();
    }
    
    m_basicProgram->release();
    
    // Render grid
    m_gridProgram->bind();
    
    m_gridProgram->setUniformValue("model", modelMatrix);
    m_gridProgram->setUniformValue("view", m_viewMatrix);
    m_gridProgram->setUniformValue("projection", m_projectionMatrix);
    m_gridProgram->setUniformValue("lineColor", QVector3D(0.3f, 0.3f, 0.3f)); // Dark gray grid
    
    m_gridVAO.bind();
    glDrawArrays(GL_LINES, 0, m_gridVertexCount);
    m_gridVAO.release();
    
    m_gridProgram->release();
}

void GLArenaWidget::renderCharacters()
{
    if (!m_billboardProgram || !m_initialized) {
        return;
    }
    
    // Safety check for OpenGL context
    if (!context() || !context()->isValid()) {
        qWarning() << "Invalid OpenGL context in renderCharacters";
        return;
    }
    
    try {
        // First, make sure the billboard program is compiled and linked
        if (!m_billboardProgram->isLinked()) {
            qWarning() << "Billboard program is not linked, cannot render characters";
            return;
        }
        
        // BUGFIX: Clear any potential GL errors before rendering
        // This can help identify if errors are coming from previous operations
        while (glGetError() != GL_NO_ERROR) { /* Clear error queue */ }
        
        // Enable depth test but disable depth write for transparent billboards
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        
        // Bind the program once for all sprites
        if (!m_billboardProgram->bind()) {
            qWarning() << "Failed to bind billboard program";
            return;
        }
        
        // Set up common shader uniforms
        if (m_billboardProgram->uniformLocation("view") != -1)
            m_billboardProgram->setUniformValue("view", m_viewMatrix);
        
        if (m_billboardProgram->uniformLocation("projection") != -1)
            m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
        
        // Extract camera vectors for billboarding from view matrix
        QMatrix4x4 view = m_viewMatrix;
        QVector3D right(view(0, 0), view(1, 0), view(2, 0));
        QVector3D up(view(0, 1), view(1, 1), view(2, 1));
        
        if (m_billboardProgram->uniformLocation("cameraRight") != -1)
            m_billboardProgram->setUniformValue("cameraRight", right);
        
        if (m_billboardProgram->uniformLocation("cameraUp") != -1)
            m_billboardProgram->setUniformValue("cameraUp", up);
        
        // Render each character sprite one by one
        for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
            CharacterSprite* sprite = it.value();
            if (!sprite) continue;
            
            try {
                // Use sprite's own render method to handle its specific rendering
                sprite->render(m_billboardProgram, m_viewMatrix, m_projectionMatrix);
                
                // Check for OpenGL errors after rendering each sprite
                GLenum err = glGetError();
                if (err != GL_NO_ERROR) {
                    qWarning() << "OpenGL error during character rendering:" << err;
                }
            }
            catch (const std::exception& e) {
                qWarning() << "Exception rendering character" << it.key() << ":" << e.what();
                // Continue with next sprite
            }
        }
        
        // Unbind the program when done with all sprites
        m_billboardProgram->release();
    } 
    catch (const std::exception& e) {
        qWarning() << "Exception in renderCharacters:" << e.what();
        
        // Ensure shader program is released
        if (m_billboardProgram && m_billboardProgram->isLinked()) {
            m_billboardProgram->release();
        }
    }
}