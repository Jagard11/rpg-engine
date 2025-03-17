// src/rendering/gl_arena_widget.cpp
// src/gl_arena_widget.cpp - Fixed version with improved OpenGL handling
#include "../include/rendering/gl_arena_widget.h"
#include <QImage>
#include <QDebug>
#include <QPainter>
#include <QFileInfo>
#include <cmath>

// Vertex struct for passing to OpenGL
struct Vertex {
    QVector3D position;
    QVector3D normal;
    QVector2D texCoord;
};

// Shader source for basic rendering (walls, floor)
const char* basicVertexShaderSource =
    "#version 330 core\n"
    "layout(location = 0) in vec3 position;\n"
    "layout(location = 1) in vec3 normal;\n"
    "layout(location = 2) in vec2 texCoord;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "out vec3 fragNormal;\n"
    "out vec2 fragTexCoord;\n"
    "out vec3 fragPos;\n"
    "void main() {\n"
    "    gl_Position = projection * view * model * vec4(position, 1.0);\n"
    "    fragNormal = mat3(transpose(inverse(model))) * normal;\n"
    "    fragTexCoord = texCoord;\n"
    "    fragPos = vec3(model * vec4(position, 1.0));\n"
    "}\n";

const char* basicFragmentShaderSource =
    "#version 330 core\n"
    "in vec3 fragNormal;\n"
    "in vec2 fragTexCoord;\n"
    "in vec3 fragPos;\n"
    "uniform vec3 objectColor;\n"
    "uniform vec3 lightPos;\n"
    "uniform vec3 viewPos;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    // Ambient lighting\n"
    "    float ambientStrength = 0.3;\n"
    "    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);\n"
    "    \n"
    "    // Diffuse lighting\n"
    "    vec3 norm = normalize(fragNormal);\n"
    "    vec3 lightDir = normalize(lightPos - fragPos);\n"
    "    float diff = max(dot(norm, lightDir), 0.0);\n"
    "    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);\n"
    "    \n"
    "    // Specular lighting\n"
    "    float specularStrength = 0.5;\n"
    "    vec3 viewDir = normalize(viewPos - fragPos);\n"
    "    vec3 reflectDir = reflect(-lightDir, norm);\n"
    "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
    "    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);\n"
    "    \n"
    "    vec3 result = (ambient + diffuse + specular) * objectColor;\n"
    "    fragColor = vec4(result, 1.0);\n"
    "}\n";

// Simplified billboard shader source for more robust operation
const char* billboardVertexShaderSource =
    "#version 330 core\n"
    "layout(location = 0) in vec3 position;\n"
    "layout(location = 1) in vec2 texCoord;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "uniform vec3 cameraRight;\n"
    "uniform vec3 cameraUp;\n"
    "uniform vec3 billboardPos;\n"
    "uniform vec2 billboardSize;\n"
    "out vec2 fragTexCoord;\n"
    "void main() {\n"
    "    vec3 vertPos = billboardPos;\n"
    "    vertPos += cameraRight * position.x * billboardSize.x;\n"
    "    vertPos += cameraUp * position.y * billboardSize.y;\n"
    "    gl_Position = projection * view * vec4(vertPos, 1.0);\n"
    "    fragTexCoord = texCoord;\n"
    "}\n";

const char* billboardFragmentShaderSource =
    "#version 330 core\n"
    "in vec2 fragTexCoord;\n"
    "uniform sampler2D textureSampler;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    vec4 texColor = texture(textureSampler, fragTexCoord);\n"
    "    fragColor = texColor;\n"
    "}\n";

// Shader source for grid
const char* gridVertexShaderSource =
    "#version 330 core\n"
    "layout(location = 0) in vec3 position;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "    gl_Position = projection * view * model * vec4(position, 1.0);\n"
    "}\n";

const char* gridFragmentShaderSource =
    "#version 330 core\n"
    "uniform vec3 lineColor;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    fragColor = vec4(lineColor, 0.5);\n"
    "}\n";

/////////////////////////////////////////////////////////////////////////////
// CharacterSprite Implementation
/////////////////////////////////////////////////////////////////////////////

CharacterSprite::CharacterSprite()
    : m_texture(nullptr), m_width(1.0f), m_height(2.0f), m_depth(1.0f),
      m_vertexBuffer(QOpenGLBuffer::VertexBuffer), m_indexBuffer(QOpenGLBuffer::IndexBuffer)
{
}

