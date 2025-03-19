// src/rendering/gl_arena/gl_arena_widget_core.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include <QDebug>
#include <QDir>
#include <QCursor>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QtMath>
#include <QMessageBox>
#include <stdexcept>

GLArenaWidget::GLArenaWidget(CharacterManager* charManager, QWidget* parent)
    : QOpenGLWidget(parent),
      m_characterManager(charManager),
      m_gameScene(nullptr),
      m_playerController(nullptr),
      m_activeCharacter(""),
      m_voxelSystem(nullptr),
      m_billboardProgram(nullptr),
      m_initialized(false),
      m_arenaRadius(0.0),
      m_wallHeight(0.0),
      m_maxPlacementDistance(10.0f) // Max distance for placement of voxels
{
    // Set focus policy to receive keyboard and mouse events
    setFocusPolicy(Qt::StrongFocus);
    
    // Set mouse tracking to receive mouse move events even when no buttons are pressed
    setMouseTracking(true);
    
    try {
        // Create game scene
        m_gameScene = new GameScene(this);
        if (!m_gameScene) {
            throw std::runtime_error("Failed to create game scene");
        }
        
        // Create player controller
        m_playerController = new PlayerController(m_gameScene, this);
        if (!m_playerController) {
            throw std::runtime_error("Failed to create player controller");
        }
        
        // Connect player signals
        connect(m_playerController, &PlayerController::positionChanged,
                this, &GLArenaWidget::onPlayerPositionChanged);
        connect(m_playerController, &PlayerController::rotationChanged,
                this, &GLArenaWidget::onPlayerRotationChanged);
        connect(m_playerController, &PlayerController::pitchChanged,
                this, &GLArenaWidget::onPlayerPitchChanged);
                
        // Set OpenGL format to ensure compatibility with Qt 5.9+ and OpenGL 3.3+
        QSurfaceFormat format;
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        format.setVersion(3, 3);
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setSamples(4); // Enable multisampling for smoother rendering
        format.setSwapInterval(1); // Enable vsync
        setFormat(format);
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in GLArenaWidget constructor:" << e.what();
        
        // Ensure destructor doesn't try to access bad pointers by nulling them
        if (m_gameScene) {
            delete m_gameScene;
            m_gameScene = nullptr;
        }
        if (m_playerController) {
            delete m_playerController;
            m_playerController = nullptr;
        }
        
        // Rethrow to notify parent
        throw;
    }
}

GLArenaWidget::~GLArenaWidget() {
    // Make sure we have current OpenGL context before cleaning up resources
    makeCurrent();
    
    try {
        // Clean up character sprites
        for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
            if (it.value()) {
                delete it.value();
            }
        }
        m_characterSprites.clear();
        
        // Clean up floor and grid VAOs and VBOs
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
        
        // Clean up wall geometry
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
        
        // Clean up shader program
        if (m_billboardProgram) {
            delete m_billboardProgram;
            m_billboardProgram = nullptr;
        }
        
        // Clean up inventory UI
        if (m_inventoryUI) {
            delete m_inventoryUI;
            m_inventoryUI = nullptr;
        }
        
        // Clean up inventory
        if (m_inventory) {
            delete m_inventory;
            m_inventory = nullptr;
        }
        
        // Clean up voxel system
        if (m_voxelSystem) {
            delete m_voxelSystem;
            m_voxelSystem = nullptr;
        }
    }
    catch (const std::exception& e) {
        qWarning() << "Exception during GLArenaWidget cleanup:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception during GLArenaWidget cleanup";
    }
    
    doneCurrent();
}

void GLArenaWidget::updateMouseTrackingState() {
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        // Show cursor and enable tracking when inventory is visible
        setCursor(Qt::ArrowCursor);
        setMouseTracking(true);
    } else {
        // Hide cursor for game mode
        setCursor(Qt::BlankCursor);
        setMouseTracking(true); // Still need tracking for camera movement
    }
}

void GLArenaWidget::initializeArena(double width, double height) {
    if (!m_initialized) {
        qWarning() << "Cannot initialize arena: OpenGL not yet initialized";
        return;
    }
    
    m_arenaRadius = width / 2.0;
    m_wallHeight = height;
    
    try {
        // Create arena in game scene
        if (m_gameScene) {
            m_gameScene->createRectangularArena(m_arenaRadius, m_wallHeight);
        }
        
        // Initialize voxel system with a basic world
        if (m_voxelSystem) {
            m_voxelSystem->createDefaultWorld();
        }
        
        // Create visual floor and grid
        createFloor(m_arenaRadius);
        createGrid(m_arenaRadius * 2, 10);
        
        // Create player entity
        if (m_playerController) {
            m_playerController->createPlayerEntity();
            
            // Start player movement updates
            m_playerController->startUpdates();
        }
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in initializeArena:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown exception in initializeArena";
    }
}

