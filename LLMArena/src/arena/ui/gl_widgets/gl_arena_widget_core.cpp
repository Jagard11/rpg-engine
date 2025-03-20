// src/arena/ui/gl_widgets/gl_arena_widget_core.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include "../../include/game/game_scene.h"
#include "../../include/game/player_controller.h"
#include <QWebEngineSettings>
#include <QWebEngineScript>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebChannel>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QDebug>

// Forward declaration of static cleanup function
void cleanupStaticGLResources();

GLArenaWidget::GLArenaWidget(CharacterManager* charManager, QWidget* parent) 
    : QOpenGLWidget(parent), 
      m_characterManager(charManager), 
      m_gameScene(nullptr),
      m_playerController(nullptr),
      m_initialized(false),
      m_billboardProgram(nullptr),
      m_voxelSystem(nullptr),
      m_inventory(nullptr),
      m_inventoryUI(nullptr),
      m_highlightedVoxelFace(-1),
      m_arenaRadius(10.0),
      m_wallHeight(2.0)
{
    setFocusPolicy(Qt::StrongFocus);
    
    // Create game scene with proper parent
    m_gameScene = new GameScene(this);
    
    // Create player controller with proper parent
    m_playerController = new PlayerController(m_gameScene, this);
    
    // Connect signals with safety checks
    if (m_playerController) {
        connect(m_playerController, &PlayerController::positionChanged, 
                this, &GLArenaWidget::onPlayerPositionChanged);
        connect(m_playerController, &PlayerController::rotationChanged, 
                this, &GLArenaWidget::onPlayerRotationChanged);
        connect(m_playerController, &PlayerController::pitchChanged, 
                this, &GLArenaWidget::onPlayerPitchChanged);
    }
    
    // Set up a timer for regular updates with safety checks
    QTimer* updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, QOverload<>::of(&QWidget::update));
    updateTimer->start(50);
}

GLArenaWidget::~GLArenaWidget()
{
    try {
        // Check if context is valid before doing OpenGL operations
        if (context() && context()->isValid()) {
            // Lock the context for thread safety
            makeCurrent();
            
            // Clean up character sprites
            for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
                if (it.value()) {
                    delete it.value();
                }
            }
            m_characterSprites.clear();
            
            // Clean up shader programs
            delete m_billboardProgram;
            m_billboardProgram = nullptr;
            
            // Clean up voxel system
            delete m_voxelSystem;
            m_voxelSystem = nullptr;
            
            // Clean up static resources
            cleanupStaticGLResources();
            
            doneCurrent();
        } else {
            // If context is invalid, just delete the pointers without
            // trying to clean up OpenGL resources
            for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
                delete it.value();
            }
            m_characterSprites.clear();
            
            delete m_billboardProgram;
            m_billboardProgram = nullptr;
            
            delete m_voxelSystem;
            m_voxelSystem = nullptr;
        }
        
        // Inventory objects will be deleted by Qt parent/child relationship
    } 
    catch (const std::exception& e) {
        qCritical() << "Exception in GLArenaWidget destructor:" << e.what();
    } 
    catch (...) {
        qCritical() << "Unknown exception in GLArenaWidget destructor";
    }
}

