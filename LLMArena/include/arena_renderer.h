// include/arena_renderer.h
#ifndef ARENA_RENDERER_H
#define ARENA_RENDERER_H

#include <QObject>
#include <QString>
#include "character_persistence.h"

// Forward declarations
class GameScene;
class PlayerController;

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
    GameScene* getGameScene() const { return gameScene; }
    
    // Get the player controller
    PlayerController* getPlayerController() const { return playerController; }
    
    // Load a character sprite
    Q_INVOKABLE void loadCharacterSprite(const QString &characterName, const QString &spritePath);
    
    // Update character position
    Q_INVOKABLE void updateCharacterPosition(const QString &characterName, double x, double y, double z);
    
    // Update player position
    Q_INVOKABLE void updatePlayerPosition(double x, double y, double z);
    
    // Set arena parameters
    Q_INVOKABLE void setArenaParameters(double radius, double wallHeight);

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
    // Forward declaration of private implementation
    class Private;
    Private *d;
    
    GameScene *gameScene;
    PlayerController *playerController;
    QString activeCharacter;
    bool initialized;
    CharacterManager *characterManager; // Store CharacterManager reference
    
    // Helper method to inject custom JavaScript
    void injectJavaScript(const QString &script);
    
    // Initialize the WebGL context
    void initializeWebGL();
    
    // Create the octagonal arena
    void createArena(double radius, double wallHeight);
    
    // Create a character billboard
    void createCharacterBillboard(const QString &characterName, const QString &spritePath, 
                                 const CharacterCollisionGeometry &collisionGeometry);
};

#endif // ARENA_RENDERER_H