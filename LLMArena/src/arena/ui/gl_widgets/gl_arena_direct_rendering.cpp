// src/arena/ui/gl_widgets/gl_arena_direct_rendering.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include <QDebug>
#include <QOpenGLFramebufferObject>

// Global static buffers to prevent recreation
static QOpenGLBuffer* s_vbo = nullptr;
static QOpenGLVertexArrayObject* s_vao = nullptr;
static bool s_buffersInitialized = false;

// Simplest possible quad drawing
void GLArenaWidget::drawCharacterQuad(QOpenGLTexture* texture, float x, float y, float z, float width, float height)
{
    // Enhanced safety checks
    if (!m_billboardProgram || !m_billboardProgram->isLinked()) {
        return;
    }
    
    // Skip if no texture or invalid texture
    bool hasTexture = texture && texture->isCreated();
    if (!hasTexture) {
        return;
    }
    
    try {
        // Initialize static buffers if needed (only once)
        if (!s_buffersInitialized) {
            // First clean up any existing buffers
            if (s_vao) {
                if (s_vao->isCreated()) {
                    s_vao->destroy();
                }
                delete s_vao;
                s_vao = nullptr;
            }
            
            if (s_vbo) {
                if (s_vbo->isCreated()) {
                    s_vbo->destroy();
                }
                delete s_vbo;
                s_vbo = nullptr;
            }
            
            // Define the quad vertices - only position and texture coords
            float vertices[] = {
                -0.5f, -0.5f,  0.0f, 1.0f,  // Bottom left
                 0.5f, -0.5f,  1.0f, 1.0f,  // Bottom right
                 0.5f,  0.5f,  1.0f, 0.0f,  // Top right
                -0.5f,  0.5f,  0.0f, 0.0f   // Top left
            };
            
            // Create VBO and VAO
            s_vao = new QOpenGLVertexArrayObject();
            if (!s_vao) {
                qWarning() << "Failed to allocate static VAO";
                return;
            }
            
            s_vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
            if (!s_vbo) {
                qWarning() << "Failed to allocate static VBO";
                delete s_vao;
                s_vao = nullptr;
                return;
            }
            
            // Create the VAO
            if (!s_vao->create()) {
                qWarning() << "Failed to create static VAO";
                delete s_vao;
                delete s_vbo;
                s_vao = nullptr;
                s_vbo = nullptr;
                return;
            }
            s_vao->bind();
            
            // Create and populate the VBO
            if (!s_vbo->create()) {
                qWarning() << "Failed to create static VBO";
                s_vao->release();
                s_vao->destroy();
                delete s_vao;
                delete s_vbo;
                s_vao = nullptr;
                s_vbo = nullptr;
                return;
            }
            s_vbo->bind();
            s_vbo->allocate(vertices, sizeof(vertices));
            
            // Set up vertex attributes
            glEnableVertexAttribArray(0); // positions
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
            
            glEnableVertexAttribArray(1); // texture coordinates
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
            
            // Release bindings
            s_vbo->release();
            s_vao->release();
            
            // Mark as initialized
            s_buffersInitialized = true;
            qDebug() << "Static quad buffers initialized successfully";
        }
        
        // Check if static buffers are available
        if (!s_vao || !s_vao->isCreated() || !s_vbo || !s_vbo->isCreated()) {
            qWarning() << "Static buffers not properly initialized";
            return;
        }
        
        // Verify uniform locations
        int posLoc = m_billboardProgram->uniformLocation("position");
        int sizeLoc = m_billboardProgram->uniformLocation("size");
        int texLoc = m_billboardProgram->uniformLocation("textureSampler");
        int colorLoc = m_billboardProgram->uniformLocation("color");
        
        if (posLoc == -1 || sizeLoc == -1 || texLoc == -1 || colorLoc == -1) {
            qWarning() << "Missing uniform locations in billboard shader";
            return;
        }
        
        // Set uniforms (position, size)
        m_billboardProgram->setUniformValue(posLoc, QVector3D(x, y + height/2, z));
        m_billboardProgram->setUniformValue(sizeLoc, QVector2D(width, height));
        m_billboardProgram->setUniformValue(texLoc, 0);
        m_billboardProgram->setUniformValue(colorLoc, QVector4D(1.0f, 1.0f, 1.0f, 1.0f));
        
        // Bind texture
        glActiveTexture(GL_TEXTURE0);
        texture->bind();
        
        // Bind VAO and draw
        s_vao->bind();
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        s_vao->release();
        
        // Release texture
        texture->release();
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in drawCharacterQuad:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception in drawCharacterQuad";
    }
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
        if (!m_billboardProgram->bind()) {
            qWarning() << "Failed to bind billboard program";
            return;
        }
        
        // Set common uniforms
        m_billboardProgram->setUniformValue("view", m_viewMatrix);
        m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
        
        // Make a COPY of the character map to avoid issues if it changes during iteration
        QMap<QString, CharacterSprite*> characterCopy;
        for (auto it = m_characterSprites.constBegin(); it != m_characterSprites.constEnd(); ++it) {
            characterCopy.insert(it.key(), it.value());
        }
        
        // Verify we have characters to render
        if (characterCopy.isEmpty()) {
            m_billboardProgram->release();
            return;
        }
        
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
                // Safely get the game entity
                try {
                    GameEntity entity = m_gameScene->getEntity(name);
                    if (!entity.id.isEmpty()) {
                        position = entity.position;
                        
                        // Validate position
                        if (!qIsFinite(position.x()) || !qIsFinite(position.y()) || !qIsFinite(position.z())) {
                            qWarning() << "Invalid entity position for" << name;
                            continue;
                        }
                    }
                } catch (...) {
                    // Silently continue with default position
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

// Initialize inventory system
void GLArenaWidget::initializeInventory() {
    if (!m_initialized) return;
    
    try {
        // Create inventory system first
        qDebug() << "Creating inventory object...";
        m_inventory = new Inventory(this);
        
        if (!m_inventory) {
            qCritical() << "Failed to create inventory object";
            return;
        }
        
        // IMPORTANT: Only create the UI after we have a valid and linked shader program
        if (!m_billboardProgram || !m_billboardProgram->isLinked()) {
            qWarning() << "Deferring inventory UI creation until shaders are ready";
            // We'll initialize the UI later when the shader is ready
            return;
        }
        
        // Create inventory UI
        qDebug() << "Creating inventory UI...";
        m_inventoryUI = new InventoryUI(m_inventory, this);
        
        if (!m_inventoryUI) {
            qCritical() << "Failed to create inventory UI object";
            return;
        }
        
        // Initialize inventory UI with OpenGL context
        if (context() && context()->isValid()) {
            makeCurrent();
            
            try {
                m_inventoryUI->initialize();
            } catch (const std::exception& e) {
                qCritical() << "Exception initializing inventory UI:" << e.what();
            } catch (...) {
                qCritical() << "Unknown exception initializing inventory UI";
            }
            
            doneCurrent();
        } else {
            qWarning() << "No valid OpenGL context for inventory UI initialization";
        }
        
        // Hide cursor by default (only show when inventory is open)
        setCursor(Qt::BlankCursor);
        
        // Connect inventory visibility signal
        connect(m_inventoryUI, &InventoryUI::visibilityChanged, this, &GLArenaWidget::onInventoryVisibilityChanged);
        
        qDebug() << "Inventory system initialized";
    } catch (const std::exception& e) {
        qCritical() << "Failed to initialize inventory system:" << e.what();
    }
}

void GLArenaWidget::onInventoryVisibilityChanged(bool visible) {
    // Show/hide cursor based on inventory visibility
    if (visible) {
        setCursor(Qt::ArrowCursor);
        setMouseTracking(true);
    } else {
        setCursor(Qt::BlankCursor);
        // Recapture cursor in center of window
        QCursor::setPos(mapToGlobal(QPoint(width()/2, height()/2)));
    }
}

// Clean up static OpenGL resources
void cleanupStaticGLResources() {
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (!ctx || !ctx->isValid()) {
        // If no valid context, just delete the pointers
        delete s_vao;
        delete s_vbo;
        s_vao = nullptr;
        s_vbo = nullptr;
        s_buffersInitialized = false;
        return;
    }
    
    // Clean up with valid context
    if (s_vao && s_vao->isCreated()) {
        s_vao->destroy();
    }
    
    if (s_vbo && s_vbo->isCreated()) {
        s_vbo->destroy();
    }
    
    delete s_vao;
    delete s_vbo;
    s_vao = nullptr;
    s_vbo = nullptr;
    s_buffersInitialized = false;
}