// src/arena/ui/gl_widgets/gl_arena_widget_core.cpp
#include "../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include <QDebug>
#include <QMouseEvent>
#include <QMatrix4x4>
#include <QtMath>
#include <QCursor>

GLArenaWidget::GLArenaWidget(CharacterManager* charManager, QWidget* parent)
    : QOpenGLWidget(parent),
      m_characterManager(charManager),
      m_gameScene(nullptr),
      m_playerController(nullptr),
      m_voxelSystem(nullptr),
      m_billboardProgram(nullptr),
      m_initialized(false),
      m_floorIndexCount(0),
      m_gridVertexCount(0),
      m_arenaRadius(10.0),
      m_wallHeight(3.0),
      m_highlightedVoxelFace(-1)
{
    // Set focus policy to accept keyboard input
    setFocusPolicy(Qt::StrongFocus);
    
    // Enable mouse tracking for continuous updates
    setMouseTracking(true);
    
    // Create game scene
    m_gameScene = new GameScene(this);
    
    // Create player controller
    m_playerController = new PlayerController(m_gameScene, this);
    
    // Connect player signals for camera updates
    connect(m_playerController, &PlayerController::positionChanged,
            this, &GLArenaWidget::onPlayerPositionChanged);
            
    connect(m_playerController, &PlayerController::rotationChanged,
            this, &GLArenaWidget::onPlayerRotationChanged);
            
    connect(m_playerController, &PlayerController::pitchChanged,
            this, &GLArenaWidget::onPlayerPitchChanged);
    
    // Set default eye height
    m_playerController->setEyeHeight(1.7f);
}

GLArenaWidget::~GLArenaWidget()
{
    // Resources will be cleaned up automatically when context is destroyed
    
    // Clean up character sprites
    for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
        delete it.value();
    }
    m_characterSprites.clear();
    
    // Clean up shader programs
    delete m_billboardProgram;
}

void GLArenaWidget::initializeGL()
{
    // Initialize OpenGL functions
    initializeOpenGLFunctions();
    
    // Set clear color (sky blue)
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Create shaders
    bool shadersOk = initShaders();
    if (!shadersOk) {
        qWarning() << "Failed to initialize shaders";
    }
    
    // Create floor grid
    createFloor(m_arenaRadius);
    createGrid(m_arenaRadius * 2, 20);
    
    // Create initial arena walls
    createArena(m_arenaRadius, m_wallHeight);
    
    // Initialize inventory system
    initializeInventory();
    
    // Initialize voxel system
    m_voxelSystem = new VoxelSystemIntegration(m_gameScene, this);
    m_voxelSystem->initialize();
    m_voxelSystem->createDefaultWorld();
    
    // Create player entity
    m_playerController->createPlayerEntity();
    m_playerController->startUpdates();
    
    // Initialize debug system
    initializeDebugSystem();
    
    // Mark initialization as complete
    m_initialized = true;
    
    // Signal that rendering is initialized
    emit renderingInitialized();
}

void GLArenaWidget::resizeGL(int w, int h)
{
    // Update projection matrix for new aspect ratio
    float aspect = static_cast<float>(w) / static_cast<float>(h ? h : 1);
    
    // Create perspective projection matrix
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(70.0f, aspect, 0.1f, 1000.0f);
    
    // Update screen dimensions for mouse handling
    m_playerController->setScreenSize(w, h);
}

void GLArenaWidget::paintGL()
{
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Check if fully initialized
    if (!m_initialized || !m_billboardProgram || !m_playerController) {
        return;
    }
    
    // Update view matrix based on player position and rotation
    m_viewMatrix.setToIdentity();
    
    // Apply player rotation (pitch and yaw)
    QVector3D playerPos = m_playerController->getPosition();
    float playerRotation = m_playerController->getRotation();
    float playerPitch = m_playerController->getPitch();
    
    // Apply player pitch rotation around X axis
    m_viewMatrix.rotate(playerPitch * 180.0f / M_PI, 1, 0, 0);
    
    // Apply player yaw rotation around Y axis
    m_viewMatrix.rotate(playerRotation * 180.0f / M_PI, 0, 1, 0);
    
    // Apply player translation (negated for camera)
    m_viewMatrix.translate(-playerPos);
    
    // Try to render voxel world if initialized
    if (m_voxelSystem) {
        try {
            m_voxelSystem->render(m_viewMatrix, m_projectionMatrix);
        } catch (...) {
            // Ignore errors and continue rendering other elements
        }
    }
    
    // Render floor, grid, and walls
    renderFloor();
    renderGrid();
    renderWalls();
    
    // Render characters
    try {
        renderCharacters();
    } catch (...) {
        // Fall back to simpler rendering method if there are errors
        try {
            renderCharactersFallback();
        } catch (...) {
            // Ignore if even fallback fails
        }
    }
    
    // Render inventory UI if initialized
    if (m_inventoryUI) {
        m_inventoryUI->render(width(), height());
    }
    
    // Render voxel highlight
    try {
        renderVoxelHighlight();
    } catch (...) {
        // Ignore errors in highlight rendering
    }
    
    // Render debug visualizations last (on top of everything else)
    renderDebugSystem();
    
    // Request another update to keep animation smooth
    update();
}

