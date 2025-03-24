// src/arena/ui/gl_widgets/gl_arena_widget_core.cpp
#include "../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include "../../../include/arena/debug/debug_system.h" // Add include for DebugSystem definition
#include <QDebug>
#include <QCursor>
#include <QApplication>

GLArenaWidget::GLArenaWidget(CharacterManager* charManager, QWidget* parent)
    : QOpenGLWidget(parent),
    m_characterManager(charManager),
    m_billboardProgram(nullptr),
    m_initialized(false),
    m_gameScene(nullptr),
    m_playerController(nullptr),
    m_voxelSystem(nullptr),
    m_inventory(nullptr),
    m_inventoryUI(nullptr),
    m_arenaRadius(10.0),
    m_wallHeight(3.0)
{
    // Set focus policy to accept keyboard input
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    
    // Create game scene for entity tracking
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
}

GLArenaWidget::~GLArenaWidget()
{
    // The context must be made current for cleanup to work correctly
    makeCurrent();
    
    // Clean up character sprites first
    for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
        delete it.value();
    }
    m_characterSprites.clear();
    
    // Clean up inventory UI
    delete m_inventoryUI;
    
    // Clean up inventory system
    delete m_inventory;
    
    // Clean up voxel system
    delete m_voxelSystem;
    
    // Clean up OpenGL buffers
    m_floorVBO.destroy();
    m_floorIBO.destroy();
    m_floorVAO.destroy();
    
    m_gridVBO.destroy();
    m_gridVAO.destroy();
    
    for (auto& wall : m_walls) {
        if (wall.vbo) wall.vbo->destroy();
        if (wall.ibo) wall.ibo->destroy();
        if (wall.vao) wall.vao->destroy();
    }
    m_walls.clear();
    
    // Clean up shaders
    delete m_billboardProgram;
    
    // Clean up game entities
    delete m_playerController;
    delete m_gameScene;
    
    doneCurrent();
}

void GLArenaWidget::initializeGL()
{
    // Initialize OpenGL functions
    initializeOpenGLFunctions();
    
    // Set up OpenGL state
    glClearColor(0.1f, 0.1f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Initialize shaders
    if (!initShaders()) {
        qCritical() << "Failed to initialize shaders";
        return;
    }
    
    // Set up view and projection matrices
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(60.0f, float(width()) / height(), 0.1f, 100.0f);
    
    m_viewMatrix.setToIdentity();
    m_viewMatrix.lookAt(QVector3D(0, 2, 10), QVector3D(0, 0, 0), QVector3D(0, 1, 0));
    
    // Create initial geometry
    createFloor(20.0); // Large floor
    createGrid(20.0, 20); // 20x20 grid
    createArena(m_arenaRadius, m_wallHeight);
    
    // Create player entity
    m_playerController->createPlayerEntity();
    m_playerController->startUpdates();
    
    // Initialize voxel system
    m_voxelSystem = new VoxelSystemIntegration(m_gameScene, this);
    m_voxelSystem->initialize();
    m_voxelSystem->createDefaultWorld();
    
    // Initialize inventory system
    initializeInventory();
    
    // Initialize debug system
    initializeDebugSystem();
    
    // Mark initialization as complete
    m_initialized = true;
    
    // Start the player controller updates
    m_playerController->startUpdates();
    
    // Center cursor and setup mouse tracking
    updateMouseTrackingState();
    
    // Emit signal that rendering is initialized
    emit renderingInitialized();
}

void GLArenaWidget::resizeGL(int w, int h)
{
    // Update projection matrix for new aspect ratio
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(60.0f, float(w) / h, 0.1f, 100.0f);
    
    // Update viewport
    glViewport(0, 0, w, h);
    
    // Update screen dimensions for player controller
    // We'll use updateMouseTrackingState() since it handles the screen dimensions implicitly
    updateMouseTrackingState();
}

void GLArenaWidget::paintGL()
{
    // Clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Skip if not fully initialized
    if (!m_initialized || !m_playerController) {
        return;
    }
    
    // Update view matrix from player controller
    m_viewMatrix.setToIdentity();
    
    QVector3D position = m_playerController->getPosition();
    float yaw = m_playerController->getRotation();
    float pitch = m_playerController->getPitch();
    
    // Calculate view matrix from player position and rotation
    QVector3D forwardVector(cos(yaw) * cos(pitch), sin(pitch), sin(yaw) * cos(pitch));
    QVector3D upVector(0.0f, 1.0f, 0.0f);
    
    m_viewMatrix.lookAt(position, position + forwardVector, upVector);
    
    // Render floor and grid
    renderFloor();
    renderGrid();
    
    // Render arena walls
    renderWalls();
    
    // Render voxels if system is initialized
    if (m_voxelSystem) {
        m_voxelSystem->render(m_viewMatrix, m_projectionMatrix);
    }
    
    // Raycast for voxel placement/removal
    if (m_voxelSystem && !m_inventoryUI->isVisible()) {
        QVector3D forwardDirection(cos(yaw) * cos(pitch), sin(pitch), sin(yaw) * cos(pitch));
        raycastVoxels(position, forwardDirection);
        renderVoxelHighlight();
    }
    
    // Render characters
    if (m_billboardProgram && m_billboardProgram->isLinked()) {
        renderCharacters();
    } else {
        // Try simpler methods if shader program isn't ready
        renderCharactersSimple();
    }
    
    // Render inventory UI
    if (m_inventoryUI) {
        m_inventoryUI->render(width(), height());
    }
    
    // Render debug system last (on top of everything else)
    if (m_debugSystem) {
        renderDebugSystem();
    }
}

void GLArenaWidget::updateMouseTrackingState()
{
    if (hasFocus() && isActiveWindow() && !m_inventoryUI->isVisible()) {
        // Hide cursor and enable mouse tracking
        setCursor(Qt::BlankCursor);
        
        // Center cursor position
        QCursor::setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));
    } else {
        // Show cursor
        setCursor(Qt::ArrowCursor);
    }
}

PlayerController* GLArenaWidget::getPlayerController() const
{
    return m_playerController;
}

void GLArenaWidget::setActiveCharacter(const QString& name) 
{
    m_activeCharacter = name;
    qDebug() << "Set active character to:" << name;
}

void GLArenaWidget::onPlayerPositionChanged(const QVector3D& position)
{
    // Update player position in game scene
    if (m_gameScene) {
        m_gameScene->updateEntityPosition("player", position);
    }
    
    // Emit signal for player position change
    emit playerPositionUpdated(position.x(), position.y(), position.z());
    
    // Update voxel streaming around player
    if (m_voxelSystem) {
        m_voxelSystem->streamChunksAroundPlayer(position);
    }
    
    // Request redraw
    update();
}

void GLArenaWidget::onPlayerRotationChanged(float rotation)
{
    // Update view based on new rotation
    update();
}

void GLArenaWidget::onPlayerPitchChanged(float pitch)
{
    // Update view based on new pitch
    update();
}