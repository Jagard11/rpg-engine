// src/rendering/gl_arena/gl_arena_widget_rendering.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include <QDebug>
#include <QMutex>
#include <stdexcept>
#include <cmath>

// Static mutex for thread safety during rendering
static QMutex renderingMutex;
static int errorCount = 0;  // To limit error reporting

void GLArenaWidget::paintGL()
{
    // Try to lock mutex - skip frame if can't acquire immediately
    if (!renderingMutex.tryLock()) {
        return;
    }
    
    // Ensure mutex is released when function exits
    struct MutexReleaser {
        QMutex& mutex;
        MutexReleaser(QMutex& m) : mutex(m) {}
        ~MutexReleaser() { mutex.unlock(); }
    } releaser(renderingMutex);
    
    // Render uninitialized state
    if (!m_initialized) {
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return;
    }
    
    // Clear any accumulated errors before rendering
    while (glGetError() != GL_NO_ERROR) {
        // Just clear errors, don't log them here
    }
    
    try {
        // Save important OpenGL state
        GLint oldDepthFunc;
        glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);
        
        GLboolean oldDepthTest = glIsEnabled(GL_DEPTH_TEST);
        GLboolean oldBlend = glIsEnabled(GL_BLEND);
        
        GLint oldBlendSrcRGB, oldBlendDstRGB;
        glGetIntegerv(GL_BLEND_SRC_RGB, &oldBlendSrcRGB);
        glGetIntegerv(GL_BLEND_DST_RGB, &oldBlendDstRGB);
        
        // Make sure we have valid objects
        if (!m_playerController) {
            glClearColor(0.3f, 0.0f, 0.0f, 1.0f); // Red for error
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            return;
        }
        
        // Clear the buffer with sky color
        glClearColor(0.2f, 0.3f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Default rendering state
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        
        // Get player position and rotation - with safety checks
        QVector3D playerPos;
        float playerRot = 0.0f;
        
        try {
            playerPos = m_playerController->getPosition();
            playerRot = m_playerController->getRotation();
        } catch (...) {
            playerPos = QVector3D(0, 0, 0);
            playerRot = 0.0f;
        }
        
        // Update view matrix based on player position, rotation and pitch
        m_viewMatrix.setToIdentity();
        
        // Position camera at player's eye level based on stance
        float eyeHeight = playerPos.y() + 1.6f; // Default eye height
        
        // If player controller exists, get actual eye height
        if (m_playerController) {
            eyeHeight = playerPos.y() + m_playerController->getEyeHeight();
        }
        
        QVector3D eyePos = QVector3D(playerPos.x(), eyeHeight, playerPos.z());
        
        // Get player pitch if available
        float playerPitch = 0.0f;
        if (m_playerController) {
            playerPitch = m_playerController->getPitch();
        }
        
        // Calculate look direction based on player rotation (yaw) and pitch
        // First calculate the horizontal direction vector
        QVector3D horizontalDir(cos(playerRot), 0.0f, sin(playerRot));
        
        // Then apply pitch to get the final direction
        // sin(pitch) gives vertical component, cos(pitch) scales horizontal component
        QVector3D lookDir(
            horizontalDir.x() * cos(playerPitch),
            sin(playerPitch),
            horizontalDir.z() * cos(playerPitch)
        );
        
        QVector3D lookAt = eyePos + lookDir;
        
        m_viewMatrix.lookAt(eyePos, lookAt, QVector3D(0.0f, 1.0f, 0.0f));
        
        // Render voxel system if available
        if (m_voxelSystem) {
            try {
                m_voxelSystem->render(m_viewMatrix, m_projectionMatrix);
            } catch (...) {
                // Silent catch - just continue
            }
        }
        
        // Render characters
        try {
            renderCharactersFallback();
        } catch (...) {
            // Silent catch - just continue
        }
        
        // Check for any OpenGL errors (but limit reporting to avoid spam)
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            if (errorCount < 10) {
                qWarning() << "OpenGL error in paintGL:" << err;
                errorCount++;
                if (errorCount == 10) {
                    qWarning() << "Suppressing further OpenGL errors...";
                }
            }
        }
        
        // Restore OpenGL state
        if (oldDepthTest)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
            
        if (oldBlend)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
            
        glBlendFunc(oldBlendSrcRGB, oldBlendDstRGB);
        glDepthFunc(oldDepthFunc);
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in GLArenaWidget::paintGL:" << e.what();
        
        // Emergency drawing
        glClearColor(0.5f, 0.0f, 0.0f, 1.0f); // Bright red for critical error
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    catch (...) {
        qCritical() << "Unknown exception in GLArenaWidget::paintGL";
        
        // Emergency drawing
        glClearColor(0.5f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

// This is now just a stub - actual impl is in fallback mode
void GLArenaWidget::renderCharacters()
{
    renderCharactersFallback();
}

// Also a stub now
void GLArenaWidget::renderCharactersSimple()
{
    renderCharactersFallback();
}