CharacterSprite::~CharacterSprite()
{
    try {
        // Release all OpenGL resources in a safe way
        if (m_vertexBuffer.isCreated()) {
            m_vertexBuffer.destroy();
        }
        if (m_indexBuffer.isCreated()) {
            m_indexBuffer.destroy();
        }
        if (m_vao.isCreated()) {
            m_vao.destroy();
        }
        
        // Delete texture with a safety check
        if (m_texture) {
            // Only delete if it's created, otherwise just leak it to avoid crash
            if (m_texture->isCreated()) {
                m_texture->destroy();
            }
            delete m_texture;
            m_texture = nullptr;
        }
    } catch (const std::exception& e) {
        qWarning() << "Exception in CharacterSprite destructor:" << e.what();
        // We can't do much in a destructor except prevent the app from crashing
    }
}

void CharacterSprite::init(QOpenGLContext* context, const QString& texturePath, 
                          double width, double height, double depth)
{
    // Make sure we have a valid context
    if (!context || !context->isValid()) {
        qWarning() << "CharacterSprite init: Invalid OpenGL context";
        return;
    }
    
    // Initialize OpenGL functions with the provided context
    initializeOpenGLFunctions();
    
    m_width = width;
    m_height = height;
    m_depth = depth;
    
    // First delete any existing texture to prevent memory leaks
    if (m_texture) {
        delete m_texture;
        m_texture = nullptr;
    }
    
    // Create a new texture
    m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    
    // Create a default image for cases where we can't load the requested texture
    QImage defaultImage(64, 128, QImage::Format_RGBA8888);
    defaultImage.fill(Qt::transparent);
    
    // Draw a simple figure for default texture
    QPainter painter(&defaultImage);
    painter.setPen(Qt::black);
    painter.setBrush(QColor(255, 0, 255)); // Pink for default
    painter.fillRect(QRect(0, 0, 64, 128), QColor(255, 0, 255));
    painter.setPen(Qt::white);
    painter.drawRect(16, 16, 32, 96);
    painter.end();
    
    // The actual image we'll use
    QImage textureImage = defaultImage;
    
    // Try to load the requested texture, but only if we have a valid texture path
    if (!texturePath.isEmpty()) {
        // Try to load the image safely
        bool loaded = false;
        try {
            loaded = textureImage.load(texturePath);
            qDebug() << "Loaded image from path:" << texturePath << "success:" << loaded;
        } catch (const std::exception& e) {
            qWarning() << "Exception loading image from path:" << e.what();
            // We'll keep using the default image in this case
        }
        
        if (!loaded || textureImage.isNull()) {
            qDebug() << "Could not load texture, using default:" << texturePath;
            textureImage = defaultImage; // Ensure we use the default image
        } else {
            // Convert to a format suitable for textures if needed
            if (textureImage.format() != QImage::Format_RGBA8888 && 
                textureImage.format() != QImage::Format_RGB888) {
                textureImage = textureImage.convertToFormat(QImage::Format_RGBA8888);
            }
        }
    }
    
    // IMPORTANT: Configure texture parameters BEFORE allocating storage
    // This is the key fix - set all parameters before adding data
    m_texture->create();
    m_texture->setFormat(QOpenGLTexture::RGBA8_UNorm);
    m_texture->setMinificationFilter(QOpenGLTexture::Linear);
    m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);
    m_texture->setSize(textureImage.width(), textureImage.height());
    
    // Now we can safely set the data
    m_texture->setData(textureImage);
    
    // Create a simple quad for the billboard with a safety-first approach
    // Use a safer static array with explicit size to avoid potential memory issues
    static const float vertices[] = {
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, // Bottom left
         0.5f, -0.5f, 0.0f,  1.0f, 1.0f, // Bottom right
         0.5f,  0.5f, 0.0f,  1.0f, 0.0f, // Top right
        -0.5f,  0.5f, 0.0f,  0.0f, 0.0f  // Top left
    };
    
    static const GLuint indices[] = {
        0, 1, 2,    // First triangle
        2, 3, 0     // Second triangle
    };
    
    // Make sure we have a valid texture before creating buffers
    if (!m_texture->isCreated()) {
        qWarning() << "Cannot create billboard without a valid texture";
        return;
    }
    
    // Initialize OpenGL buffers with try-catch for safety
    try {
        // Clean up any existing buffers
        if (m_vertexBuffer.isCreated()) {
            m_vertexBuffer.destroy();
        }
        if (m_indexBuffer.isCreated()) {
            m_indexBuffer.destroy();
        }
        if (m_vao.isCreated()) {
            m_vao.destroy();
        }
        
        // VAO must be created and bound first
        if (!m_vao.create()) {
            qWarning() << "Failed to create vertex array object";
            return;
        }
        m_vao.bind();
        
        // Create and set up vertex buffer
        if (!m_vertexBuffer.create()) {
            qWarning() << "Failed to create vertex buffer";
            m_vao.release();
            return;
        }
        m_vertexBuffer.bind();
        m_vertexBuffer.allocate(vertices, sizeof(vertices));
        
        // Create and set up index buffer
        if (!m_indexBuffer.create()) {
            qWarning() << "Failed to create index buffer";
            m_vertexBuffer.release();
            m_vao.release();
            return;
        }
        m_indexBuffer.bind();
        m_indexBuffer.allocate(indices, sizeof(indices));
        
        // Position attribute - using our own QOpenGLFunctions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
        
        // Texture coordinate attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
        
        // Clean up
        m_indexBuffer.release();
        m_vertexBuffer.release();
        m_vao.release();
        
        qDebug() << "Successfully initialized billboard for sprite with texture dimensions:" 
                 << textureImage.width() << "x" << textureImage.height();
    } catch (const std::exception& e) {
        qWarning() << "Exception during buffer setup:" << e.what();
        
        // Clean up resources safely
        if (m_indexBuffer.isCreated()) {
            m_indexBuffer.destroy();
        }
        if (m_vertexBuffer.isCreated()) {
            m_vertexBuffer.destroy();
        }
        if (m_vao.isCreated()) {
            m_vao.release();
            m_vao.destroy();
        }
    }
}

