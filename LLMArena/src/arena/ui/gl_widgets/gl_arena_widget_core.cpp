// src/arena/ui/gl_widgets/gl_arena_widget_core.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include "../../../../include/arena/debug/debug_system.h"
#include <QDebug>
#include <QCursor>
#include <QtMath>

GLArenaWidget::GLArenaWidget(CharacterManager* charManager, QWidget* parent)
    : QOpenGLWidget(parent),
      m_characterManager(charManager),
      m_gameScene(nullptr),
      m_playerController(nullptr),
      m_voxelSystem(nullptr),
      m_billboardProgram(nullptr),
      m_initialized(false),
      m_arenaRadius(10.0),
      m_wallHeight(5.0),
      m_highlightedVoxelFace(-1),
      m_maxPlacementDistance(5.0f)
{
    // Set up widget attributes
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    
    // Create game scene
    m_gameScene = new GameScene(this);
    
    // Create player controller
    m_playerController = new PlayerController(m_gameScene, this);
    
    // Connect player controller signals
    connect(m_playerController, &PlayerController::positionChanged,
            this, &GLArenaWidget::onPlayerPositionChanged);
    
    connect(m_playerController, &PlayerController::rotationChanged,
            this, &GLArenaWidget::onPlayerRotationChanged);
    
    connect(m_playerController, &PlayerController::pitchChanged,
            this, &GLArenaWidget::onPlayerPitchChanged);
    
    // Create voxel system
    try {
        qDebug() << "Creating VoxelSystemIntegration...";
        m_voxelSystem = new VoxelSystemIntegration(m_gameScene, this);
        qDebug() << "VoxelSystemIntegration created successfully";
    } catch (const std::exception& e) {
        qWarning() << "Failed to create voxel system:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception creating voxel system";
    }
    
    // Hide cursor initially
    updateMouseTrackingState();
}

GLArenaWidget::~GLArenaWidget() {
    // Clean up OpenGL resources
    makeCurrent();
    
    // Delete character sprites
    for (auto sprite : m_characterSprites) {
        delete sprite;
    }
    m_characterSprites.clear();
    
    // Clean up buffers
    if (m_floorVAO.isCreated()) {
        m_floorVAO.destroy();
    }
    
    if (m_floorVBO.isCreated()) {
        m_floorVBO.destroy();
    }
    
    if (m_floorIBO.isCreated()) {
        m_floorIBO.destroy();
    }
    
    if (m_gridVAO.isCreated()) {
        m_gridVAO.destroy();
    }
    
    if (m_gridVBO.isCreated()) {
        m_gridVBO.destroy();
    }
    
    // Clean up walls
    for (auto& wall : m_walls) {
        if (wall.vao && wall.vao->isCreated()) {
            wall.vao->destroy();
        }
        
        if (wall.vbo && wall.vbo->isCreated()) {
            wall.vbo->destroy();
        }
        
        if (wall.ibo && wall.ibo->isCreated()) {
            wall.ibo->destroy();
        }
    }
    
    m_walls.clear();
    
    // Delete shader program
    delete m_billboardProgram;
    
    // Debug system is deleted automatically through unique_ptr
    
    doneCurrent();
}

void GLArenaWidget::initializeGL() {
    // Initialize OpenGL functions
    initializeOpenGLFunctions();
    
    // Set up basic OpenGL state
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Initialize shaders
    if (!initShaders()) {
        qCritical() << "Failed to initialize shaders!";
        return;
    }
    
    // Initialize voxel system
    if (m_voxelSystem) {
        try {
            qDebug() << "Initializing VoxelSystemIntegration...";
            m_voxelSystem->initialize();
            qDebug() << "VoxelSystemIntegration initialization complete";
        } catch (const std::exception& e) {
            qWarning() << "Failed to initialize voxel system:" << e.what();
        } catch (...) {
            qWarning() << "Unknown exception initializing voxel system";
        }
    }
    
    // Initialize inventory
    try {
        initializeInventory();
    } catch (const std::exception& e) {
        qWarning() << "Failed to initialize inventory:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception initializing inventory";
    }
    
    // Mark as initialized
    m_initialized = true;
    
    // Initialize debug system
    try {
        initializeDebugSystem();
        qDebug() << "Debug system initialized successfully";
    } catch (const std::exception& e) {
        qWarning() << "Failed to initialize debug system:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception initializing debug system";
    }
    
    // Set player screen dimensions
    if (m_playerController) {
        m_playerController->setScreenDimensions(width(), height());
    }
    
    // Create default world
    if (m_voxelSystem) {
        qDebug() << "Creating default world...";
        m_voxelSystem->createDefaultWorld();
    }
    
    // Emit initialization signal
    emit renderingInitialized();
}

