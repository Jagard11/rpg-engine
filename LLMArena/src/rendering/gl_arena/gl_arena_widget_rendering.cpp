// src/rendering/gl_arena/gl_arena_widget_rendering.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include <QDebug>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QCursor>
#include <QImage>
#include <QMutex>
#include <stdexcept>
#include <cmath>

// Static mutex for thread safety during rendering
static QMutex renderingMutex;
static int errorCount = 0;  // To limit error reporting

// Helper function to output OpenGL info
void printOpenGLInfo() {
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (!context) {
        qWarning() << "No current OpenGL context";
        return;
    }
    
    QString glVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    QString glRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    QString glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    
    qDebug() << "OpenGL Vendor:" << glVendor;
    qDebug() << "OpenGL Renderer:" << glRenderer;
    qDebug() << "OpenGL Version:" << glVersion;
    
    bool hasOpenGL = QOpenGLContext::currentContext()->isValid();
    qDebug() << "OpenGL availability:" << hasOpenGL;
}

// Helper function to update the view matrix within paintGL
// This is not a class method, just a local helper function
void updateViewMatrixFromPlayer(QMatrix4x4& viewMatrix, PlayerController* playerController) {
    if (!playerController) {
        // Default view if no player
        viewMatrix.setToIdentity();
        return;
    }
    
    try {
        // Get player position
        const QVector3D& position = playerController->getPosition();
        float rotation = playerController->getRotation();
        float pitch = playerController->getPitch();
        
        // Get player eye height based on stance
        float eyeHeight = playerController->getEyeHeight();
        
        // Player position with eye height
        QVector3D eyePosition = position + QVector3D(0, eyeHeight, 0);
        
        // Direction vectors
        float cosYaw = cos(rotation);
        float sinYaw = sin(rotation);
        float cosPitch = cos(pitch);
        float sinPitch = sin(pitch);
        
        // Forward, up, and right vectors
        QVector3D forward(sinYaw * cosPitch, -sinPitch, cosYaw * cosPitch);
        QVector3D up(0, 1, 0); // World up
        QVector3D right = QVector3D::crossProduct(forward, up).normalized();
        up = QVector3D::crossProduct(right, forward); // Corrected up
        
        // Set up view matrix
        viewMatrix.setToIdentity();
        viewMatrix.lookAt(
            eyePosition,                // Eye position
            eyePosition + forward,      // Look at point
            up                          // Up vector
        );
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in updateViewMatrix:" << e.what();
        
        // Reset view matrix on failure
        viewMatrix.setToIdentity();
    }
}

