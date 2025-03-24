// src/arena/ui/gl_widgets/gl_arena_widget_core.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include "../../../../include/arena/debug/debug_system.h"
#include <QDateTime>
#include <QDebug>
#include <QMessageBox>
#include <QUrl>
#include <QDir>

// Core functionality implementation for GLArenaWidget

GLArenaWidget::GLArenaWidget(CharacterManager* charManager, QWidget* parent)
    : QOpenGLWidget(parent),
      m_characterManager(charManager),
      m_gameScene(nullptr),
      m_playerController(nullptr),
      m_activeCharacter(""),
      m_voxelSystem(nullptr),
      m_inventory(nullptr),
      m_inventoryUI(nullptr),
      m_billboardProgram(nullptr),
      m_initialized(false),
      m_arenaRadius(10.0),
      m_wallHeight(3.0),
      m_highlightedVoxelFace(-1),
      m_maxPlacementDistance(5.0f)
{
    // Create format with multisampling anti-aliasing
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    setFormat(format);
    
    // Setup game scene
    m_gameScene = new GameScene(this);
    
    // Setup player controller
    m_playerController = new PlayerController(m_gameScene, this);
    
    // Connect player controller signals
    connect(m_playerController, &PlayerController::positionChanged, 
            this, &GLArenaWidget::onPlayerPositionChanged);
    
    connect(m_playerController, &PlayerController::rotationChanged,
            this, &GLArenaWidget::onPlayerRotationChanged);
    
    connect(m_playerController, &PlayerController::pitchChanged,
            this, &GLArenaWidget::onPlayerPitchChanged);
    
    // Set focus policy to receive keyboard input
    setFocusPolicy(Qt::StrongFocus);
    
    // Enable mouse tracking
    setMouseTracking(true);
}

GLArenaWidget::~GLArenaWidget()
{
    // Destroy OpenGL resources first
    makeCurrent();
    
    // Clean up sprites
    for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
        delete it.value();
    }
    m_characterSprites.clear();
    
    // Delete shader program
    delete m_billboardProgram;
    
    // Clean up other OpenGL resources
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
    
    doneCurrent();
    
    // Delete voxel system
    delete m_voxelSystem;
    
    // Delete inventory
    delete m_inventory;
    delete m_inventoryUI;
}

void GLArenaWidget::updateMouseTrackingState()
{
    // Only enable mouse tracking if the widget is focused and inventory is closed
    if (hasFocus() && m_inventoryUI && !m_inventoryUI->isVisible()) {
        // Hide cursor
        setCursor(Qt::BlankCursor);
        
        // Center mouse after capturing input
        QCursor::setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));
    } else {
        // Show cursor
        setCursor(Qt::ArrowCursor);
    }
}

void GLArenaWidget::initializeArena(double radius, double height)
{
    m_arenaRadius = radius;
    m_wallHeight = height;
    
    // Create arena if initialized
    if (m_initialized) {
        createArena(radius, height);
    }
}

void GLArenaWidget::setActiveCharacter(const QString& name)
{
    m_activeCharacter = name;
}

void GLArenaWidget::loadCharacterSprite(const QString& characterName, const QString& texturePath)
{
    // Make sure we're current
    makeCurrent();
    
    // Check if sprite already exists
    if (m_characterSprites.contains(characterName)) {
        delete m_characterSprites[characterName];
    }
    
    // Create new sprite
    CharacterSprite* sprite = new CharacterSprite();
    sprite->init(context(), texturePath, 1.0, 2.0, 1.0);
    m_characterSprites[characterName] = sprite;
    
    // If this is not the active character, create a game entity for it
    if (characterName != m_activeCharacter) {
        GameEntity characterEntity;
        characterEntity.id = characterName;
        characterEntity.type = "character";
        characterEntity.position = QVector3D(0, 1.0, 0); // Default position
        characterEntity.dimensions = QVector3D(1.0, 2.0, 1.0); // Human dimensions
        characterEntity.isStatic = false;
        characterEntity.spritePath = texturePath;
        
        // Add to game scene
        m_gameScene->addEntity(characterEntity);
    }
    
    doneCurrent();
}

void GLArenaWidget::updateCharacterPosition(const QString& characterName, float x, float y, float z)
{
    if (m_gameScene) {
        m_gameScene->updateEntityPosition(characterName, QVector3D(x, y, z));
    }
}

PlayerController* GLArenaWidget::getPlayerController() const
{
    return m_playerController;
}