void GLArenaWidget::setActiveCharacter(const QString& name) {
    // Don't do anything if the name hasn't changed
    if (m_activeCharacter == name) {
        return;
    }
    
    qDebug() << "Setting active character to:" << name;
    m_activeCharacter = name;
}

void GLArenaWidget::loadCharacterSprite(const QString& characterName, const QString& texturePath) {
    if (!m_initialized) {
        qWarning() << "Cannot load character sprite: OpenGL not yet initialized";
        return;
    }
    
    try {
        // Make sure we have current context before creating textures
        makeCurrent();
        
        // Check if we already have this character
        if (m_characterSprites.contains(characterName)) {
            // Update existing character sprite
            m_characterSprites[characterName]->init(context(), texturePath, 1.0, 2.0, 0.5);
        }
        else {
            // Create new character sprite
            CharacterSprite* sprite = new CharacterSprite();
            sprite->init(context(), texturePath, 1.0, 2.0, 0.5);
            m_characterSprites[characterName] = sprite;
            
            // Position the character in the center of the arena initially
            float x = 0.0f; // Centered horizontally
            float y = 1.0f; // Standing on the floor
            float z = -3.0f; // A bit in front of the origin
            
            updateCharacterPosition(characterName, x, y, z);
        }
        
        // Update the scene
        update();
        
        doneCurrent();
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in loadCharacterSprite:" << e.what();
        doneCurrent();
    }
    catch (...) {
        qCritical() << "Unknown exception in loadCharacterSprite";
        doneCurrent();
    }
}

void GLArenaWidget::updateCharacterPosition(const QString& characterName, float x, float y, float z) {
    if (m_characterSprites.contains(characterName)) {
        // Update the character sprite position
        m_characterSprites[characterName]->updatePosition(x, y, z);
        
        // Find or create game entity for this character
        GameEntity entity = m_gameScene->getEntity(characterName);
        if (entity.id.isEmpty()) {
            // Create new entity
            entity.id = characterName;
            entity.type = "character";
            entity.position = QVector3D(x, y, z);
            entity.dimensions = QVector3D(1.0f, 2.0f, 0.5f); // Width, height, depth
            entity.isStatic = false;
            
            m_gameScene->addEntity(entity);
        }
        else {
            // Update existing entity
            m_gameScene->updateEntityPosition(characterName, QVector3D(x, y, z));
        }
        
        // Signal the position change
        emit characterPositionUpdated(characterName, x, y, z);
        
        // Update the scene
        update();
    }
}

void GLArenaWidget::onPlayerPositionChanged(const QVector3D& position) {
    // Update player entity in game scene (handled by PlayerController)
    
    // Update the view matrix
    float eyeHeight = m_playerController->getEyeHeight();
    
    // Create view matrix from player position
    m_viewMatrix.setToIdentity();
    
    // Apply pitch (look up/down)
    m_viewMatrix.rotate(qRadiansToDegrees(m_playerController->getPitch()), QVector3D(1, 0, 0));
    
    // Apply rotation (look left/right)
    m_viewMatrix.rotate(qRadiansToDegrees(m_playerController->getRotation()), QVector3D(0, 1, 0));
    
    // Apply translation (position)
    m_viewMatrix.translate(-position.x(), -(position.y() + eyeHeight), -position.z());
    
    // Signal the position change
    emit playerPositionUpdated(position.x(), position.y(), position.z());
    
    // Perform raycasting for voxel highlighting - start at the camera position
    // and shoot a ray in the camera's forward direction
    QVector3D rayOrigin = position + QVector3D(0, eyeHeight, 0);
    
    // Forward vector based on player rotation - we need to negate z since OpenGL uses right-handed coordinate system
    float rotationAngle = m_playerController->getRotation();
    float pitchAngle = m_playerController->getPitch();
    
    // Calculate the forward vector considering both yaw and pitch
    QVector3D forward;
    forward.setX(sin(rotationAngle) * cos(pitchAngle));
    forward.setY(-sin(pitchAngle)); // Negative because y is up
    forward.setZ(-cos(rotationAngle) * cos(pitchAngle)); // Negative because of right-handed coordinate system
    
    // Raycast using this origin and direction
    if (m_voxelSystem) {
        raycastVoxels(rayOrigin, forward);
    }
    
    // Set the voxel highlight in the voxel system
    if (m_voxelSystem && m_highlightedVoxelFace >= 0) {
        m_voxelSystem->setVoxelHighlight(
            VoxelPos(m_highlightedVoxelPos.x(), m_highlightedVoxelPos.y(), m_highlightedVoxelPos.z()),
            m_highlightedVoxelFace);
    }
    
    // Update the scene
    update();
}

void GLArenaWidget::onPlayerRotationChanged(float rotation) {
    // Update the view matrix - will be handled by onPlayerPositionChanged
    update();
}

void GLArenaWidget::onPlayerPitchChanged(float pitch) {
    // Update the view matrix - will be handled by onPlayerPositionChanged
    update();
}