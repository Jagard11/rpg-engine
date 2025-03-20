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
    
    // Additional early validity checks
    if (!context() || !context()->isValid()) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return;
    }
    
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
        QVector3D playerPos(0, 0, 0);
        float playerRot = 0.0f;
        float playerPitch = 0.0f;
        
        try {
            if (m_playerController) {
                playerPos = m_playerController->getPosition();
                playerRot = m_playerController->getRotation();
                playerPitch = m_playerController->getPitch();
            }
        } catch (...) {
            // Silently use default values
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
        
        // Enhanced safety checks for voxel highlighting
        // Only perform raycast if all necessary components are available and initialized
        bool canPerformRaycast = m_voxelSystem && m_voxelSystem->getWorld() && 
                                m_inventoryUI && m_inventoryUI->hasVoxelTypeSelected();
        
        if (canPerformRaycast) {
            try {
                raycastVoxels(eyePos, lookDir);
            } catch (const std::exception& e) {
                qWarning() << "Exception in raycastVoxels:" << e.what();
                // Ensure highlight is reset on error
                m_highlightedVoxelFace = -1;
            } catch (...) {
                qWarning() << "Unknown exception in raycastVoxels";
                m_highlightedVoxelFace = -1;
            }
        }
        
        // Render voxel system if available - with added protection
        if (m_voxelSystem) {
            try {
                // Double-check for null pointers
                if (m_voxelSystem->getWorld()) {
                    m_voxelSystem->render(m_viewMatrix, m_projectionMatrix);
                }
            } catch (const std::exception& e) {
                qWarning() << "Exception in voxel system render:" << e.what();
            } catch (...) {
                qWarning() << "Unknown exception in voxel system render";
            }
        }
        
        // Render characters - with safety check
        try {
            // Additional check to ensure we have everything needed
            if (m_characterSprites.size() > 0 && context() && context()->isValid() && m_billboardProgram && m_billboardProgram->isLinked()) {
                renderCharactersFallback();
            }
        } catch (const std::exception& e) {
            qWarning() << "Exception in renderCharactersFallback:" << e.what();
        } catch (...) {
            qWarning() << "Unknown exception in renderCharactersFallback";
        }
        
        // Enhanced safety checks for voxel highlight rendering
        // Only render highlight if all necessary components are available and initialized
        bool canRenderHighlight = m_voxelSystem && m_voxelSystem->getWorld() && 
                                 m_inventoryUI && m_inventoryUI->hasVoxelTypeSelected() && 
                                 m_highlightedVoxelFace >= 0 && m_highlightedVoxelFace < 6;
        
        if (canRenderHighlight) {
            try {
                renderVoxelHighlight();
            } catch (const std::exception& e) {
                qWarning() << "Exception in renderVoxelHighlight:" << e.what();
            } catch (...) {
                qWarning() << "Unknown exception in renderVoxelHighlight";
            }
        }
        
        // Render inventory UI (including action bar)
        if (m_inventoryUI) {
            try {
                // Additional check to ensure we have everything needed
                if (context() && context()->isValid()) {
                    // Switch to 2D mode for UI rendering
                    glDisable(GL_DEPTH_TEST);
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    
                    // Use ortho projection for UI
                    QMatrix4x4 uiProjection;
                    uiProjection.ortho(0, width(), height(), 0, -1, 1);
                    
                    // Save original matrices
                    QMatrix4x4 savedProjection = m_projectionMatrix;
                    QMatrix4x4 savedView = m_viewMatrix;
                    
                    // Render UI with 2D projection
                    m_projectionMatrix = uiProjection;
                    m_viewMatrix.setToIdentity();
                    
                    // Now render the UI elements
                    renderInventory();
                    
                    // Restore matrices
                    m_projectionMatrix = savedProjection;
                    m_viewMatrix = savedView;
                }
            } catch (const std::exception& e) {
                qWarning() << "Exception in inventory UI rendering:" << e.what();
            } catch (...) {
                qWarning() << "Unknown exception in inventory UI rendering";
            }
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

// Function stubs that now call the fallback rendering
void GLArenaWidget::renderCharacters()
{
    // Add extra safety check
    if (m_characterSprites.size() > 0 && context() && context()->isValid() && m_billboardProgram && m_billboardProgram->isLinked()) {
        renderCharactersFallback();
    }
}

void GLArenaWidget::renderCharactersSimple()
{
    // Add extra safety check
    if (m_characterSprites.size() > 0 && context() && context()->isValid() && m_billboardProgram && m_billboardProgram->isLinked()) {
        renderCharactersFallback();
    }
}

// Render inventory UI
void GLArenaWidget::renderInventory() {
    if (!m_inventoryUI) {
        return;
    }
    
    try {
        // Additional context check
        if (!context() || !context()->isValid()) {
            return;
        }
    
        // Save current OpenGL state
        GLboolean depthTestEnabled;
        glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
        
        GLint blendSrc, blendDst;
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrc);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDst);
        
        // Setup for 2D UI rendering
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Verify width and height are valid
        int w = width();
        int h = height();
        if (w <= 0 || h <= 0) {
            qWarning() << "Invalid widget dimensions for inventory UI:" << w << "x" << h;
            return;
        }
        
        // Render inventory UI
        m_inventoryUI->render(w, h);
        
        // Restore OpenGL state
        if (depthTestEnabled) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        
        glBlendFunc(blendSrc, blendDst);
    } catch (const std::exception& e) {
        qCritical() << "Exception in renderInventory:" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception in renderInventory";
    }
}