void CharacterSprite::updatePosition(float x, float y, float z)
{
    m_position = QVector3D(x, y, z);
}

void CharacterSprite::render(QOpenGLShaderProgram* program, QMatrix4x4& viewMatrix, QMatrix4x4& projectionMatrix)
{
    // More extensive safety checks
    if (!program) {
        qWarning() << "Cannot render CharacterSprite: shader program is null";
        return;
    }
    
    if (!m_texture || !m_texture->isCreated()) {
        qWarning() << "Cannot render CharacterSprite: texture is not valid";
        return;
    }
    
    if (!m_vao.isCreated() || !m_vertexBuffer.isCreated() || !m_indexBuffer.isCreated()) {
        qWarning() << "Cannot render CharacterSprite: buffers not properly initialized";
        return;
    }
    
    // Track binding states
    bool programBound = false;
    bool vaoIsBound = false;
    bool textureBound = false;
    
    try {
        // Bind the shader program
        if (!program->bind()) {
            qWarning() << "Failed to bind shader program";
            return;
        }
        programBound = true;
        
        // Set up camera right and up vectors for billboarding
        QMatrix4x4 view = viewMatrix;
        QVector3D right(view(0, 0), view(1, 0), view(2, 0));
        QVector3D up(view(0, 1), view(1, 1), view(2, 1));
        
        // Set uniform values - check each one is valid in the shader
        if (program->uniformLocation("model") != -1)
            program->setUniformValue("model", QMatrix4x4()); // Identity matrix
        
        if (program->uniformLocation("view") != -1)
            program->setUniformValue("view", viewMatrix);
        
        if (program->uniformLocation("projection") != -1)
            program->setUniformValue("projection", projectionMatrix);
        
        if (program->uniformLocation("cameraRight") != -1)
            program->setUniformValue("cameraRight", right);
        
        if (program->uniformLocation("cameraUp") != -1)
            program->setUniformValue("cameraUp", up);
        
        if (program->uniformLocation("billboardPos") != -1)
            program->setUniformValue("billboardPos", m_position);
        
        if (program->uniformLocation("billboardSize") != -1)
            program->setUniformValue("billboardSize", QVector2D(m_width, m_height));
        
        // Bind texture
        m_texture->bind(0);
        textureBound = true;
        
        if (program->uniformLocation("textureSampler") != -1)
            program->setUniformValue("textureSampler", 0);
        
        // Bind VAO and draw
        m_vao.bind();
        vaoIsBound = true;
        
        // Draw the billboard quad
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        
        // Release resources in reverse order
        m_vao.release();
        vaoIsBound = false;
        
        m_texture->release();
        textureBound = false;
        
        program->release();
        programBound = false;
    } 
    catch (const std::exception& e) {
        qWarning() << "Exception in CharacterSprite::render:" << e.what();
    }
    
    // Ensure all resources are released even if there was an exception
    if (vaoIsBound) m_vao.release();
    if (textureBound && m_texture) m_texture->release();
    if (programBound && program) program->release();
}