void GLArenaWidget::initializeGL()
{
    // Initialize OpenGL functions
    initializeOpenGLFunctions();
    
    // Set clear color
    glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Enable back face culling
    glEnable(GL_CULL_FACE);
    
    // Create shaders
    if (!initShaders()) {
        QMessageBox::critical(this, "Error", "Failed to initialize shaders");
        return;
    }
    
    // Create floor, walls and grid
    createArena(m_arenaRadius, m_wallHeight);
    
    // Create voxel system
    m_voxelSystem = new VoxelSystemIntegration(m_gameScene, this);
    
    // Initialize inventory
    initializeInventory();
    
    // Initialize debug system 
    initializeDebugSystem();
    
    // Set initialization flag
    m_initialized = true;
    
    // Emit signal that rendering is initialized
    emit renderingInitialized();
    
    // Now set up player controller - moved here from constructor
    if (m_playerController) {
        m_playerController->createPlayerEntity();
        m_playerController->startUpdates();
    }
    
    // Safe to initialize the voxel system now that everything is set up
    if (m_voxelSystem) {
        m_voxelSystem->initialize();
        m_voxelSystem->createDefaultWorld();
    }
}

void GLArenaWidget::resizeGL(int w, int h)
{
    // Update projection matrix
    float aspectRatio = float(w) / float(h);
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(70.0f, aspectRatio, 0.1f, 100.0f);
    
    // Update player screen dimensions
    if (m_playerController) {
        m_playerController->setScreenDimensions(w, h);
    }
    
    // Update inventory UI
    if (m_inventoryUI) {
        m_inventoryUI->render(w, h);
    }
}

void GLArenaWidget::paintGL()
{
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Create view matrix from player controller
    if (m_playerController) {
        // Get player position and rotation
        QVector3D position = m_playerController->getPosition();
        float rotation = m_playerController->getRotation();
        float pitch = m_playerController->getPitch();
        
        // Create view matrix
        m_viewMatrix.setToIdentity();
        
        // Rotate for pitch (looking up/down)
        m_viewMatrix.rotate(pitch * 180.0f / M_PI, 1, 0, 0);
        
        // Rotate for yaw (looking left/right)
        m_viewMatrix.rotate(rotation * 180.0f / M_PI, 0, 1, 0);
        
        // Translate to player position (negated for camera space)
        m_viewMatrix.translate(-position);
    }
    
    // Call voxel system render first (includes sky)
    if (m_voxelSystem) {
        m_voxelSystem->render(m_viewMatrix, m_projectionMatrix);
    }
    
    // Render floor and walls with simple shader
    renderFloor();
    renderWalls();
    renderGrid();
    
    // Render dynamic objects (characters)
    renderCharacters();
    
    // Render inventory UI if visible
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->render(width(), height());
    }
    
    // Render voxel highlight for placement
    renderVoxelHighlight();
    
    // Render debug overlays
    renderDebugSystem();
}

void GLArenaWidget::onPlayerPositionChanged(const QVector3D& position)
{
    // Forward position to game scene
    if (m_gameScene) {
        m_gameScene->updateEntityPosition("player", position);
    }
    
    // Raycast for voxel highlighting/placement
    QVector3D forward = QVector3D(
        cos(m_playerController->getPitch()) * cos(m_playerController->getRotation()),
        sin(m_playerController->getPitch()),
        cos(m_playerController->getPitch()) * sin(m_playerController->getRotation())
    );
    
    raycastVoxels(position, forward);
    
    // Pass player position to voxel system for streaming
    if (m_voxelSystem) {
        m_voxelSystem->streamChunksAroundPlayer(position);
    }
    
    // Signal position update for other components
    emit playerPositionUpdated(position.x(), position.y(), position.z());
    
    // Request repaint
    update();
}

void GLArenaWidget::onPlayerRotationChanged(float rotation)
{
    // Forward rotation for voxel raycast
    QVector3D position = m_playerController->getPosition();
    float pitch = m_playerController->getPitch();
    
    QVector3D forward = QVector3D(
        cos(pitch) * cos(rotation),
        sin(pitch),
        cos(pitch) * sin(rotation)
    );
    
    raycastVoxels(position, forward);
    
    // Request repaint
    update();
}

void GLArenaWidget::onPlayerPitchChanged(float pitch)
{
    // Forward rotation for voxel raycast
    QVector3D position = m_playerController->getPosition();
    float rotation = m_playerController->getRotation();
    
    QVector3D forward = QVector3D(
        cos(pitch) * cos(rotation),
        sin(pitch),
        cos(pitch) * sin(rotation)
    );
    
    raycastVoxels(position, forward);
    
    // Request repaint
    update();
}

