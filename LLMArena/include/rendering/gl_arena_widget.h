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
#include <memory>

#include "../character/character_persistence.h"
#include "../game/game_scene.h"
#include "../game/player_controller.h"
#include "../game/inventory.h"
#include "../voxel/voxel_system_integration.h"
#include "../ui/inventory_ui.h"

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
    
    // Safe getters with null checks
    bool hasValidTexture() const { return m_texture && m_texture->isCreated(); }
    bool hasValidVAO() const { return m_vao.isCreated(); }
    QOpenGLTexture* getTexture() const { return m_texture; }
    QOpenGLVertexArrayObject* getVAO() { return &m_vao; }
    
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
    
    // Helper to manage cursor and mouse tracking state
    void updateMouseTrackingState();

    // Initialize with arena parameters
    void initializeArena(double width, double height);
    
    // Character management
    void setActiveCharacter(const QString& name);
    void loadCharacterSprite(const QString& characterName, const QString& texturePath);
    void updateCharacterPosition(const QString& characterName, float x, float y, float z);
    
    // Get player controller
    PlayerController* getPlayerController() const { return m_playerController; }
    
    // Handle key events
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    
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
    void onPlayerPitchChanged(float pitch);

private:
    // Initialize shaders
    bool initShaders();
    
    // Render characters
    void renderCharacters();
    
    // Simpler character rendering method
    void renderCharactersSimple();
    
    // Absolute fallback rendering method
    void renderCharactersFallback();
    
    // Direct quad drawing without VAOs
    void drawCharacterQuad(QOpenGLTexture* texture, float x, float y, float z, float width, float height);
    
    // Convert world coords to normalized device coords
    QVector3D worldToNDC(const QVector3D& worldPos);
    
    // Voxel placement and highlighting
    void raycastVoxels(const QVector3D& origin, const QVector3D& direction);
    void renderVoxelHighlight();
    void placeVoxel();
    void removeVoxel();
    
    // Inventory system
    void initializeInventory();
    void renderInventory();
    void onInventoryVisibilityChanged(bool visible);
    
    // Create geometric objects for rendering
    void createFloor(double radius);
    void createArena(double radius, double wallHeight);
    void createGrid(double size, int divisions);
    
    // Character manager for loading sprites
    CharacterManager* m_characterManager;
    
    // Game scene for entity tracking
    GameScene* m_gameScene;
    
    // Player controller
    PlayerController* m_playerController;
    
    // Active character
    QString m_activeCharacter;
    
    // Voxel system integration
    VoxelSystemIntegration* m_voxelSystem;
    
    // Inventory system
    Inventory* m_inventory = nullptr;
    InventoryUI* m_inventoryUI = nullptr;
    
    // Voxel highlighting
    QVector3D m_highlightedVoxelPos;
    int m_highlightedVoxelFace = -1;
    float m_maxPlacementDistance = 5.0f;
    
    // Shader program for billboards
    QOpenGLShaderProgram* m_billboardProgram;
    
    // Flag to indicate if OpenGL is properly initialized
    bool m_initialized;
    
    // Geometry-related members
    QOpenGLVertexArrayObject m_floorVAO;
    QOpenGLBuffer m_floorVBO;
    QOpenGLBuffer m_floorIBO;
    int m_floorIndexCount;
    
    QOpenGLVertexArrayObject m_gridVAO;
    QOpenGLBuffer m_gridVBO;
    int m_gridVertexCount;
    
    // Assets
    QMap<QString, CharacterSprite*> m_characterSprites;
    
    // OpenGL matrices
    QMatrix4x4 m_projectionMatrix;
    QMatrix4x4 m_viewMatrix;
    
    // Arena parameters
    double m_arenaRadius;
    double m_wallHeight;
    
    // Wall geometry
    struct WallGeometry {
        std::unique_ptr<QOpenGLVertexArrayObject> vao;
        std::unique_ptr<QOpenGLBuffer> vbo;
        std::unique_ptr<QOpenGLBuffer> ibo;
        int indexCount;
    };
    
    std::vector<WallGeometry> m_walls;
};

#endif // GL_ARENA_WIDGET_H