/////////////////////////////////////////////////////////////////////////////
// GLArenaWidget Implementation
/////////////////////////////////////////////////////////////////////////////

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
            
            // Clean up character sprites
            for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
                delete it.value();
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

bool GLArenaWidget::initShaders()
{
    // Create and compile basic shader program
    m_basicProgram = new QOpenGLShaderProgram(this);
    m_basicProgram->setObjectName("basicProgram");
    
    if (!m_basicProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, basicVertexShaderSource)) {
        qWarning() << "Failed to compile basic vertex shader:" << m_basicProgram->log();
        return false;
    }
    
    if (!m_basicProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, basicFragmentShaderSource)) {
        qWarning() << "Failed to compile basic fragment shader:" << m_basicProgram->log();
        return false;
    }
    
    if (!m_basicProgram->link()) {
        qWarning() << "Failed to link basic shader program:" << m_basicProgram->log();
        return false;
    }
    qDebug() << "Basic shader program linked successfully";
    
    // Create and compile billboard shader program
    m_billboardProgram = new QOpenGLShaderProgram(this);
    m_billboardProgram->setObjectName("billboardProgram");
    
    if (!m_billboardProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, billboardVertexShaderSource)) {
        qWarning() << "Failed to compile billboard vertex shader:" << m_billboardProgram->log();
        return false;
    }
    qDebug() << "Billboard vertex shader compiled successfully";
    
    if (!m_billboardProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, billboardFragmentShaderSource)) {
        qWarning() << "Failed to compile billboard fragment shader:" << m_billboardProgram->log();
        return false;
    }
    qDebug() << "Billboard fragment shader compiled successfully";
    
    if (!m_billboardProgram->link()) {
        qWarning() << "Failed to link billboard shader program:" << m_billboardProgram->log();
        return false;
    }
    qDebug() << "Billboard shader program linked successfully";
    
    // List all uniform locations in billboard program
    qDebug() << "Billboard Shader Uniforms:";
    QList<QByteArray> uniformNames = {
        "view", "projection", "cameraRight", "cameraUp", 
        "billboardPos", "billboardSize", "textureSampler"
    };
    
    for (const QByteArray &name : uniformNames) {
        int loc = m_billboardProgram->uniformLocation(name);
        qDebug() << "  Uniform" << name << "location:" << loc;
    }
    
    // Create and compile grid shader program
    m_gridProgram = new QOpenGLShaderProgram(this);
    m_gridProgram->setObjectName("gridProgram");
    
    if (!m_gridProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, gridVertexShaderSource)) {
        qWarning() << "Failed to compile grid vertex shader:" << m_gridProgram->log();
        return false;
    }
    
    if (!m_gridProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, gridFragmentShaderSource)) {
        qWarning() << "Failed to compile grid fragment shader:" << m_gridProgram->log();
        return false;
    }
    
    if (!m_gridProgram->link()) {
        qWarning() << "Failed to link grid shader program:" << m_gridProgram->log();
        return false;
    }
    qDebug() << "Grid shader program linked successfully";
    
    return true;
}

void GLArenaWidget::resizeGL(int w, int h)
{
    // Update projection matrix when widget resizes
    float aspectRatio = float(w) / float(h ? h : 1);
    const float zNear = 0.1f, zFar = 100.0f, fov = 60.0f;
    
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(fov, aspectRatio, zNear, zFar);
}

