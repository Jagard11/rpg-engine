// src/rendering/gl_arena/gl_arena_widget_core.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include "../../include/game/game_scene.h"
#include "../../include/game/player_controller.h"
#include <QOpenGLContext>
#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <stdexcept>

GLArenaWidget::GLArenaWidget(CharacterManager* charManager, QWidget* parent)
    : QOpenGLWidget(parent), m_characterManager(charManager), 
      m_arenaRadius(10.0), m_wallHeight(2.0), m_initialized(false),
      m_floorVBO(QOpenGLBuffer::VertexBuffer), m_floorIBO(QOpenGLBuffer::IndexBuffer),
      m_gridVBO(QOpenGLBuffer::VertexBuffer),
      m_basicProgram(nullptr), m_billboardProgram(nullptr), m_gridProgram(nullptr),
      m_floorIndexCount(0), m_gridVertexCount(0)
{
    // Set focus policy to receive keyboard events
    setFocusPolicy(Qt::StrongFocus);
    
    // Create game scene
    m_gameScene = new GameScene(this);
    
    // Create player controller
    m_playerController = new PlayerController(m_gameScene, this);
    
    // Connect player signals
    connect(m_playerController, &PlayerController::positionChanged, 
            this, &GLArenaWidget::onPlayerPositionChanged);
    connect(m_playerController, &PlayerController::rotationChanged, 
            this, &GLArenaWidget::onPlayerRotationChanged);
    
    // Use a safer update approach with lower frequency
    QTimer *safeTimer = new QTimer(this);
    connect(safeTimer, &QTimer::timeout, this, [this]() {
        if (isVisible() && isValid()) {
            update(); // Request repaint only if widget is visible and valid
        }
    });
    safeTimer->start(33); // ~30 FPS - balanced for performance
}

GLArenaWidget::~GLArenaWidget()
{
    // Make sure we handle OpenGL resource cleanup safely
    try {
        // Only try to make current if the context is valid
        if (context() && context()->isValid()) {
            // makeCurrent() returns void, not bool, so we can't check its return value
            makeCurrent();
            
            // Clean up OpenGL resources
            if (m_floorVBO.isCreated()) m_floorVBO.destroy();
            if (m_floorIBO.isCreated()) m_floorIBO.destroy();
            if (m_floorVAO.isCreated()) m_floorVAO.destroy();
            if (m_gridVBO.isCreated()) m_gridVBO.destroy();
            if (m_gridVAO.isCreated()) m_gridVAO.destroy();
            
            // Walls are auto-cleaned by the std::unique_ptr
            m_walls.clear();
            
            // Clean up character sprites - POTENTIAL BUG AREA
            // Safer deletion with checks
            for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
                if (it.value()) {
                    delete it.value();
                    it.value() = nullptr;
                }
            }
            m_characterSprites.clear();
            
            // Clean up shader programs
            delete m_basicProgram;
            m_basicProgram = nullptr;
            
            delete m_billboardProgram;
            m_billboardProgram = nullptr;
            
            delete m_gridProgram;
            m_gridProgram = nullptr;
            
            doneCurrent();
        } else {
            qWarning() << "Invalid OpenGL context during destructor cleanup";
        }
    } catch (const std::exception& e) {
        qWarning() << "Exception during OpenGL cleanup:" << e.what();
    }
}

void GLArenaWidget::initializeGL()
{
    // Initialize OpenGL functions
    initializeOpenGLFunctions();
    
    // Set up OpenGL state
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Initialize shaders
    if (!initShaders()) {
        qWarning() << "Failed to initialize shaders";
        return;
    }
    
    // Create arena geometry
    createArena(m_arenaRadius, m_wallHeight);
    
    // Create floor
    createFloor(m_arenaRadius);
    
    // Create grid
    createGrid(m_arenaRadius * 2, 20);
    
    // Create player entity
    m_playerController->createPlayerEntity();
    m_playerController->startUpdates();
    
    m_initialized = true;
    
    // Emit signal properly
    emit renderingInitialized();
}

void GLArenaWidget::resizeGL(int w, int h)
{
    // Update projection matrix when widget resizes
    float aspectRatio = float(w) / float(h ? h : 1);
    const float zNear = 0.1f, zFar = 100.0f, fov = 60.0f;
    
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(fov, aspectRatio, zNear, zFar);
}

void GLArenaWidget::initializeArena(double radius, double wallHeight)
{
    // Store parameters to be applied during OpenGL initialization
    m_arenaRadius = radius;
    m_wallHeight = wallHeight;
    
    // If already initialized, recreate the arena
    if (m_initialized && isValid()) {
        makeCurrent();
        
        // Remove existing entities from game scene
        for (int i = 0; i < 8; i++) {
            m_gameScene->removeEntity(QString("arena_wall_%1").arg(i));
        }
        m_gameScene->removeEntity("arena_floor");
        
        // Clear existing walls vector
        m_walls.clear();
        
        // Remove floor geometry
        if (m_floorVBO.isCreated()) m_floorVBO.destroy();
        if (m_floorIBO.isCreated()) m_floorIBO.destroy();
        if (m_floorVAO.isCreated()) m_floorVAO.destroy();
        
        // Create new geometries
        createArena(radius, wallHeight);
        createFloor(radius);
        
        doneCurrent();
        update();
    }
}

void GLArenaWidget::setActiveCharacter(const QString& name)
{
    m_activeCharacter = name;
}

QVector3D GLArenaWidget::worldToNDC(const QVector3D& worldPos)
{
    // Convert world coordinates to normalized device coordinates (for debugging)
    QVector4D clipSpace = m_projectionMatrix * m_viewMatrix * QVector4D(worldPos, 1.0f);
    QVector3D ndcSpace;
    
    if (clipSpace.w() != 0.0f) {
        ndcSpace = QVector3D(clipSpace.x() / clipSpace.w(), 
                            clipSpace.y() / clipSpace.w(),
                            clipSpace.z() / clipSpace.w());
    }
    
    return ndcSpace;
}

void GLArenaWidget::onPlayerPositionChanged(const QVector3D& position)
{
    // Trigger redraw when player position changes
    emit playerPositionUpdated(position.x(), position.y(), position.z());
    update();
}

void GLArenaWidget::onPlayerRotationChanged(float rotation)
{
    // Trigger redraw when player rotation changes
    QVector3D pos = m_playerController->getPosition();
    emit playerPositionUpdated(pos.x(), pos.y(), pos.z());
    update();
}