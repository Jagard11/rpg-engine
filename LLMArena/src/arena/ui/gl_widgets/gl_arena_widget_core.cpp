// src/arena/ui/gl_widgets/gl_arena_widget_core.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include "../../../../include/arena/debug/debug_system.h"
#include <QDebug>
#include <QMouseEvent>
#include <QCursor>
#include <QApplication>
#include <QtMath>

GLArenaWidget::GLArenaWidget(CharacterManager* charManager, QWidget* parent)
    : QOpenGLWidget(parent),
      m_characterManager(charManager),
      m_gameScene(new GameScene(this)),
      m_playerController(new PlayerController(m_gameScene, this)),
      m_activeCharacter(""),
      m_voxelSystem(nullptr),
      m_billboardProgram(nullptr),
      m_initialized(false),
      m_arenaRadius(10.0),
      m_wallHeight(3.0),
      m_floorIndexCount(0),
      m_gridVertexCount(0)
{
    // Set focus policy to receive keyboard events
    setFocusPolicy(Qt::StrongFocus);
    
    // Set mouse tracking for camera control
    setMouseTracking(true);
    
    // Create voxel system
    try {
        qDebug() << "Creating VoxelSystemIntegration...";
        m_voxelSystem = new VoxelSystemIntegration(m_gameScene, this);
        qDebug() << "VoxelSystemIntegration created successfully";
    } catch (const std::exception& e) {
        qCritical() << "Failed to create VoxelSystemIntegration:" << e.what();
        m_voxelSystem = nullptr;
    } catch (...) {
        qCritical() << "Unknown exception creating VoxelSystemIntegration";
        m_voxelSystem = nullptr;
    }
    
    // Connect player signals
    connect(m_playerController, &PlayerController::positionChanged,
            this, &GLArenaWidget::onPlayerPositionChanged);
    connect(m_playerController, &PlayerController::rotationChanged,
            this, &GLArenaWidget::onPlayerRotationChanged);
    connect(m_playerController, &PlayerController::pitchChanged,
            this, &GLArenaWidget::onPlayerPitchChanged);
    
    // Set initial projection matrix
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(70.0f, width() / static_cast<float>(height()), 0.1f, 100.0f);
    
    // Set initial view matrix
    m_viewMatrix.setToIdentity();
    
    // Initialize debug system
    try {
        initializeDebugSystem();
    } catch (const std::exception& e) {
        qWarning() << "Failed to initialize debug system:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception initializing debug system";
    }
}

GLArenaWidget::~GLArenaWidget() {
    // Make sure the context is current when deleting OpenGL resources
    makeCurrent();
    
    // Clean up shader program
    delete m_billboardProgram;
    
    // Delete character sprites
    for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
        delete it.value();
    }
    m_characterSprites.clear();
    
    // Delete inventory UI
    delete m_inventoryUI;
    
    // Delete inventory
    delete m_inventory;
    
    // Delete voxel system
    delete m_voxelSystem;
    
    // Clean up game scene resources
    delete m_gameScene;
    
    doneCurrent();
}

void GLArenaWidget::initializeGL() {
    // Initialize OpenGL functions
    initializeOpenGLFunctions();
    
    // Set up OpenGL state
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // Initialize shaders
    if (!initShaders()) {
        qCritical() << "Failed to initialize shaders";
        return;
    }
    
    // Initialize arena if not already done
    if (m_arenaRadius > 0.1) {
        // Use stored values
        try {
            createFloor(m_arenaRadius);
            createGrid(m_arenaRadius * 2, 20);
            createArena(m_arenaRadius, m_wallHeight);
        } catch (const std::exception& e) {
            qCritical() << "Failed to create arena geometry:" << e.what();
        }
    } else {
        // Use default values
        m_arenaRadius = 10.0;
        m_wallHeight = 3.0;
        try {
            createFloor(m_arenaRadius);
            createGrid(m_arenaRadius * 2, 20);
            createArena(m_arenaRadius, m_wallHeight);
        } catch (const std::exception& e) {
            qCritical() << "Failed to create default arena geometry:" << e.what();
        }
    }
    
    // Set up player controller in the scene
    m_playerController->createPlayerEntity();
    m_playerController->startUpdates();
    
    // Update player screen dimensions
    m_playerController->setScreenDimensions(width(), height());
    
    // Initialize voxel system
    if (m_voxelSystem) {
        try {
            qDebug() << "Initializing VoxelSystemIntegration...";
            m_voxelSystem->initialize();
            qDebug() << "VoxelSystemIntegration initialization complete";
            
            // Create default voxel world
            qDebug() << "Creating default world...";
            m_voxelSystem->createDefaultWorld();
        } catch (const std::exception& e) {
            qCritical() << "Failed to initialize voxel system:" << e.what();
        } catch (...) {
            qCritical() << "Unknown exception initializing voxel system";
        }
    }
    
    // Initialize inventory system
    try {
        initializeInventory();
    } catch (const std::exception& e) {
        qWarning() << "Failed to initialize inventory:" << e.what();
    }
    
    m_initialized = true;
    emit renderingInitialized();
}