void GLArenaWidget::keyPressEvent(QKeyEvent* event)
{
    if (!event) {
        return;
    }
    
    // Try to process through debug system first
    if (processDebugKeyEvent(event)) {
        // Debug system handled the key, don't pass to other systems
        event->accept();
        return;
    }
    
    // Process inventory UI key events if inventory is visible
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleKeyPress(event->key());
        event->accept();
        return;
    }
    
    // Handle inventory toggle with 'I' key
    if (event->key() == Qt::Key_I && m_inventoryUI) {
        m_inventoryUI->setVisible(!m_inventoryUI->isVisible());
        updateMouseTrackingState();
        event->accept();
        return;
    }
    
    // Handle voxel placement with mouse buttons
    if (event->key() == Qt::Key_E) {
        placeVoxel();
        event->accept();
        return;
    }
    
    if (event->key() == Qt::Key_Q) {
        removeVoxel();
        event->accept();
        return;
    }
    
    // Check for F3 key (debug menu toggle)
    if (event->key() == Qt::Key_F3) {
        toggleDebugConsole();
        event->accept();
        return;
    }
    
    // Check for F4 key (frustum visualization toggle)
    if (event->key() == Qt::Key_F4) {
        toggleFrustumVisualization();
        event->accept();
        return;
    }
    
    // Pass event to player controller for movement
    m_playerController->handleKeyPress(event);
    
    // Special handling for Escape key
    if (event->key() == Qt::Key_Escape) {
        // If debug console is visible, hide it
        if (isDebugConsoleVisible()) {
            toggleDebugConsole();
            event->accept();
            return;
        }
        
        // If inventory is visible, hide it
        if (m_inventoryUI && m_inventoryUI->isVisible()) {
            m_inventoryUI->setVisible(false);
            updateMouseTrackingState();
            event->accept();
            return;
        }
    }
}

void GLArenaWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (!event) {
        return;
    }
    
    // Pass key release to player controller
    m_playerController->handleKeyRelease(event);
}

void GLArenaWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!event) {
        return;
    }
    
    // Check if inventory is visible
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMouseMove(event->x(), event->y());
        event->accept();
        return;
    }
    
    // Check if debug console is visible
    if (isDebugConsoleVisible()) {
        // Don't process mouse movement when console is open
        event->accept();
        return;
    }
    
    // Pass event to player controller
    m_playerController->handleMouseMove(event);
    
    // Reset cursor position to center of widget for continuous rotation
    QCursor::setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));
    
    // Raycast for voxel highlighting
    QVector3D playerPos = m_playerController->getPosition();
    
    // Calculate view direction from rotation
    float rotation = m_playerController->getRotation();
    float pitch = m_playerController->getPitch();
    
    QVector3D viewDir(
        cos(pitch) * cos(rotation),
        sin(pitch),
        cos(pitch) * sin(rotation)
    );
    
    raycastVoxels(playerPos, viewDir);
}

void GLArenaWidget::mousePressEvent(QMouseEvent* event)
{
    if (!event) {
        return;
    }
    
    // Check if inventory is visible
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMousePress(event->x(), event->y(), event->button());
        event->accept();
        return;
    }
    
    // Left click to place voxel
    if (event->button() == Qt::LeftButton) {
        placeVoxel();
    }
    // Right click to remove voxel
    else if (event->button() == Qt::RightButton) {
        removeVoxel();
    }
}

void GLArenaWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (!event) {
        return;
    }
    
    // Check if inventory is visible
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMouseRelease(event->x(), event->y(), event->button());
        event->accept();
        return;
    }
}

void GLArenaWidget::updateMouseTrackingState()
{
    // Check if inventory UI is visible
    bool inventoryVisible = m_inventoryUI && m_inventoryUI->isVisible();
    
    // Check if debug console is visible
    bool consoleVisible = isDebugConsoleVisible();
    
    if (inventoryVisible || consoleVisible) {
        // Show cursor for UI interaction
        setCursor(Qt::ArrowCursor);
        setMouseTracking(true);
    } else {
        // Hide cursor for FPS camera control
        setCursor(Qt::BlankCursor);
        setMouseTracking(true);
        
        // Reset cursor to center of screen
        QCursor::setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));
    }
}

void GLArenaWidget::initializeArena(double radius, double wallHeight)
{
    // Update arena parameters
    m_arenaRadius = radius;
    m_wallHeight = wallHeight;
    
    // Create arena in game scene
    if (m_gameScene) {
        m_gameScene->createRectangularArena(radius, wallHeight);
    }
    
    // Update arena geometry
    if (isValid() && context()->isValid()) {
        makeCurrent();
        
        // Re-create floor and grid with new radius
        createFloor(radius);
        createGrid(radius * 2, 20);
        
        // Re-create walls with new dimensions
        createArena(radius, wallHeight);
        
        doneCurrent();
    }
}

// Player controller getter
PlayerController* GLArenaWidget::getPlayerController() const
{
    return m_playerController;
}

// Player position change handler
void GLArenaWidget::onPlayerPositionChanged(const QVector3D& position)
{
    // Update player entity in game scene
    if (m_gameScene) {
        m_gameScene->updateEntityPosition("player", position);
    }
    
    // Signal position update
    emit playerPositionUpdated(position.x(), position.y(), position.z());
    
    // Update voxel chunks around player if voxel system is enabled
    if (m_voxelSystem) {
        m_voxelSystem->streamChunksAroundPlayer(position);
    }
    
    // Request redraw
    update();
}

// Player rotation change handler
void GLArenaWidget::onPlayerRotationChanged(float rotation)
{
    // Request redraw
    update();
}

// Player pitch change handler
void GLArenaWidget::onPlayerPitchChanged(float pitch)
{
    // Request redraw
    update();
}