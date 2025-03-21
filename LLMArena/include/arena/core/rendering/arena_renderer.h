// include/arena/core/rendering/arena_renderer.h
#ifndef ARENA_RENDERER_H
#define ARENA_RENDERER_H

#include <QObject>
#include <QString>
#include <QWebEngineView>
#include <QWebChannel>
#include "character/core/character_persistence.h"

// Forward declarations
class GameScene;
class PlayerController;

// Helper function to check for WebGL support
bool isWebGLSupported();

// Class to handle 3D rendering of the arena using WebGL
class ArenaRenderer : public QObject {
    Q_OBJECT

public:
    ArenaRenderer(QWidget *parent = nullptr, CharacterManager *charManager = nullptr);
    ~ArenaRenderer();
    
    // Initialize the renderer with WebGL
    void initialize();
    
    // Set the active character to display
    Q_INVOKABLE void setActiveCharacter(const QString &name);
    
    // Get the game scene
    GameScene* getGameScene() const;
    
    // Get the player controller
    PlayerController* getPlayerController() const;
    
    // Load a character sprite
    Q_INVOKABLE void loadCharacterSprite(const QString &characterName, const QString &spritePath);
    
    // Update character position
    Q_INVOKABLE void updateCharacterPosition(const QString &characterName, double x, double y, double z);
    
    // Update player position
    Q_INVOKABLE void updatePlayerPosition(double x, double y, double z);
    
    // Set arena parameters
    Q_INVOKABLE void setArenaParameters(double radius, double wallHeight);
    
    // Create the arena
    Q_INVOKABLE void createArena(double radius, double wallHeight);
    
    // Create a character billboard
    Q_INVOKABLE void createCharacterBillboard(const QString &characterName, 
                                            const QString &spritePath, 
                                            const CharacterCollisionGeometry &collisionGeometry);

public slots:
    // Handle JavaScript messages
    void handleJavaScriptMessage(const QString &message);
    
    // Handle page loading finished
    void handleLoadFinished(bool ok);

signals:
    // Signal for when rendering is initialized
    void renderingInitialized();
    
    // Signal for character position update
    void characterPositionUpdated(const QString &characterName, double x, double y, double z);
    
    // Signal for player position update
    void playerPositionUpdated(double x, double y, double z);
    
    // Signal for collision events
    void collisionDetected(const QString &objectA, const QString &objectB);

private:
    GameScene *gameScene;
    PlayerController *playerController;
    QString activeCharacter;
    bool initialized;
    CharacterManager *characterManager;
    QWebEngineView *webView;
    QWebChannel *webChannel;
    
    // Helper method to inject custom JavaScript
    void injectJavaScript(const QString &script);
    
    // Initialize the WebGL context
    void initializeWebGL();
    
    // Create HTML file with WebGL setup
    bool createArenaHtmlFile(const QString &filePath);
    
    // Scene initialization functions
    void appendThreeJsSceneInit();
    void appendCharacterBillboardCode();
    void appendPlayerMovementCode();
};

#endif // ARENA_RENDERER_H