void GLArenaWidget::resizeGL(int w, int h) {
    // Update projection matrix
    float aspectRatio = static_cast<float>(w) / std::max(1, h);
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(70.0f, aspectRatio, 0.1f, 1000.0f);
    
    // Update player controller screen dimensions
    if (m_playerController) {
        m_playerController->setScreenDimensions(w, h);
    }
    
    // Update inventory UI
    if (m_inventoryUI) {
        // Nothing specific to do here, as inventory UI gets screen dimensions during rendering
    }
}

void GLArenaWidget::paintGL() {
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Skip if not initialized
    if (!m_initialized) {
        return;
    }
    
    // Calculate view matrix from player controller
    m_viewMatrix.setToIdentity();
    
    if (m_playerController) {
        // Get player position and rotation
        QVector3D position = m_playerController->getPosition();
        float rotation = m_playerController->getRotation();
        float pitch = m_playerController->getPitch();
        
        // Create view matrix (reversed order of transformations)
        m_viewMatrix.rotate(pitch * 180.0f / M_PI, 1, 0, 0); // Pitch around X axis
        m_viewMatrix.rotate(rotation * 180.0f / M_PI, 0, 1, 0); // Yaw around Y axis
        m_viewMatrix.translate(-position);
    }
    
    // Render voxel system first (includes skybox)
    if (m_voxelSystem) {
        try {
            m_voxelSystem->render(m_viewMatrix, m_projectionMatrix);
        } catch (const std::exception& e) {
            qWarning() << "Exception in voxel system rendering:" << e.what();
        } catch (...) {
            qWarning() << "Unknown exception in voxel system rendering";
        }
    }
    
    // Render floor
    renderFloor();
    
    // Render grid
    renderGrid();
    
    // Render walls
    renderWalls();
    
    // Render characters
    try {
        renderCharacters();
    } catch (const std::exception& e) {
        qWarning() << "Exception in character rendering:" << e.what();
        
        try {
            // Try fallback rendering
            renderCharactersFallback();
        } catch (...) {
            // Give up if even the fallback fails
        }
    } catch (...) {
        qWarning() << "Unknown exception in character rendering";
        
        try {
            // Try fallback rendering
            renderCharactersFallback();
        } catch (...) {
            // Give up if even the fallback fails
        }
    }
    
    // Render voxel highlight if needed
    if (m_highlightedVoxelFace >= 0) {
        try {
            renderVoxelHighlight();
        } catch (const std::exception& e) {
            qWarning() << "Exception in voxel highlight rendering:" << e.what();
        } catch (...) {
            qWarning() << "Unknown exception in voxel highlight rendering";
        }
    }
    
    // Render inventory UI
    try {
        renderInventory();
    } catch (const std::exception& e) {
        qWarning() << "Exception in inventory rendering:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception in inventory rendering";
    }
    
    // Render debug system
    try {
        renderDebugSystem();
    } catch (const std::exception& e) {
        qWarning() << "Exception in debug system rendering:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception in debug system rendering";
    }
}