void GLArenaWidget::paintGL()
{
    if (!m_initialized) {
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return;
    }
    
    try {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Make sure player controller exists
        if (!m_playerController) {
            qWarning() << "No player controller in paintGL!";
            return;
        }
        
        // Update view matrix based on player position and rotation
        QVector3D playerPos = m_playerController->getPosition();
        float playerRot = m_playerController->getRotation();
        
        m_viewMatrix.setToIdentity();
        
        // Position camera at player's eye level (1.6m above position)
        QVector3D eyePos = playerPos + QVector3D(0.0f, 1.6f, 0.0f);
        
        // Calculate look direction based on player rotation
        QVector3D lookDir(cos(playerRot), 0.0f, sin(playerRot));
        QVector3D lookAt = eyePos + lookDir;
        
        m_viewMatrix.lookAt(eyePos, lookAt, QVector3D(0.0f, 1.0f, 0.0f));
        
        // Render scene
        renderArena();
        
        // Render characters
        renderCharacters();
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in GLArenaWidget::paintGL:" << e.what();
    }
}

void GLArenaWidget::createArena(double radius, double wallHeight)
{
    // Store arena parameters
    m_arenaRadius = radius;
    m_wallHeight = wallHeight;
    
    // Create octagonal arena walls
    const int numWalls = 8;
    m_walls.clear();
    m_walls.resize(numWalls);
    
    for (int i = 0; i < numWalls; i++) {
        const double angle1 = 2.0 * M_PI * i / numWalls;
        const double angle2 = 2.0 * M_PI * (i + 1) / numWalls;
        
        const double x1 = radius * cos(angle1);
        const double z1 = radius * sin(angle1);
        const double x2 = radius * cos(angle2);
        const double z2 = radius * sin(angle2);
        
        // Compute wall normal (pointing inward)
        QVector3D wallDir(x2 - x1, 0.0, z2 - z1);
        QVector3D normal = QVector3D::crossProduct(wallDir, QVector3D(0.0, 1.0, 0.0)).normalized();
        normal = -normal; // Invert to point inward
        
        // Calculate wall length
        float wallLength = QVector2D(x2 - x1, z2 - z1).length();
        
        // Create vertices for wall
        Vertex vertices[4];
        // Bottom left
        vertices[0].position = QVector3D(x1, 0.0, z1);
        vertices[0].normal = normal;
        vertices[0].texCoord = QVector2D(0.0, 1.0);
        
        // Bottom right
        vertices[1].position = QVector3D(x2, 0.0, z2);
        vertices[1].normal = normal;
        vertices[1].texCoord = QVector2D(1.0, 1.0);
        
        // Top right
        vertices[2].position = QVector3D(x2, wallHeight, z2);
        vertices[2].normal = normal;
        vertices[2].texCoord = QVector2D(1.0, 0.0);
        
        // Top left
        vertices[3].position = QVector3D(x1, wallHeight, z1);
        vertices[3].normal = normal;
        vertices[3].texCoord = QVector2D(0.0, 0.0);
        
        // Indices for two triangles
        GLuint indices[6] = { 0, 1, 2, 2, 3, 0 };
        
        // Create OpenGL buffers - using the unique_ptr in WallGeometry
        WallGeometry& wall = m_walls[i];
        
        wall.vao->create();
        wall.vao->bind();
        
        wall.vbo->create();
        wall.vbo->bind();
        wall.vbo->allocate(vertices, 4 * sizeof(Vertex));
        
        // Enable vertex attributes
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        
        wall.ibo->create();
        wall.ibo->bind();
        wall.ibo->allocate(indices, 6 * sizeof(GLuint));
        
        wall.indexCount = 6;
        
        wall.vao->release();
        
        // Create wall entity in game scene
        GameEntity wallEntity;
        wallEntity.id = QString("arena_wall_%1").arg(i);
        wallEntity.type = "arena_wall";
        wallEntity.position = QVector3D((x1 + x2) / 2, wallHeight / 2, (z1 + z2) / 2);
        wallEntity.dimensions = QVector3D(wallLength, wallHeight, 0.2);
        wallEntity.isStatic = true;
        
        m_gameScene->addEntity(wallEntity);
    }
}