void GLArenaWidget::initializeGL()
{
    try {
        // Initialize OpenGL functions
        initializeOpenGLFunctions();
        
        // Make sure resource directory exists
        QString resourcePath = QDir::currentPath() + "/resources/";
        QDir resourceDir(resourcePath);
        if (!resourceDir.exists()) {
            qDebug() << "Creating resources directory";
            QDir().mkpath(resourcePath);
        }
        
        // Initialize basic OpenGL settings
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Initialize shaders for character billboards
        bool shadersInitialized = initShaders();
        if (!shadersInitialized) {
            qWarning() << "Failed to initialize shaders";
        }
        
        // Create player entity at default position (before voxel system)
        if (m_playerController) {
            m_playerController->createPlayerEntity();
        }
        
        // Initialize inventory first (before voxel system)
        try {
            qDebug() << "Creating inventory...";
            m_inventory = new Inventory(this);
            
            // Initialize inventory (after shaders, before voxel system)
            if (shadersInitialized) {
                QTimer::singleShot(0, this, [this]() {
                    try {
                        if (m_inventory && context() && context()->isValid()) {
                            makeCurrent();
                            
                            qDebug() << "Creating inventory UI...";
                            m_inventoryUI = new InventoryUI(m_inventory, this);
                            
                            if (m_inventoryUI) {
                                qDebug() << "Initializing inventory UI...";
                                m_inventoryUI->initialize();
                                
                                // Connect UI signals
                                connect(m_inventoryUI, &InventoryUI::visibilityChanged,
                                        this, &GLArenaWidget::onInventoryVisibilityChanged);
                            }
                            
                            doneCurrent();
                        }
                    } catch (const std::exception& e) {
                        qCritical() << "Exception initializing inventory UI:" << e.what();
                    }
                });
            }
        } catch (const std::exception& e) {
            qCritical() << "Failed to create inventory system:" << e.what();
        }
        
        // Create and initialize voxel system using a timer to delay
        QTimer::singleShot(100, this, [this]() {
            try {
                if (context() && context()->isValid()) {
                    makeCurrent();
                    
                    qDebug() << "Creating voxel system...";
                    m_voxelSystem = new VoxelSystemIntegration(m_gameScene, this);
                    
                    // Initialize the voxel system with a try-catch to handle any errors
                    if (m_voxelSystem) {
                        qDebug() << "Initializing voxel system...";
                        m_voxelSystem->initialize();
                        
                        // Create default world after initialization
                        QTimer::singleShot(100, this, [this]() {
                            if (m_voxelSystem) {
                                qDebug() << "Creating default world...";
                                m_voxelSystem->createDefaultWorld();
                            }
                        });
                    }
                    
                    doneCurrent();
                }
            } catch (const std::exception& e) {
                qCritical() << "Failed to initialize voxel system:" << e.what();
            }
        });
        
        // Start player controller updates
        if (m_playerController) {
            QTimer::singleShot(200, this, [this]() {
                if (m_playerController) {
                    qDebug() << "Starting player controller updates...";
                    m_playerController->startUpdates();
                }
            });
        }
        
        // Mark initialization as complete
        m_initialized = true;
        
        // Signal that rendering is initialized
        QTimer::singleShot(300, this, [this]() {
            emit renderingInitialized();
        });
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in GLArenaWidget::initializeGL:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown exception in GLArenaWidget::initializeGL";
    }
}

void GLArenaWidget::resizeGL(int w, int h)
{
    try {
        // Validate dimensions
        if (w <= 0 || h <= 0) {
            qWarning() << "Invalid viewport dimensions in resizeGL:" << w << "x" << h;
            return;
        }
        
        float aspectRatio = float(w) / float(h ? h : 1);
        const float zNear = 0.1f, zFar = 100.0f, fov = 60.0f;
        
        // Reset projection matrix
        m_projectionMatrix.setToIdentity();
        m_projectionMatrix.perspective(fov, aspectRatio, zNear, zFar);
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in GLArenaWidget::resizeGL:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception in GLArenaWidget::resizeGL";
    }
}

// Initialize arena with dimensions
void GLArenaWidget::initializeArena(double width, double height)
{
    // Check if we're properly initialized
    if (!m_initialized) {
        qWarning() << "Cannot initialize arena: OpenGL not initialized";
        return;
    }
    
    // Store dimensions
    m_arenaRadius = width / 2.0;
    m_wallHeight = height;
    
    // Create the voxel system if it doesn't exist yet
    if (!m_voxelSystem) {
        try {
            qDebug() << "Creating voxel system in initializeArena...";
            m_voxelSystem = new VoxelSystemIntegration(m_gameScene, this);
            
            if (m_voxelSystem) {
                qDebug() << "Initializing voxel system in initializeArena...";
                m_voxelSystem->initialize();
                
                // Create a default world once the system is initialized
                QTimer::singleShot(100, this, [this]() {
                    if (m_voxelSystem) {
                        qDebug() << "Creating default world in initializeArena...";
                        m_voxelSystem->createDefaultWorld();
                    }
                });
            }
        } catch (const std::exception& e) {
            qWarning() << "Exception creating voxel system in initializeArena:" << e.what();
        }
        return;
    }
    
    // Create a default room with the voxel system
    QTimer::singleShot(0, this, [this]() {
        try {
            if (m_voxelSystem) {
                qDebug() << "Creating default world in initializeArena (existing system)...";
                m_voxelSystem->createDefaultWorld();
            }
        } catch (const std::exception& e) {
            qWarning() << "Exception in initializeArena:" << e.what();
        }
    });
}