void GLArenaWidget::keyPressEvent(QKeyEvent* event) {
    // First, check if debug system wants to handle this
    if (m_debugSystem && processDebugKeyEvent(event)) {
        return;
    }
    
    // Handle inventory UI
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleKeyPress(event->key());
        return;
    }
    
    // Handle voxel placement
    if (event->key() == Qt::Key_E && m_highlightedVoxelFace >= 0 && m_voxelSystem) {
        placeVoxel();
        return;
    }
    
    // Handle voxel removal
    if (event->key() == Qt::Key_Q && m_highlightedVoxelFace >= 0 && m_voxelSystem) {
        removeVoxel();
        return;
    }
    
    // Handle inventory toggle
    if (event->key() == Qt::Key_I && m_inventoryUI) {
        m_inventoryUI->setVisible(!m_inventoryUI->isVisible());
        // Mouse handling is managed in updateMouseTrackingState
        updateMouseTrackingState();
        return;
    }
    
    // Handle debug console toggle
    if (event->key() == Qt::Key_Escape) {
        if (m_debugSystem && isDebugConsoleVisible()) {
            toggleDebugConsole();
            return;
        }
        
        // If inventory is open, close it
        if (m_inventoryUI && m_inventoryUI->isVisible()) {
            m_inventoryUI->setVisible(false);
            updateMouseTrackingState();
            return;
        }
    }
    
    // Pass to player controller
    if (m_playerController) {
        m_playerController->handleKeyPress(event);
    }
}

void GLArenaWidget::keyReleaseEvent(QKeyEvent* event) {
    // Pass to player controller
    if (m_playerController) {
        m_playerController->handleKeyRelease(event);
    }
}

void GLArenaWidget::mouseMoveEvent(QMouseEvent* event) {
    // Skip if not initialized
    if (!m_initialized) {
        return;
    }
    
    // Handle inventory first
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMouseMove(event->x(), event->y());
        return;
    }
    
    // Handle debug console
    if (m_debugSystem && isDebugConsoleVisible()) {
        // Debug console doesn't handle mouse moves, but we should skip player handling
        return;
    }
    
    // Pass to player controller for look control
    if (m_playerController) {
        m_playerController->handleMouseMove(event);
    }
    
    // Reset cursor to center after handling movement
    QCursor::setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));
}

void GLArenaWidget::mousePressEvent(QMouseEvent* event) {
    // Handle inventory first
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMousePress(event->x(), event->y(), event->button());
        return;
    }
    
    // Voxel interaction check
    if (event->button() == Qt::LeftButton && m_highlightedVoxelFace >= 0 && m_voxelSystem) {
        placeVoxel();
        return;
    }
    
    if (event->button() == Qt::RightButton && m_highlightedVoxelFace >= 0 && m_voxelSystem) {
        removeVoxel();
        return;
    }
}

void GLArenaWidget::mouseReleaseEvent(QMouseEvent* event) {
    // Handle inventory first
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMouseRelease(event->x(), event->y(), event->button());
        return;
    }
}

void GLArenaWidget::setActiveCharacter(const QString& name) {
    m_activeCharacter = name;
}

PlayerController* GLArenaWidget::getPlayerController() const {
    return m_playerController;
}

void GLArenaWidget::updateMouseTrackingState() {
    bool hideCursor = m_initialized && 
                      (!m_inventoryUI || !m_inventoryUI->isVisible()) &&
                      (!m_debugSystem || !isDebugConsoleVisible());
    
    if (hideCursor) {
        setCursor(Qt::BlankCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
    
    // Update focus state
    if (hideCursor) {
        setFocus();
    }
}

void GLArenaWidget::renderCharacters() {
    // Skip if no characters or invalid program
    if (m_characterSprites.isEmpty() || !m_billboardProgram || !m_billboardProgram->isLinked()) {
        return;
    }
    
    // Bind shader program
    m_billboardProgram->bind();
    
    // Render each character
    for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
        if (it.value()) {
            try {
                it.value()->render(m_billboardProgram, m_viewMatrix, m_projectionMatrix);
            } catch (const std::exception& e) {
                qWarning() << "Exception rendering character" << it.key() << ":" << e.what();
            } catch (...) {
                qWarning() << "Unknown exception rendering character" << it.key();
            }
        }
    }
    
    // Unbind shader
    m_billboardProgram->release();
}

void GLArenaWidget::renderCharactersSimple() {
    // Skip if no characters or invalid program
    if (m_characterSprites.isEmpty() || !m_billboardProgram || !m_billboardProgram->isLinked()) {
        return;
    }
    
    // Bind shader program
    m_billboardProgram->bind();
    
    // For each character, get a QOpenGLTexture
    for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
        if (it.value() && it.value()->hasValidTexture()) {
            try {
                // Get position
                QVector3D pos = it.value()->getPosition();
                
                // Get dimensions
                float width = it.value()->width();
                float height = it.value()->height();
                
                // Draw character using simple quad
                drawCharacterQuad(it.value()->getTexture(), pos.x(), pos.y(), pos.z(), width, height);
            } catch (const std::exception& e) {
                qWarning() << "Exception in simplified character rendering:" << e.what();
            } catch (...) {
                qWarning() << "Unknown exception in simplified character rendering";
            }
        }
    }
    
    // Unbind shader
    m_billboardProgram->release();
}