void GLArenaWidget::createFloor(double radius)
{
    // Create a circular floor with appropriate number of segments
    const int segments = 32;
    QVector<Vertex> vertices;
    QVector<GLuint> indices;
    
    // Center vertex
    Vertex center;
    center.position = QVector3D(0.0f, -0.05f, 0.0f); // Slightly below 0 to avoid z-fighting
    center.normal = QVector3D(0.0f, 1.0f, 0.0f);
    center.texCoord = QVector2D(0.5f, 0.5f);
    vertices.append(center);
    
    // Outer rim vertices
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = radius * cos(angle);
        float z = radius * sin(angle);
        
        Vertex v;
        v.position = QVector3D(x, -0.05f, z);
        v.normal = QVector3D(0.0f, 1.0f, 0.0f);
        
        // Texture coordinates - map [0, 1] x [0, 1] to floor
        v.texCoord = QVector2D(0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle));
        
        vertices.append(v);
    }
    
    // Create indices for triangle fan
    for (int i = 1; i <= segments; i++) {
        indices.append(0);  // Center
        indices.append(i);
        indices.append(i + 1 > segments ? 1 : i + 1);
    }
    
    // Create OpenGL buffers
    m_floorVAO.create();
    m_floorVAO.bind();
    
    m_floorVBO.create();
    m_floorVBO.bind();
    m_floorVBO.allocate(vertices.constData(), vertices.size() * sizeof(Vertex));
    
    // Enable vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    
    m_floorIBO.create();
    m_floorIBO.bind();
    m_floorIBO.allocate(indices.constData(), indices.size() * sizeof(GLuint));
    
    m_floorIndexCount = indices.size();
    
    m_floorVAO.release();
    
    // Create floor entity in game scene
    GameEntity floorEntity;
    floorEntity.id = "arena_floor";
    floorEntity.type = "arena_floor";
    floorEntity.position = QVector3D(0.0f, -0.05f, 0.0f);
    floorEntity.dimensions = QVector3D(radius * 2, 0.1f, radius * 2);
    floorEntity.isStatic = true;
    
    m_gameScene->addEntity(floorEntity);
}

void GLArenaWidget::createGrid(double size, int divisions)
{
    // Create a grid of lines on the floor for better orientation
    QVector<QVector3D> lineVertices;
    
    float step = size / divisions;
    float halfSize = size / 2.0f;
    
    // Create grid lines along x-axis
    for (int i = 0; i <= divisions; i++) {
        float x = -halfSize + i * step;
        lineVertices.append(QVector3D(x, -0.04f, -halfSize)); // Start point
        lineVertices.append(QVector3D(x, -0.04f, halfSize));  // End point
    }
    
    // Create grid lines along z-axis
    for (int i = 0; i <= divisions; i++) {
        float z = -halfSize + i * step;
        lineVertices.append(QVector3D(-halfSize, -0.04f, z)); // Start point
        lineVertices.append(QVector3D(halfSize, -0.04f, z));  // End point
    }
    
    // Create OpenGL buffers
    m_gridVAO.create();
    m_gridVAO.bind();
    
    m_gridVBO.create();
    m_gridVBO.bind();
    m_gridVBO.allocate(lineVertices.constData(), lineVertices.size() * sizeof(QVector3D));
    
    // Enable vertex attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), nullptr);
    
    m_gridVertexCount = lineVertices.size();
    
    m_gridVAO.release();
}

void GLArenaWidget::renderArena()
{
    if (!m_basicProgram || !m_gridProgram || !m_initialized) return;
    
    // Render floor
    m_basicProgram->bind();
    
    QMatrix4x4 modelMatrix;
    modelMatrix.setToIdentity();
    
    m_basicProgram->setUniformValue("model", modelMatrix);
    m_basicProgram->setUniformValue("view", m_viewMatrix);
    m_basicProgram->setUniformValue("projection", m_projectionMatrix);
    m_basicProgram->setUniformValue("objectColor", QVector3D(0.5f, 0.5f, 0.5f)); // Gray floor
    
    // Set lighting parameters
    QVector3D playerPos = m_playerController->getPosition();
    QVector3D eyePos = playerPos + QVector3D(0.0f, 1.6f, 0.0f);
    m_basicProgram->setUniformValue("lightPos", QVector3D(0.0f, 10.0f, 0.0f)); // Light above the arena
    m_basicProgram->setUniformValue("viewPos", eyePos);
    
    m_floorVAO.bind();
    glDrawElements(GL_TRIANGLES, m_floorIndexCount, GL_UNSIGNED_INT, nullptr);
    m_floorVAO.release();
    
    // Render walls
    for (int i = 0; i < m_walls.size(); i++) {
        const WallGeometry& wall = m_walls[i];
        
        // Set wall color
        m_basicProgram->setUniformValue("objectColor", QVector3D(0.7f, 0.7f, 0.7f)); // Light gray walls
        
        wall.vao->bind();
        glDrawElements(GL_TRIANGLES, wall.indexCount, GL_UNSIGNED_INT, nullptr);
        wall.vao->release();
    }
    
    m_basicProgram->release();
    
    // Render grid
    m_gridProgram->bind();
    
    m_gridProgram->setUniformValue("model", modelMatrix);
    m_gridProgram->setUniformValue("view", m_viewMatrix);
    m_gridProgram->setUniformValue("projection", m_projectionMatrix);
    m_gridProgram->setUniformValue("lineColor", QVector3D(0.3f, 0.3f, 0.3f)); // Dark gray grid
    
    m_gridVAO.bind();
    glDrawArrays(GL_LINES, 0, m_gridVertexCount);
    m_gridVAO.release();
    
    m_gridProgram->release();
}