void GLArenaWidget::raycastVoxels(const QVector3D& origin, const QVector3D& direction)
{
    // Do the raycast
    if (m_voxelSystem && m_inventory && m_inventoryUI) {
        // Only raycast if inventory is closed
        if (!m_inventoryUI->isVisible()) {
            QVector3D hitPos, hitNormal;
            Voxel hitVoxel;
            
            if (m_voxelSystem->raycast(origin, direction, m_maxPlacementDistance, 
                                     hitPos, hitNormal, hitVoxel)) {
                // We hit something, set highlight
                VoxelPos hitVoxelPos = VoxelPos::fromVector3D(hitPos);
                
                // Determine which face was hit
                int face = -1;
                if (hitNormal == QVector3D(1, 0, 0)) face = 0;
                else if (hitNormal == QVector3D(-1, 0, 0)) face = 1;
                else if (hitNormal == QVector3D(0, 1, 0)) face = 2;
                else if (hitNormal == QVector3D(0, -1, 0)) face = 3;
                else if (hitNormal == QVector3D(0, 0, 1)) face = 4;
                else if (hitNormal == QVector3D(0, 0, -1)) face = 5;
                
                // Update highlight
                m_voxelSystem->setVoxelHighlight(hitVoxelPos, face);
                m_highlightedVoxelPos = hitVoxelPos.toVector3D();
                m_highlightedVoxelFace = face;
                
                // Pass to inventory UI
                m_inventoryUI->setHighlightedVoxelFace(hitPos, face);
            } else {
                // No hit, clear highlight
                m_voxelSystem->setVoxelHighlight(VoxelPos(0, 0, 0), -1);
                m_highlightedVoxelFace = -1;
                
                // Pass to inventory UI
                m_inventoryUI->setHighlightedVoxelFace(QVector3D(0, 0, 0), -1);
            }
        }
    }
}

void GLArenaWidget::renderVoxelHighlight()
{
    // Voxel highlighting is handled by the voxel system
}

void GLArenaWidget::placeVoxel()
{
    // Place voxel at highlighted position
    if (m_voxelSystem && m_inventory && m_inventoryUI && m_highlightedVoxelFace != -1) {
        // Get selected voxel type
        VoxelType type = m_inventoryUI->getSelectedVoxelType();
        
        // Skip if no voxel type selected
        if (type == VoxelType::Air) {
            return;
        }
        
        // Get position and face
        QVector3D position = m_highlightedVoxelPos;
        QVector3D normal;
        
        // Set normal based on face
        switch (m_highlightedVoxelFace) {
            case 0: normal = QVector3D(1, 0, 0); break;
            case 1: normal = QVector3D(-1, 0, 0); break;
            case 2: normal = QVector3D(0, 1, 0); break;
            case 3: normal = QVector3D(0, -1, 0); break;
            case 4: normal = QVector3D(0, 0, 1); break;
            case 5: normal = QVector3D(0, 0, -1); break;
            default: return; // Invalid face
        }
        
        // Create voxel based on type
        Voxel voxel;
        switch (type) {
            case VoxelType::Dirt:
                voxel = Voxel(VoxelType::Dirt, QColor(139, 69, 19));
                break;
            case VoxelType::Grass:
                voxel = Voxel(VoxelType::Grass, QColor(34, 139, 34));
                break;
            case VoxelType::Cobblestone:
                voxel = Voxel(VoxelType::Cobblestone, QColor(128, 128, 128));
                break;
            default:
                return; // Unsupported type
        }
        
        // Place the voxel
        m_voxelSystem->placeVoxel(position, normal, voxel);
    }
}

void GLArenaWidget::removeVoxel()
{
    // Remove voxel at highlighted position
    if (m_voxelSystem && m_highlightedVoxelFace != -1) {
        // Get position
        QVector3D position = m_highlightedVoxelPos;
        
        // Remove the voxel
        m_voxelSystem->removeVoxel(position);
    }
}

void GLArenaWidget::initializeInventory()
{
    // Create inventory
    m_inventory = new Inventory(this);
    
    // Create inventory UI
    m_inventoryUI = new InventoryUI(m_inventory, this);
    
    // Initialize UI
    m_inventoryUI->initialize();
    
    // Connect signals
    connect(m_inventoryUI, &InventoryUI::visibilityChanged,
            this, &GLArenaWidget::onInventoryVisibilityChanged);
}

void GLArenaWidget::renderInventory()
{
    if (m_inventoryUI) {
        m_inventoryUI->render(width(), height());
    }
}