void GLArenaWidget::renderCharactersFallback() {
    // This is the most basic fallback for rendering characters
    // It uses the game scene entities rather than character sprites
    
    // Skip if no scene or invalid program
    if (!m_gameScene || !m_billboardProgram || !m_billboardProgram->isLinked()) {
        return;
    }
    
    // Get all character entities
    QVector<GameEntity> characters = m_gameScene->getEntitiesByType("character");
    
    // Skip if no characters
    if (characters.isEmpty()) {
        return;
    }
    
    // Bind shader program
    m_billboardProgram->bind();
    
    // For each character, create a temporary quad
    for (const GameEntity& entity : characters) {
        try {
            // Draw a colored cube or billboard
            QMatrix4x4 modelMatrix;
            modelMatrix.setToIdentity();
            modelMatrix.translate(entity.position);
            
            // Set uniforms
            m_billboardProgram->setUniformValue("view", m_viewMatrix);
            m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
            m_billboardProgram->setUniformValue("model", modelMatrix);
            m_billboardProgram->setUniformValue("color", QVector4D(0.0f, 1.0f, 1.0f, 1.0f)); // Cyan
            
            // Define a simplified cube
            const float halfWidth = entity.dimensions.x() / 2.0f;
            const float halfHeight = entity.dimensions.y() / 2.0f;
            const float halfDepth = entity.dimensions.z() / 2.0f;
            
            const float vertices[] = {
                // Front face
                -halfWidth, -halfHeight, halfDepth,
                halfWidth, -halfHeight, halfDepth,
                halfWidth, halfHeight, halfDepth,
                -halfWidth, halfHeight, halfDepth
            };
            
            // Create temporary buffer
            QOpenGLBuffer tempBuffer(QOpenGLBuffer::VertexBuffer);
            tempBuffer.create();
            tempBuffer.bind();
            tempBuffer.allocate(vertices, sizeof(vertices));
            
            // Set attributes
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
            
            // Draw
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            
            // Clean up
            tempBuffer.release();
            tempBuffer.destroy();
            
        } catch (const std::exception& e) {
            qWarning() << "Exception in fallback character rendering:" << e.what();
        } catch (...) {
            qWarning() << "Unknown exception in fallback character rendering";
        }
    }
    
    // Unbind shader
    m_billboardProgram->release();
}

void GLArenaWidget::raycastVoxels(const QVector3D& origin, const QVector3D& direction) {
    if (!m_voxelSystem) {
        m_highlightedVoxelFace = -1;
        return;
    }
    
    try {
        // Raycast to find voxel hit
        QVector3D hitPos, hitNormal;
        Voxel hitVoxel;
        
        bool hit = m_voxelSystem->raycast(origin, direction, m_maxPlacementDistance, 
                                         hitPos, hitNormal, hitVoxel);
        
        if (hit && hitVoxel.type != VoxelType::Air) {
            // Set highlight position
            m_highlightedVoxelPos = hitPos;
            
            // Determine which face was hit based on normal
            if (hitNormal.x() > 0.9f) m_highlightedVoxelFace = 0;      // +X
            else if (hitNormal.x() < -0.9f) m_highlightedVoxelFace = 1; // -X
            else if (hitNormal.y() > 0.9f) m_highlightedVoxelFace = 2;  // +Y
            else if (hitNormal.y() < -0.9f) m_highlightedVoxelFace = 3; // -Y
            else if (hitNormal.z() > 0.9f) m_highlightedVoxelFace = 4;  // +Z
            else if (hitNormal.z() < -0.9f) m_highlightedVoxelFace = 5; // -Z
            else m_highlightedVoxelFace = -1;
            
            // Update highlighted voxel in inventory UI
            if (m_inventoryUI) {
                m_inventoryUI->setHighlightedVoxelFace(m_highlightedVoxelPos, m_highlightedVoxelFace);
            }
        } else {
            // No hit, clear highlight
            m_highlightedVoxelFace = -1;
            
            if (m_inventoryUI) {
                m_inventoryUI->setHighlightedVoxelFace(QVector3D(), -1);
            }
        }
    } catch (const std::exception& e) {
        qWarning() << "Exception in voxel raycasting:" << e.what();
        m_highlightedVoxelFace = -1;
    } catch (...) {
        qWarning() << "Unknown exception in voxel raycasting";
        m_highlightedVoxelFace = -1;
    }
}