void GLArenaWidget::renderCharacters()
{
    if (!m_billboardProgram || !m_initialized) {
        return;
    }
    
    // Safety check for OpenGL context
    if (!context() || !context()->isValid()) {
        qWarning() << "Invalid OpenGL context in renderCharacters";
        return;
    }
    
    try {
        // First, make sure the billboard program is compiled and linked
        if (!m_billboardProgram->isLinked()) {
            qWarning() << "Billboard program is not linked, cannot render characters";
            return;
        }
        
        // Bind the program once for all sprites
        if (!m_billboardProgram->bind()) {
            qWarning() << "Failed to bind billboard program";
            return;
        }
        
        // Set up camera right and up vectors for billboarding
        QMatrix4x4 view = m_viewMatrix;
        QVector3D right(view(0, 0), view(1, 0), view(2, 0));
        QVector3D up(view(0, 1), view(1, 1), view(2, 1));
        
        // Set common shader uniforms
        if (m_billboardProgram->uniformLocation("view") != -1)
            m_billboardProgram->setUniformValue("view", m_viewMatrix);
        
        if (m_billboardProgram->uniformLocation("projection") != -1)
            m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
        
        if (m_billboardProgram->uniformLocation("cameraRight") != -1)
            m_billboardProgram->setUniformValue("cameraRight", right);
        
        if (m_billboardProgram->uniformLocation("cameraUp") != -1)
            m_billboardProgram->setUniformValue("cameraUp", up);
        
        // Render each character sprite one by one
        for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
            CharacterSprite* sprite = it.value();
            if (!sprite) continue;
            
            try {
                // Set per-sprite uniforms
                if (m_billboardProgram->uniformLocation("billboardPos") != -1)
                    m_billboardProgram->setUniformValue("billboardPos", QVector3D(0.0f, 1.0f, 0.0f)); // Fixed position for now
                
                if (m_billboardProgram->uniformLocation("billboardSize") != -1)
                    m_billboardProgram->setUniformValue("billboardSize", 
                                                      QVector2D(sprite->width(), sprite->height()));
                
                // Enable depth test but disable depth write for transparent billboards
                glEnable(GL_DEPTH_TEST);
                glDepthMask(GL_TRUE);
                
                // Get texture and VAO from sprite
                QOpenGLTexture* texture = sprite->getTexture();
                QOpenGLVertexArrayObject* vao = sprite->getVAO();
                
                if (texture && texture->isCreated() && vao && vao->isCreated()) {
                    // Bind texture
                    texture->bind(0);
                    if (m_billboardProgram->uniformLocation("textureSampler") != -1) {
                        m_billboardProgram->setUniformValue("textureSampler", 0);
                    }
                    
                    // Bind VAO and draw
                    vao->bind();
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
                    vao->release();
                    
                    // Release texture
                    texture->release();
                }
            }
            catch (const std::exception& e) {
                qWarning() << "Exception rendering character" << it.key() << ":" << e.what();
                // Continue with next sprite
            }
        }
        
        // Unbind the program when done with all sprites
        m_billboardProgram->release();
    } 
    catch (const std::exception& e) {
        qWarning() << "Exception in renderCharacters:" << e.what();
    }
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

