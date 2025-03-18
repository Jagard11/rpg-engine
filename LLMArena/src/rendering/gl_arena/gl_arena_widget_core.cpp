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
#include <cmath>
#include <vector>

GLArenaWidget::GLArenaWidget(CharacterManager* charManager, QWidget* parent)
    : QOpenGLWidget(parent), m_characterManager(charManager), 
      m_initialized(false),
      m_billboardProgram(nullptr),
      m_voxelSystem(nullptr)  // Initialize to nullptr
{
    setFocusPolicy(Qt::StrongFocus);
    m_gameScene = new GameScene(this);
    m_playerController = new PlayerController(m_gameScene, this);
    
    connect(m_playerController, &PlayerController::positionChanged, 
            this, &GLArenaWidget::onPlayerPositionChanged);
    connect(m_playerController, &PlayerController::rotationChanged, 
            this, &GLArenaWidget::onPlayerRotationChanged);
    
    // Don't create voxel system yet - we'll do it in initializeGL
    
    QTimer* updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, QOverload<>::of(&QWidget::update));
    updateTimer->start(50);
}

GLArenaWidget::~GLArenaWidget()
{
    makeCurrent();
    
    // Clean up character sprites
    for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
        if (it.value()) delete it.value();
    }
    m_characterSprites.clear();
    
    // Clean up shader programs
    delete m_billboardProgram;
    
    // Delete voxel system
    delete m_voxelSystem;
    
    doneCurrent();
}

void GLArenaWidget::initializeGL()
{
    initializeOpenGLFunctions();
    
    // Initialize basic OpenGL settings
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Initialize shaders for character billboards
    initShaders();
    
    try {
        // Create and initialize voxel system
        m_voxelSystem = new VoxelSystemIntegration(m_gameScene, this);
        
        // Initialize the voxel system with a try-catch to handle any errors
        try {
            m_voxelSystem->initialize();
            qDebug() << "Voxel system successfully initialized";
        } catch (const std::exception& e) {
            qCritical() << "Failed to initialize voxel system:" << e.what();
            // Keep the voxel system object but mark it as not initialized
        }
    } catch (const std::exception& e) {
        qCritical() << "Failed to create voxel system:" << e.what();
        // Don't throw here, just continue without the voxel system
    }
    
    // Create player entity at default position
    m_playerController->createPlayerEntity();
    m_playerController->startUpdates();
    
    // Mark initialization as complete
    m_initialized = true;
    
    // Signal that rendering is initialized
    emit renderingInitialized();
}

void GLArenaWidget::resizeGL(int w, int h)
{
    float aspectRatio = float(w) / float(h ? h : 1);
    const float zNear = 0.1f, zFar = 100.0f, fov = 60.0f;
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(fov, aspectRatio, zNear, zFar);
}

// Initialize arena with dimensions
void GLArenaWidget::initializeArena(double width, double height)
{
    // Check if we're properly initialized
    if (!m_initialized) {
        qWarning() << "Cannot initialize arena: OpenGL not initialized";
        return;
    }
    
    // Check if voxel system is initialized with more details
    if (!m_voxelSystem) {
        qWarning() << "Cannot initialize arena: voxel system not initialized";
        return;
    }
    
    try {
        // Store dimensions
        m_arenaRadius = width / 2.0;
        m_wallHeight = height;
        
        // Create a default room with the voxel system
        qDebug() << "Creating default world in voxel system";
        m_voxelSystem->createDefaultWorld();
        
        qDebug() << "Arena initialized with radius" << m_arenaRadius << "and height" << m_wallHeight;
    } catch (const std::exception& e) {
        qCritical() << "Exception in initializeArena:" << e.what();
    }
}

// Helper function to check OpenGL errors
static void checkGLError(const char* step) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        qCritical() << "OpenGL error at" << step << ":" << err;
    }
}