void GLArenaWidget::resizeGL(int w, int h) {
    // Update projection matrix for new aspect ratio
    float aspect = w / static_cast<float>(h);
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(70.0f, aspect, 0.1f, 100.0f);
    
    // Update player controller screen dimensions
    m_playerController->setScreenDimensions(w, h);
    
    // Center mouse cursor
    QCursor::setPos(mapToGlobal(QPoint(w/2, h/2)));
}

void GLArenaWidget::paintGL() {
    // Skip rendering if not initialized
    if (!m_initialized) {
        return;
    }
    
    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Update view matrix from player controller
    if (m_playerController) {
        QVector3D position = m_playerController->getPosition();
        float rotation = m_playerController->getRotation();
        float pitch = m_playerController->getPitch();
        
        // Set up camera view matrix
        m_viewMatrix.setToIdentity();
        m_viewMatrix.rotate(-pitch * 180.0f / M_PI, 1.0f, 0.0f, 0.0f);
        m_viewMatrix.rotate(-rotation * 180.0f / M_PI, 0.0f, 1.0f, 0.0f);
        m_viewMatrix.translate(-position);
    }
    
    try {
        // Render basic scene elements
        renderFloor();
        renderGrid();
        renderWalls();
        
        // Render characters
        renderCharacters();
        
        // Render voxel system
        if (m_voxelSystem) {
            m_voxelSystem->render(m_viewMatrix, m_projectionMatrix);
        }
        
        // Render UI elements
        renderInventory();
        
        // Render debug overlays last (on top of everything)
        renderDebugSystem();
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in paintGL:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown exception in paintGL";
    }
}

bool GLArenaWidget::initShaders() {
    // Create billboard shader program
    m_billboardProgram = new QOpenGLShaderProgram();
    
    // Vertex shader
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec3 normal;
        layout(location = 2) in vec2 texCoord;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        out vec3 fragPosition;
        out vec3 fragNormal;
        out vec2 fragTexCoord;
        
        void main() {
            fragPosition = vec3(model * vec4(position, 1.0));
            fragNormal = mat3(transpose(inverse(model))) * normal;
            fragTexCoord = texCoord;
            gl_Position = projection * view * model * vec4(position, 1.0);
        }
    )";
    
    // Fragment shader
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 fragPosition;
        in vec3 fragNormal;
        in vec2 fragTexCoord;
        
        uniform vec4 color;
        uniform sampler2D textureSampler;
        uniform bool useTexture;
        
        out vec4 fragColor;
        
        void main() {
            vec4 baseColor = color;
            
            // Use texture if enabled
            if (useTexture) {
                baseColor = texture(textureSampler, fragTexCoord);
                if (baseColor.a < 0.1) discard; // Transparency handling
            }
            
            // Simple lighting
            vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
            float diff = max(dot(fragNormal, lightDir), 0.0);
            vec3 ambient = vec3(0.3);
            vec3 diffuse = vec3(0.7) * diff;
            
            fragColor = vec4((ambient + diffuse) * baseColor.rgb, baseColor.a);
        }
    )";
    
    // Compile and link shaders
    bool success = true;
    
    if (!m_billboardProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qCritical() << "Failed to compile vertex shader:" << m_billboardProgram->log();
        success = false;
    }
    
    if (!m_billboardProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qCritical() << "Failed to compile fragment shader:" << m_billboardProgram->log();
        success = false;
    }
    
    if (!m_billboardProgram->link()) {
        qCritical() << "Failed to link shader program:" << m_billboardProgram->log();
        success = false;
    }
    
    if (success) {
        qDebug() << "Shaders initialized successfully";
    }
    
    return success;
}