void GLArenaWidget::initializeGL() {
    qDebug() << "Initializing OpenGL context...";
    
    // Initialize OpenGL functions BEFORE accessing any OpenGL functions
    // This is critical - OpenGL function pointers must be resolved first
    initializeOpenGLFunctions();
    
    // Add safety checks for OpenGL context
    if (!QOpenGLContext::currentContext() || !QOpenGLContext::currentContext()->isValid()) {
        qCritical() << "Invalid OpenGL context in initializeGL";
        return;
    }
    
    // Now safely get OpenGL information with additional error handling
    QString vendor, renderer, version;
    try {
        const GLubyte* vendorStr = glGetString(GL_VENDOR);
        if (vendorStr) vendor = QString::fromLatin1(reinterpret_cast<const char*>(vendorStr));
        
        const GLubyte* rendererStr = glGetString(GL_RENDERER);
        if (rendererStr) renderer = QString::fromLatin1(reinterpret_cast<const char*>(rendererStr));
        
        const GLubyte* versionStr = glGetString(GL_VERSION);
        if (versionStr) version = QString::fromLatin1(reinterpret_cast<const char*>(versionStr));
    }
    catch (...) {
        qCritical() << "Exception during OpenGL information retrieval";
        return;
    }
    
    // Output the information with null checks
    qDebug() << "OpenGL Vendor:" << (!vendor.isEmpty() ? vendor : "Unknown");
    qDebug() << "OpenGL Renderer:" << (!renderer.isEmpty() ? renderer : "Unknown");
    qDebug() << "OpenGL Version:" << (!version.isEmpty() ? version : "Unknown");
    
    // Check for OpenGL support
    bool hasOpenGL = QOpenGLContext::currentContext()->isValid();
    qDebug() << "OpenGL availability:" << hasOpenGL;
    
    // Set clear color (sky blue)
    glClearColor(0.5f, 0.7f, 1.0f, 1.0f);
    
    // Enable depth testing for 3D rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Enable alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Init shaders - must be done first as other components depend on them
    if (!initShaders()) {
        qCritical() << "Failed to initialize shaders";
        return;
    }
    
    // Set up initial view and projection matrices
    m_projectionMatrix.setToIdentity();
    m_viewMatrix.setToIdentity();
    
    // Set projection matrix (perspective projection)
    float aspectRatio = static_cast<float>(width()) / static_cast<float>(height());
    m_projectionMatrix.perspective(45.0f, aspectRatio, 0.1f, 100.0f);
    
    // Set up view matrix (camera looking down -Z axis from origin)
    m_viewMatrix.lookAt(
        QVector3D(0, 1.5, 5),  // Eye position
        QVector3D(0, 1, 0),    // Look at point
        QVector3D(0, 1, 0)     // Up vector
    );
    
    // Create game scene if not already created
    if (!m_gameScene) {
        m_gameScene = new GameScene(this);
    }
    
    // Create player controller if not already created
    if (!m_playerController) {
        m_playerController = new PlayerController(m_gameScene, this);
        
        // Connect player position updates to view matrix updates
        connect(m_playerController, &PlayerController::positionChanged,
                this, &GLArenaWidget::onPlayerPositionChanged);
        
        connect(m_playerController, &PlayerController::rotationChanged,
                this, &GLArenaWidget::onPlayerRotationChanged);
        
        connect(m_playerController, &PlayerController::pitchChanged,
                this, &GLArenaWidget::onPlayerPitchChanged);
    }
    
    // Initialize voxel system
    try {
        qDebug() << "Creating voxel system...";
        m_voxelSystem = new VoxelSystemIntegration(m_gameScene, this);
        if (!m_voxelSystem) {
            throw std::runtime_error("Failed to create voxel system");
        }
        
        // Make absolutely sure the context is current before initializing voxel system
        if (!QOpenGLContext::currentContext() || !QOpenGLContext::currentContext()->isValid()) {
            qCritical() << "No valid OpenGL context before voxel system initialization";
            makeCurrent();
        }
        
        // Initialize voxel system
        qDebug() << "Initializing voxel system...";
        m_voxelSystem->initialize();
    }
    catch (const std::exception& e) {
        qCritical() << "Failed to initialize voxel system:" << e.what();
    }
    
    // Create character sprites after initialization
    foreach (const QString &name, m_characterSprites.keys()) {
        if (m_characterManager && !name.isEmpty()) {
            // Try to load or refresh character sprite
            CharacterAppearance appearance = m_characterManager->loadCharacterAppearance(name);
            if (!appearance.spritePath.isEmpty()) {
                loadCharacterSprite(name, appearance.spritePath);
            }
        }
    }
    
    // Initialize inventory system
    try {
        qDebug() << "Creating inventory...";
        initializeInventory();
    }
    catch (const std::exception& e) {
        qCritical() << "Failed to initialize inventory system:" << e.what();
    }
    
    // Mark initialization as complete
    m_initialized = true;
    
    // Signal that rendering is initialized
    emit renderingInitialized();
    
    // Hide cursor in game mode
    setCursor(Qt::BlankCursor);
}

