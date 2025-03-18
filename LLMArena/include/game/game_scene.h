// include/game/game_scene.h
#ifndef GAME_SCENE_H
#define GAME_SCENE_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QVector3D>

// Structure to represent a game entity with position and collision data
struct GameEntity {
    QString id;
    QString type;        // "player", "npc", "wall", etc.
    QVector3D position;  
    QVector3D dimensions; // width, height, depth
    QString spritePath;   // Path to sprite image (for billboard entities)
    bool isStatic;        // Static entities don't move (walls, etc.)
    
    GameEntity() : isStatic(false) {}
};

// Class to manage the game scene and entities
class GameScene : public QObject {
    Q_OBJECT

public:
    GameScene(QObject *parent = nullptr);
    
    // Add an entity to the scene
    Q_INVOKABLE void addEntity(const GameEntity &entity);
    
    // Remove an entity from the scene
    Q_INVOKABLE void removeEntity(const QString &id);
    
    // Update entity position
    Q_INVOKABLE void updateEntityPosition(const QString &id, const QVector3D &position);
    
    // Get entity by ID
    Q_INVOKABLE GameEntity getEntity(const QString &id) const;
    
    // Get all entities of a specific type
    Q_INVOKABLE QVector<GameEntity> getEntitiesByType(const QString &type) const;
    
    // Get all entities in the scene
    QVector<GameEntity> getAllEntities() const;
    
    // Check for collisions and handle them
    Q_INVOKABLE bool checkCollision(const QString &entityId, const QVector3D &newPosition);
    
    // Create octagonal arena with walls
    void createOctagonalArena(double radius, double wallHeight);
    
    // Check if position is inside the arena
    bool isInsideArena(const QVector3D &position) const;

signals:
    // Signal for when an entity is added
    void entityAdded(const GameEntity &entity);
    
    // Signal for when an entity is removed
    void entityRemoved(const QString &id);
    
    // Signal for when an entity position is updated
    void entityPositionUpdated(const QString &id, const QVector3D &position);
    
    // Signal for collision events
    void collisionDetected(const QString &entityA, const QString &entityB);
    
    // Signal for world changes
    void worldChanged();

private:
    QMap<QString, GameEntity> entities;
    double arenaRadius;
    double arenaWallHeight;
    
    // Check if two entities are colliding
    bool areEntitiesColliding(const GameEntity &entityA, const GameEntity &entityB) const;
};

#endif // GAME_SCENE_H