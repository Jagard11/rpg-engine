// src/game/game_scene.cpp
#include "../include/game/game_scene.h"
#include <QDebug>
#include <cmath>

GameScene::GameScene(QObject *parent) 
    : QObject(parent), arenaRadius(10.0), arenaWallHeight(2.0) {
}

void GameScene::addEntity(const GameEntity &entity) {
    // Check if entity already exists and remove it first
    if (entities.contains(entity.id)) {
        removeEntity(entity.id);
    }
    
    // Skip logging voxels and celestial objects
    if (!entity.id.startsWith("voxel_") && 
        entity.id != "sun" && 
        entity.id != "moon") {
        qDebug() << "Added entity:" << entity.id;
    }
    
    entities[entity.id] = entity;
    emit entityAdded(entity);
}

void GameScene::removeEntity(const QString &id) {
    if (entities.contains(id)) {
        entities.remove(id);
        emit entityRemoved(id);
    }
}

void GameScene::updateEntityPosition(const QString &id, const QVector3D &position) {
    if (entities.contains(id)) {
        GameEntity &entity = entities[id];
        entity.position = position;
        emit entityPositionUpdated(id, position);
    }
}

GameEntity GameScene::getEntity(const QString &id) const {
    if (entities.contains(id)) {
        return entities[id];
    }
    return GameEntity(); // Return empty entity if not found
}

QVector<GameEntity> GameScene::getEntitiesByType(const QString &type) const {
    QVector<GameEntity> result;
    
    for (auto it = entities.constBegin(); it != entities.constEnd(); ++it) {
        if (it.value().type == type) {
            result.append(it.value());
        }
    }
    
    return result;
}

QVector<GameEntity> GameScene::getAllEntities() const {
    QVector<GameEntity> result;
    
    for (auto it = entities.constBegin(); it != entities.constEnd(); ++it) {
        result.append(it.value());
    }
    
    return result;
}

bool GameScene::checkCollision(const QString &entityId, const QVector3D &newPosition) {
    // Added safety to prevent excessive checks
    static qint64 lastCollisionCheck = 0;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    if (currentTime - lastCollisionCheck < 10) { // Avoid too many checks in quick succession
        return false; // Assume no collision to avoid processing overhead
    }
    lastCollisionCheck = currentTime;

    // First check that our entity exists
    if (!entities.contains(entityId)) {
        return false;
    }
    
    GameEntity &entity = entities[entityId];
    
    // SEGFAULT-FIX: First check if new position is inside the arena
    if (!isInsideArena(newPosition)) {
        return true; // Collision with arena boundary
    }
    
    // Start with checking only a limited number of entities
    // This prevents excessive checking that might overload OpenGL
    const int MAX_CHECKS = 20;
    int checkCount = 0;
    
    // Check collisions with other entities
    for (auto it = entities.constBegin(); it != entities.constEnd(); ++it) {
        // Skip self, floor entity, non-collidable entities, and static-static collisions
        if (it.key() == entityId || it.key() == "arena_floor" || 
            (entity.isStatic && it.value().isStatic)) {
            continue;
        }
        
        // SEGFAULT-FIX: Skip voxel and celestial entities after a certain count
        // to prevent excessive checking
        if ((it.key().startsWith("voxel_") || it.key() == "sun" || it.key() == "moon") && 
            checkCount > MAX_CHECKS) {
            continue;
        }
        
        checkCount++;
        
        // Create a temporary entity with the new position
        GameEntity tempEntity = entity;
        tempEntity.position = newPosition;
        
        if (areEntitiesColliding(tempEntity, it.value())) {
            // Emit collision event
            qDebug() << "SEGFAULT-FIX: Collision detected between" << entityId << "and" << it.key();
            emit collisionDetected(entityId, it.key());
            return true;
        }
    }
    
    return false;
}

void GameScene::createOctagonalArena(double radius, double wallHeight) {
    // Store the parameters
    arenaRadius = radius;
    arenaWallHeight = wallHeight;
    
    // Remove any existing arena entities before recreating
    QStringList arenaEntities;
    for (auto it = entities.constBegin(); it != entities.constEnd(); ++it) {
        if (it.value().type == "arena_wall" || it.value().type == "arena_floor") {
            arenaEntities.append(it.key());
        }
    }
    
    for (const QString &id : arenaEntities) {
        removeEntity(id);
    }
    
    // Create arena floor
    GameEntity floor;
    floor.id = "arena_floor";
    floor.type = "arena_floor";
    floor.position = QVector3D(0, -0.05, 0); // Position slightly below 0 to avoid player collisions
    floor.dimensions = QVector3D(radius * 2, 0.1, radius * 2);
    floor.isStatic = true;
    addEntity(floor);
    
    // Create eight wall segments for the octagon
    for (int i = 0; i < 8; i++) {
        const double angle1 = M_PI * 2 * i / 8;
        const double angle2 = M_PI * 2 * (i + 1) / 8;
        
        const double x1 = radius * cos(angle1);
        const double z1 = radius * sin(angle1);
        const double x2 = radius * cos(angle2);
        const double z2 = radius * sin(angle2);
        
        // Create wall entity
        GameEntity wall;
        wall.id = QString("arena_wall_%1").arg(i);
        wall.type = "arena_wall";
        
        // Position at midpoint of the wall segment
        double midX = (x1 + x2) / 2;
        double midZ = (z1 + z2) / 2;
        wall.position = QVector3D(midX, wallHeight / 2, midZ);
        
        // Calculate wall dimensions
        double wallLength = sqrt(pow(x2 - x1, 2) + pow(z2 - z1, 2));
        wall.dimensions = QVector3D(wallLength, wallHeight, 0.2); // Slightly thicker walls for better collisions
        wall.isStatic = true;
        
        addEntity(wall);
    }
}

bool GameScene::isInsideArena(const QVector3D &position) const {
    // Check if distance from center is less than arena radius
    double distanceFromCenter = sqrt(position.x() * position.x() + position.z() * position.z());
    
    // Use a small buffer (0.5m) to prevent getting too close to walls
    return distanceFromCenter < (arenaRadius - 0.5);
}

bool GameScene::areEntitiesColliding(const GameEntity &entityA, const GameEntity &entityB) const {
    // Skip static-static collisions (walls colliding with walls)
    if (entityA.isStatic && entityB.isStatic) {
        return false;
    }
    
    // SEGFAULT-FIX: Extra safety checks for null dimensions
    float aWidth = entityA.dimensions.x() > 0 ? entityA.dimensions.x() : 1.0f;
    float aHeight = entityA.dimensions.y() > 0 ? entityA.dimensions.y() : 1.0f;
    float aDepth = entityA.dimensions.z() > 0 ? entityA.dimensions.z() : 1.0f;
    
    float bWidth = entityB.dimensions.x() > 0 ? entityB.dimensions.x() : 1.0f;
    float bHeight = entityB.dimensions.y() > 0 ? entityB.dimensions.y() : 1.0f;
    float bDepth = entityB.dimensions.z() > 0 ? entityB.dimensions.z() : 1.0f;
    
    // Simple AABB collision check with relaxed tolerances
    bool xOverlap = fabs(entityA.position.x() - entityB.position.x()) < 
        (aWidth / 2 + bWidth / 2) * 0.9;
        
    bool yOverlap = fabs(entityA.position.y() - entityB.position.y()) < 
        (aHeight / 2 + bHeight / 2) * 0.9;
        
    bool zOverlap = fabs(entityA.position.z() - entityB.position.z()) < 
        (aDepth / 2 + bDepth / 2) * 0.9;
    
    return xOverlap && yOverlap && zOverlap;
}