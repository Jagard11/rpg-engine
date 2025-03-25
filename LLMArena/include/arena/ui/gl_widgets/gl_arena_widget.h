// include/arena/ui/gl_widgets/gl_arena_widget.h
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
#include <QResizeEvent>
#include <memory>

#include "../../../character/core/character_persistence.h"

#include "../../core/arena_core.h"
#include "../../game/player_controller.h"

#include "../../player/inventory/inventory.h"
#include "../../voxels/voxel_system_integration.h"
#include "../../player/inventory/inventory_ui.h"
#include "../widgets/escape_menu.h"
#include "character_sprite.h"


// Forward declarations for debug system
class DebugSystem;

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
    void initializeArena(double radius, double height);
    
    // Character management
    void setActiveCharacter(const QString& name);
    void loadCharacterSprite(const QString& characterName, const QString& texturePath);
    void updateCharacterPosition(const QString& characterName, float x, float y, float z);
    
    // Get player controller
    PlayerController* getPlayerController() const;
    
    // Handle key events
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    
signals:
    void renderingInitialized();
    void characterPositionUpdated(const QString& characterName, double x, double y, double z);
    void playerPositionUpdated(double x, double y, double z);
    void returnToMainMenu();
    
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
    void onReturnToMainMenu();

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
    void createWallGeometry(const QVector3D& position, const QVector3D& dimensions, const QVector3D& rotation);
    
    // Rendering helper methods
    void renderFloor();
    void renderGrid();
    void renderWalls();
    
    // Debug system methods
    void initializeDebugSystem();
    void renderDebugSystem();
    bool processDebugKeyEvent(QKeyEvent* event);
    void toggleDebugConsole();
    bool isDebugConsoleVisible() const;
    void toggleFrustumVisualization();
    
    // Escape menu methods
    void toggleEscapeMenu();
    
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
    
    // Escape menu
    EscapeMenu* m_escapeMenu = nullptr;
    
    // Debug system
    std::unique_ptr<DebugSystem> m_debugSystem;
    
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
    int m_floorVertexCount;  // Add this new member variable
    
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