void GLArenaWidget::onInventoryVisibilityChanged(bool visible)
{
    // Update mouse behavior
    updateMouseTrackingState();
}

void GLArenaWidget::keyPressEvent(QKeyEvent* event)
{
    // Check if debug system should handle this event
    if (processDebugKeyEvent(event)) {
        return;
    }
    
    // Handle inventory toggle
    if (event->key() == Qt::Key_I && m_inventoryUI) {
        m_inventoryUI->setVisible(!m_inventoryUI->isVisible());
        
        // Update mouse tracking
        updateMouseTrackingState();
        return;
    }
    
    // Handle voxel placement
    if (event->key() == Qt::Key_F && m_highlightedVoxelFace != -1) {
        placeVoxel();
        return;
    }
    
    // Handle voxel removal
    if (event->key() == Qt::Key_G && m_highlightedVoxelFace != -1) {
        removeVoxel();
        return;
    }
    
    // Handle inventory UI key events if inventory is open
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleKeyPress(event->key());
        return;
    }
    
    // Forward to player controller if not handled by UI
    if (m_playerController) {
        m_playerController->handleKeyPress(event);
    }
}

void GLArenaWidget::keyReleaseEvent(QKeyEvent* event)
{
    // Don't forward to player controller if inventory is open
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        return;
    }
    
    // Forward to player controller
    if (m_playerController) {
        m_playerController->handleKeyRelease(event);
    }
}

void GLArenaWidget::mouseMoveEvent(QMouseEvent* event)
{
    // Check if mouse is over inventory UI
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMouseMove(event->x(), event->y());
        return;
    }
    
    // Forward to player controller
    if (m_playerController && hasFocus()) {
        m_playerController->handleMouseMove(event);
        
        // Reset mouse to center
        QCursor::setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));
    }
}

void GLArenaWidget::mousePressEvent(QMouseEvent* event)
{
    // Set focus on click
    setFocus();
    
    // Update mouse tracking
    updateMouseTrackingState();
    
    // Check if mouse is over inventory UI
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMousePress(event->x(), event->y(), event->button());
        return;
    }
    
    // Forward to player controller
    if (m_playerController) {
        // Handle mouse click (e.g., for interaction)
    }
}

void GLArenaWidget::mouseReleaseEvent(QMouseEvent* event)
{
    // Check if mouse is over inventory UI
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMouseRelease(event->x(), event->y(), event->button());
        return;
    }
    
    // Forward to player controller
    if (m_playerController) {
        // Handle mouse release
    }
}

bool GLArenaWidget::initShaders()
{
    // Create shader program for billboards
    m_billboardProgram = new QOpenGLShaderProgram(this);
    
    // Load and compile vertex shader
    if (!m_billboardProgram->addShaderFromSourceCode(QOpenGLShader::Vertex,
        // Vertex shader
        "attribute vec3 position;\n"
        "attribute vec2 texCoord;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "uniform vec3 cameraRight;\n"
        "uniform vec3 cameraUp;\n"
        "uniform vec3 billboardPos;\n"
        "uniform vec2 billboardSize;\n"
        "varying vec2 fragTexCoord;\n"
        "varying vec3 fragPos;\n"
        "void main() {\n"
        "    // For standard objects, use model-view-projection\n"
        "    vec4 modelPos = model * vec4(position, 1.0);\n"
        "    gl_Position = projection * view * modelPos;\n"
        "    fragTexCoord = texCoord;\n"
        "    fragPos = modelPos.xyz;\n"
        "}\n"
    )) {
        qCritical() << "Failed to compile vertex shader:" << m_billboardProgram->log();
        return false;
    }
    
    // Load and compile fragment shader
    if (!m_billboardProgram->addShaderFromSourceCode(QOpenGLShader::Fragment,
        // Fragment shader
        "uniform sampler2D textureSampler;\n"
        "uniform vec4 color;\n"
        "varying vec2 fragTexCoord;\n"
        "varying vec3 fragPos;\n"
        "void main() {\n"
        "    // Sample texture or use solid color\n"
        "    vec4 texColor = texture2D(textureSampler, fragTexCoord);\n"
        "    if (texColor.a < 0.1) discard;\n"
        "    gl_FragColor = texColor * color;\n"
        "}\n"
    )) {
        qCritical() << "Failed to compile fragment shader:" << m_billboardProgram->log();
        return false;
    }
    
    // Link shader program
    if (!m_billboardProgram->link()) {
        qCritical() << "Failed to link shader program:" << m_billboardProgram->log();
        return false;
    }
    
    qDebug() << "Shaders initialized successfully";
    return true;
}