void GLArenaWidget::loadCharacterSprite(const QString& characterName, const QString& texturePath)
{
    if (!m_initialized) {
        qWarning() << "Cannot load character sprite: renderer not initialized";
        return;
    }
    
    qDebug() << "Loading character sprite:" << characterName << "path:" << texturePath;
    
    // Get character dimensions - with safety checks
    CharacterCollisionGeometry geometry;
    geometry.width = 1.0;  // Default values
    geometry.height = 2.0;
    geometry.depth = 1.0;
    
    try {
        if (m_characterManager) {
            CharacterAppearance appearance = m_characterManager->loadCharacterAppearance(characterName);
            geometry = appearance.collision;
        } else {
            qWarning() << "Character manager is null, using default geometry";
        }
    } catch (const std::exception& e) {
        qWarning() << "Error loading character appearance:" << e.what();
        // Continue with default geometry
    }
    
    // Create or update character sprite with robust error handling
    try {
        // Make sure current context is the GLWidget's context
        makeCurrent();
        
        // Check if the context is valid
        if (!context() || !context()->isValid()) {
            qWarning() << "Invalid OpenGL context when trying to load sprite";
            return;
        }
        
        // Remove any existing sprite for this character
        if (m_characterSprites.contains(characterName)) {
            try {
                delete m_characterSprites[characterName];
                m_characterSprites.remove(characterName);
            } catch (const std::exception& e) {
                qWarning() << "Exception removing old sprite:" << e.what();
                // Continue with creating new sprite
            }
        }
        
        // Check if texture file exists to avoid trying to load non-existent files
        QString finalTexturePath = texturePath;
        if (!texturePath.isEmpty()) {
            QFileInfo fileInfo(texturePath);
            if (!fileInfo.exists() || !fileInfo.isFile()) {
                qWarning() << "Texture file does not exist:" << texturePath;
                finalTexturePath = ""; // This will cause the default texture to be used
            }
        }
        
        // Create new sprite with a valid context
        CharacterSprite* sprite = new CharacterSprite();
        
        // Initialize the sprite
        sprite->init(context(), finalTexturePath, geometry.width, geometry.height, geometry.depth);
        m_characterSprites[characterName] = sprite;
        
        // Position at origin initially
        QVector3D initialPos(0.0f, 0.0f, 0.0f);
        sprite->updatePosition(initialPos.x(), initialPos.y(), initialPos.z());
        
        // Make sure we release the context before emitting signals or updating
        doneCurrent();
        
        // Emit position update signal (must be done after doneCurrent)
        emit characterPositionUpdated(characterName, initialPos.x(), initialPos.y(), initialPos.z());
        
        // Request a repaint
        update();
    } catch (const std::exception& e) {
        qWarning() << "Exception during character sprite loading:" << e.what();
        
        // Ensure we release the context
        if (context() && context()->isValid()) {
            doneCurrent();
        }
    }
}

void GLArenaWidget::updateCharacterPosition(const QString& characterName, float x, float y, float z)
{
    if (!m_characterSprites.contains(characterName)) return;
    
    CharacterSprite* sprite = m_characterSprites[characterName];
    if (sprite) {
        sprite->updatePosition(x, y, z);
        emit characterPositionUpdated(characterName, x, y, z);
        update();
    }
}

void GLArenaWidget::keyPressEvent(QKeyEvent* event)
{
    // Safety check before handling key events
    if (!m_initialized || !m_playerController || !event) {
        QOpenGLWidget::keyPressEvent(event);
        return;
    }

    try {
        qDebug() << "GLArenaWidget handling key press:" << event->key();
        m_playerController->handleKeyPress(event);
    } 
    catch (const std::exception& e) {
        qWarning() << "Exception in GLArenaWidget::keyPressEvent:" << e.what();
    }
    
    // Let parent widget handle the event too
    QOpenGLWidget::keyPressEvent(event);
}

void GLArenaWidget::keyReleaseEvent(QKeyEvent* event)
{
    // Safety check before handling key events
    if (!m_initialized || !m_playerController || !event) {
        QOpenGLWidget::keyReleaseEvent(event);
        return;
    }

    try {
        qDebug() << "GLArenaWidget handling key release:" << event->key();
        m_playerController->handleKeyRelease(event);
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in GLArenaWidget::keyReleaseEvent:" << e.what();
    }
    
    // Let parent widget handle the event too
    QOpenGLWidget::keyReleaseEvent(event);
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