void GLArenaWidget::resizeGL(int w, int h) {
    // Check for valid dimensions
    if (w <= 0 || h <= 0) {
        qWarning() << "Invalid resize dimensions:" << w << "x" << h;
        return;
    }
    
    // Update projection matrix with new aspect ratio
    float aspectRatio = static_cast<float>(w) / static_cast<float>(h);
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(45.0f, aspectRatio, 0.1f, 100.0f);
    
    // Update the viewport
    glViewport(0, 0, w, h);
    
    // Re-center the cursor if not in UI mode
    if (m_inventoryUI && !m_inventoryUI->isVisible()) {
        QPoint center(w / 2, h / 2);
        QCursor::setPos(mapToGlobal(center));
    }
}

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
        updateViewMatrixFromPlayer(m_viewMatrix, m_playerController);
        
        // Store these for later use
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
        
        // Draw grid - disable depth test temporarily if needed
        if (m_gridVAO.isCreated() && m_gridVBO.isCreated()) {
            glLineWidth(1.0f);
            
            // Bind shader
            if (m_billboardProgram && m_billboardProgram->bind()) {
                // Set uniforms
                m_billboardProgram->setUniformValue("view", m_viewMatrix);
                m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
                
                // Draw grid
                m_gridVAO.bind();
                m_billboardProgram->setUniformValue("modelView", QMatrix4x4());
                m_billboardProgram->setUniformValue("color", QVector4D(0.5f, 0.5f, 0.5f, 0.5f));
                glDrawArrays(GL_LINES, 0, m_gridVertexCount);
                m_gridVAO.release();
                
                m_billboardProgram->release();
            }
        }
        
        // Draw floor - with enhanced safety checks
        if (m_floorVAO.isCreated() && m_floorVBO.isCreated() && m_floorIBO.isCreated() && m_floorIndexCount > 0) {
            // Bind shader
            if (m_billboardProgram && m_billboardProgram->bind()) {
                // Set uniforms
                m_billboardProgram->setUniformValue("view", m_viewMatrix);
                m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
                
                // Draw floor
                m_floorVAO.bind();
                
                // Double-check VAO binding was successful
                if (m_floorVAO.isCreated() && QOpenGLContext::currentContext() && QOpenGLContext::currentContext()->isValid()) {
                    m_floorIBO.bind();
                    
                    // Double-check IBO binding was successful before drawing
                    if (m_floorIBO.isCreated() && m_floorIBO.bufferId() != 0) {
                        m_billboardProgram->setUniformValue("modelView", QMatrix4x4());
                        m_billboardProgram->setUniformValue("color", QVector4D(0.2f, 0.6f, 0.2f, 1.0f));
                        
                        // Only draw if we have valid index count and buffers
                        glDrawElements(GL_TRIANGLES, m_floorIndexCount, GL_UNSIGNED_INT, nullptr);
                    } else {
                        qWarning() << "Floor IBO not correctly bound for drawing";
                    }
                    
                    // Release IBO even if binding failed
                    m_floorIBO.release();
                } else {
                    qWarning() << "Floor VAO not correctly bound for drawing";
                }
                
                // Release VAO even if binding failed
                m_floorVAO.release();
                m_billboardProgram->release();
            }
        }
        
        // Draw walls - with enhanced safety checks
        for (const auto& wall : m_walls) {
            // First verify we have valid wall objects and a valid index count
            if (wall.vao && wall.vao->isCreated() && 
                wall.ibo && wall.ibo->isCreated() && 
                wall.indexCount > 0) {
                
                // Bind shader with safety check
                if (m_billboardProgram && m_billboardProgram->bind()) {
                    // Set uniforms
                    m_billboardProgram->setUniformValue("view", m_viewMatrix);
                    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
                    
                    // Draw wall with extra error checking
                    if (wall.vao && wall.vao->isCreated()) {
                        wall.vao->bind();
                        
                        // Verify VAO binding was successful
                        if (QOpenGLContext::currentContext() && QOpenGLContext::currentContext()->isValid()) {
                            // Verify IBO is valid and bind it
                            if (wall.ibo && wall.ibo->isCreated()) {
                                wall.ibo->bind();
                                
                                // Verify IBO binding was successful before drawing
                                if (wall.ibo->bufferId() != 0) {
                                    m_billboardProgram->setUniformValue("modelView", QMatrix4x4());
                                    m_billboardProgram->setUniformValue("color", QVector4D(0.7f, 0.7f, 0.7f, 1.0f));
                                    
                                    // Only draw if valid
                                    glDrawElements(GL_TRIANGLES, wall.indexCount, GL_UNSIGNED_INT, nullptr);
                                } else {
                                    qWarning() << "Wall IBO not correctly bound for drawing";
                                }
                                
                                // Release IBO even if binding failed
                                wall.ibo->release();
                            }
                        } else {
                            qWarning() << "No valid OpenGL context during wall drawing";
                        }
                        
                        // Release VAO even if binding failed
                        wall.vao->release();
                    }
                    
                    m_billboardProgram->release();
                }
            }
        }
        
        // Enhanced safety checks for voxel highlighting
        // Only perform raycast if all necessary components are available and initialized
        bool canPerformRaycast = m_voxelSystem && m_voxelSystem->getWorld();
        
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

// World to NDC conversion helper for character sprites
QVector3D GLArenaWidget::worldToNDC(const QVector3D& worldPos) {
    // Convert from world to clip space
    QVector4D clipPos = m_projectionMatrix * m_viewMatrix * QVector4D(worldPos, 1.0);
    
    // Perform perspective division to get normalized device coordinates
    if (qAbs(clipPos.w()) > 0.0001f) {
        return QVector3D(clipPos.x() / clipPos.w(), clipPos.y() / clipPos.w(), clipPos.z() / clipPos.w());
    }
    
    // Handle invalid w (should not happen in practice)
    return QVector3D(clipPos.x(), clipPos.y(), clipPos.z());
}

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