void GLArenaWidget::updateMouseTrackingState() {
    // Only capture mouse if not in inventory mode and window has focus
    if (m_inventory && m_inventoryUI && m_inventoryUI->isVisible()) {
        // Inventory is open, release mouse
        setMouseTracking(false);
        setCursor(Qt::ArrowCursor);
    } else if (hasFocus()) {
        // Window has focus and inventory is closed, capture mouse
        setMouseTracking(true);
        setCursor(Qt::BlankCursor);
    } else {
        // Window does not have focus, release mouse
        setMouseTracking(false);
        setCursor(Qt::ArrowCursor);
    }
}

void GLArenaWidget::initializeArena(double radius, double height) {
    m_arenaRadius = radius;
    m_wallHeight = height;
    
    if (!m_initialized) {
        // Will be initialized when OpenGL context is ready
        return;
    }
    
    // Make current context for OpenGL operations
    makeCurrent();
    
    // Create arena geometry
    try {
        createFloor(radius);
        createGrid(radius * 2, 20);
        createArena(radius, height);
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in initializeArena:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown exception in initializeArena";
    }
    
    // Create player entity
    m_playerController->createPlayerEntity();
    m_playerController->startUpdates();
    
    // Update rendering
    update();
    
    doneCurrent();
}

void GLArenaWidget::setActiveCharacter(const QString& name) {
    m_activeCharacter = name;
    
    // Load character sprite if it doesn't exist
    if (!m_characterSprites.contains(name) && !name.isEmpty()) {
        // Try to get character data from the manager
        try {
            // Use a default texture path - the actual path would need to be 
            // determined based on your character system implementation
            QString defaultTexturePath = "resources/default_character.png";
            
            // Check if a custom method exists to get specific character textures
            // Could check a character-specific directory or use character name
            QString possibleTexturePath = "resources/" + name + ".png";
            
            // Check if file exists
            QFile file(possibleTexturePath);
            if (file.exists()) {
                loadCharacterSprite(name, possibleTexturePath);
                qDebug() << "Loaded character texture from" << possibleTexturePath;
            } else {
                // Try default path
                loadCharacterSprite(name, defaultTexturePath);
                qDebug() << "Using default character texture" << defaultTexturePath;
            }
        } catch (const std::exception& e) {
            qWarning() << "Failed to load character sprite:" << e.what();
        } catch (...) {
            qWarning() << "Unknown exception loading character sprite";
        }
    }
    
    update();
}

void GLArenaWidget::loadCharacterSprite(const QString& characterName, const QString& texturePath) {
    // Make sure we have a context
    if (!isValid()) {
        qWarning() << "Cannot load sprite without valid OpenGL context";
        return;
    }
    
    makeCurrent();
    
    // Clean up existing sprite if any
    if (m_characterSprites.contains(characterName)) {
        delete m_characterSprites[characterName];
        m_characterSprites.remove(characterName);
    }
    
    // Create new sprite
    CharacterSprite* sprite = new CharacterSprite();
    
    // Initialize sprite with dimensions
    try {
        sprite->init(context(), texturePath, 1.0, 2.0, 0.1);
        m_characterSprites[characterName] = sprite;
    }
    catch (const std::exception& e) {
        qWarning() << "Failed to load character sprite:" << e.what();
        delete sprite;
    }
    
    doneCurrent();
    update();
}

void GLArenaWidget::updateCharacterPosition(const QString& characterName, float x, float y, float z) {
    // Update sprite position if it exists
    if (m_characterSprites.contains(characterName)) {
        m_characterSprites[characterName]->updatePosition(x, y, z);
    }
    
    // Update position in game scene
    GameEntity entity;
    entity.id = characterName;
    entity.type = "character";
    entity.position = QVector3D(x, y, z);
    entity.dimensions = QVector3D(0.6f, 1.8f, 0.6f);
    entity.isStatic = false;
    
    m_gameScene->addEntity(entity);
    
    update();
}

PlayerController* GLArenaWidget::getPlayerController() const {
    return m_playerController;
}