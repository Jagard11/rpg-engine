// include/rendering/gl_arena_widget.h
#ifndef GL_ARENA_WIDGET_H
#define GL_ARENA_WIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMatrix4x4>
#include <QVector3D>
#include <QMap>
#include <QTimer>
#include <QKeyEvent>
#include <memory> // For std::unique_ptr

#include "character_persistence.h"
#include "game_scene.h"
#include "player_controller.h"

// Forward declarations
class GLArenaWidget;

// Simple billboard sprite class for characters
class CharacterSprite : protected QOpenGLFunctions {
public:
    CharacterSprite();
    ~CharacterSprite();
    
    void init(QOpenGLContext* context, const QString& texturePath, 
              double width, double height, double depth);
    void render(QOpenGLShaderProgram* program, QMatrix4x4& viewMatrix, QMatrix4x4& projectionMatrix);
    void updatePosition(float x, float y, float z);
    
    float width() const { return m_width; }
    float height() const { return m_height; }
    float depth() const { return m_depth; }
    
    // Accessor methods for rendering
    QOpenGLVertexArrayObject* getVAO() { return &m_vao; }
    QOpenGLTexture* getTexture() { return m_texture; }
    
private:
    QOpenGLTexture* m_texture;
    QVector3D m_position;
    float m_width, m_height, m_depth;
    
    // Rendering data
    QOpenGLBuffer m_vertexBuffer;
    QOpenGLBuffer m_indexBuffer;
    QOpenGLVertexArrayObject m_vao;
};

// Main OpenGL widget for arena rendering
class GLArenaWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit GLArenaWidget(CharacterManager* charManager, QWidget* parent = nullptr);
    ~GLArenaWidget();
    
    // Initialize with arena parameters
    void initializeArena(double radius, double wallHeight);
    
    // Character management
    void setActiveCharacter(const QString& name);
    void loadCharacterSprite(const QString& characterName, const QString& texturePath);
    void updateCharacterPosition(const QString& characterName, float x, float y, float z);
    
    // Get player controller
    PlayerController* getPlayerController() const { return m_playerController; }
    
    // Handle key events
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    
    // Get GL context for helper classes
    QOpenGLContext* getContext() { return context(); }
    
signals:
    void renderingInitialized();
    void characterPositionUpdated(const QString& characterName, double x, double y, double z);
    void playerPositionUpdated(double x, double y, double z);
    
protected:
    // OpenGL overrides
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    
private slots:
    // Update rendering on player movement
    void onPlayerPositionChanged(const QVector3D& position);
    void onPlayerRotationChanged(float rotation);
    
private:
    // Initialize shaders
    bool initShaders();
    
    // Create arena geometry
    void createArena(double radius, double wallHeight);
    
    // Create floor geometry
    void createFloor(double radius);
    
    // Create a grid for visualization
    void createGrid(double size, int divisions);
    
    // Render the arena
    void renderArena();
    
    // Render characters
    void renderCharacters();
    
    // Convert world coords to normalized device coords
    QVector3D worldToNDC(const QVector3D& worldPos);
    
    // Character manager for loading sprites
    CharacterManager* m_characterManager;
    
    // Arena parameters
    double m_arenaRadius;
    double m_wallHeight;
    
    // Game scene for entity tracking
    GameScene* m_gameScene;
    
    // Player controller
    PlayerController* m_playerController;
    
    // Active character
    QString m_activeCharacter;
    
    // Shader programs
    QOpenGLShaderProgram* m_basicProgram;    // For walls and floor
    QOpenGLShaderProgram* m_billboardProgram; // For character billboards
    QOpenGLShaderProgram* m_gridProgram;     // For grid lines
    
    // Camera/view matrices
    QMatrix4x4 m_projectionMatrix;
    QMatrix4x4 m_viewMatrix;
    
    // Wall geometry struct with unique_ptr to prevent copy issues
    struct WallGeometry {
        std::unique_ptr<QOpenGLBuffer> vbo;
        std::unique_ptr<QOpenGLBuffer> ibo;
        std::unique_ptr<QOpenGLVertexArrayObject> vao;
        int indexCount;
        
        WallGeometry() : 
            vbo(new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer)),
            ibo(new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer)),
            vao(new QOpenGLVertexArrayObject()),
            indexCount(0) {}
            
        // No copy allowed, but move is ok
        WallGeometry(const WallGeometry&) = delete;
        WallGeometry& operator=(const WallGeometry&) = delete;
        
        WallGeometry(WallGeometry&& other) noexcept :
            vbo(std::move(other.vbo)),
            ibo(std::move(other.ibo)), 
            vao(std::move(other.vao)),
            indexCount(other.indexCount) {}
            
        WallGeometry& operator=(WallGeometry&& other) noexcept {
            vbo = std::move(other.vbo);
            ibo = std::move(other.ibo);
            vao = std::move(other.vao);
            indexCount = other.indexCount;
            return *this;
        }
    };
    
    // Geometry data
    QOpenGLBuffer m_floorVBO;
    QOpenGLBuffer m_floorIBO;
    QOpenGLVertexArrayObject m_floorVAO;
    int m_floorIndexCount;
    
    QOpenGLBuffer m_gridVBO;
    QOpenGLVertexArrayObject m_gridVAO;
    int m_gridVertexCount;
    
    std::vector<WallGeometry> m_walls; // Changed from QVector to std::vector
    
    // Character sprites
    QMap<QString, CharacterSprite*> m_characterSprites;
    
    // Rendering timer
    QTimer m_renderTimer;
    
    // Flag to indicate if OpenGL is properly initialized
    bool m_initialized;
};

#endif // GL_ARENA_WIDGET_H