// Debug system methods
void GLArenaWidget::initializeDebugSystem() {
    // Create debug system
    m_debugSystem = std::make_unique<DebugSystem>(m_gameScene, m_playerController, this);
    
    // Initialize debug system
    m_debugSystem->initialize();
}

void GLArenaWidget::renderDebugSystem() {
    if (!m_debugSystem) {
        return;
    }
    
    try {
        // Render debug system
        // We'll handle the console widget property separately
        QVariant widgetProp = QVariant::fromValue(quintptr(this));
        
        // Set widget property through DebugSystem instead of directly
        QMetaObject::invokeMethod(m_debugSystem.get(), "setConsoleWidget", 
                                 Qt::DirectConnection, Q_ARG(QVariant, widgetProp));
        
        // Render debug system
        m_debugSystem->render(m_viewMatrix, m_projectionMatrix, width(), height());
    } catch (const std::exception& e) {
        qWarning() << "Exception in debug system rendering:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception in debug system rendering";
    }
}

bool GLArenaWidget::processDebugKeyEvent(QKeyEvent* event) {
    if (!m_debugSystem) {
        return false;
    }
    
    try {
        // Pass key event to debug system
        return m_debugSystem->handleKeyPress(event->key(), event->text());
    } catch (const std::exception& e) {
        qWarning() << "Exception in debug key handling:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception in debug key handling";
    }
    
    return false;
}

void GLArenaWidget::toggleDebugConsole() {
    if (!m_debugSystem) {
        return;
    }
    
    try {
        // Toggle console visibility through the DebugSystem
        QMetaObject::invokeMethod(m_debugSystem.get(), "toggleConsoleVisibility", 
                                 Qt::DirectConnection);
        
        // Update mouse tracking state
        updateMouseTrackingState();
    } catch (const std::exception& e) {
        qWarning() << "Exception toggling debug console:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception toggling debug console";
    }
}

bool GLArenaWidget::isDebugConsoleVisible() const {
    if (!m_debugSystem) {
        return false;
    }
    
    try {
        // Get console visibility through the DebugSystem
        bool visible = false;
        QMetaObject::invokeMethod(m_debugSystem.get(), "isConsoleVisible", 
                                 Qt::DirectConnection,
                                 Q_RETURN_ARG(bool, visible));
        return visible;
    } catch (...) {
        return false;
    }
}

void GLArenaWidget::toggleFrustumVisualization() {
    if (!m_debugSystem) {
        return;
    }
    
    try {
        // Toggle frustum visualizer through the DebugSystem
        QMetaObject::invokeMethod(m_debugSystem.get(), "toggleFrustumVisualization", 
                                 Qt::DirectConnection);
    } catch (const std::exception& e) {
        qWarning() << "Exception toggling frustum visualization:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception toggling frustum visualization";
    }
}

// Inventory methods
void GLArenaWidget::initializeInventory() {
    // Create inventory
    m_inventory = new Inventory(this);
    
    // Create inventory UI
    m_inventoryUI = new InventoryUI(m_inventory, this);
    
    // Initialize inventory UI
    m_inventoryUI->initialize();
    
    // Connect signals
    connect(m_inventoryUI, &InventoryUI::visibilityChanged,
            this, &GLArenaWidget::onInventoryVisibilityChanged);
}

void GLArenaWidget::renderInventory() {
    if (!m_inventoryUI) {
        return;
    }
    
    try {
        // Render inventory UI
        m_inventoryUI->render(width(), height());
    } catch (const std::exception& e) {
        qWarning() << "Exception rendering inventory UI:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception rendering inventory UI";
    }
}

void GLArenaWidget::onInventoryVisibilityChanged(bool visible) {
    updateMouseTrackingState();
}