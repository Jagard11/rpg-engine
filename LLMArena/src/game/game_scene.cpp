// src/game/game_scene.cpp
#include "../include/game/game_scene.h"
#include <QDebug>  // Add this for qDebug
#include <QDateTime>  // Add this for QDateTime
#include <cmath>

GameScene::GameScene(QObject *parent) 
    : QObject(parent), arenaRadius(10.0), arenaWallHeight(2.0) {
}

void GameScene::addEntity(const GameEntity &entity) {
    // Check if entity already exists and remove it first
    if (entities.contains(entity.id)) {
        removeEntity(entity.id);
    }
    
    // Skip all debug logging
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
    // Silently update position without any logging whatsoever
    if (entities.contains(id)) {
        entities[id].position = position;
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
    // First check that our entity exists
    if (!entities.contains(entityId)) {
        return false;
    }
    
    GameEntity &entity = entities[entityId];
    
    // Check if any part of the entity would be outside the arena bounds
    float halfWidth = entity.dimensions.x() / 2.0f;
    float halfDepth = entity.dimensions.z() / 2.0f;
    
    // For a rectangular arena, check simple boundaries
    if (newPosition.x() - halfWidth < -arenaRadius || 
        newPosition.x() + halfWidth > arenaRadius ||
        newPosition.z() - halfDepth < -arenaRadius || 
        newPosition.z() + halfDepth > arenaRadius) {
        qDebug() << "Arena boundary collision at" << newPosition.x() << newPosition.y() << newPosition.z();
        return true; // Collision with arena boundary
    }
    
    // For debugging - count collisions
    int collisionCount = 0;
    
    // Check collisions with other entities
    for (auto it = entities.constBegin(); it != entities.constEnd(); ++it) {
        // Skip self and floor
        if (it.key() == entityId || it.key() == "arena_floor") {
            continue;
        }
        
        // Skip static-static collisions
        if (entity.isStatic && it.value().isStatic) {
            continue;
        }
        
        // Skip player-player collision
        if (entityId == "player" && it.value().type == "player") {
            continue;
        }
        
        // Skip non-solid entities and celestial objects
        if (it.value().type != "voxel" && it.value().type != "character" && 
            it.value().type != "arena_wall" && it.value().type != "object") {
            continue;
        }
        
        // Skip sun and moon
        if (it.key() == "sun" || it.key() == "moon") {
            continue;
        }
        
        // Create a temporary entity with the new position
        GameEntity tempEntity = entity;
        tempEntity.position = newPosition;
        
        if (areEntitiesColliding(tempEntity, it.value())) {
            emit collisionDetected(entityId, it.key());
            collisionCount++;
            
            // Only log first few collisions to avoid spamming
            if (collisionCount <= 3) {
                qDebug() << "Collision between" << entityId << "and" << it.key() << "at" 
                         << it.value().position.x() << it.value().position.y() << it.value().position.z();
            }
        }
    }
    
    return collisionCount > 0;
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
    
    // Create rectangular arena walls (4 sides)
    // North wall (positive Z)
    GameEntity northWall;
    northWall.id = "arena_wall_north";
    northWall.type = "arena_wall";
    northWall.position = QVector3D(0, wallHeight / 2, radius);
    northWall.dimensions = QVector3D(radius * 2, wallHeight, 0.2);
    northWall.isStatic = true;
    addEntity(northWall);
    
    // South wall (negative Z)
    GameEntity southWall;
    southWall.id = "arena_wall_south";
    southWall.type = "arena_wall";
    southWall.position = QVector3D(0, wallHeight / 2, -radius);
    southWall.dimensions = QVector3D(radius * 2, wallHeight, 0.2);
    southWall.isStatic = true;
    addEntity(southWall);
    
    // East wall (positive X)
    GameEntity eastWall;
    eastWall.id = "arena_wall_east";
    eastWall.type = "arena_wall";
    eastWall.position = QVector3D(radius, wallHeight / 2, 0);
    eastWall.dimensions = QVector3D(0.2, wallHeight, radius * 2);
    eastWall.isStatic = true;
    addEntity(eastWall);
    
    // West wall (negative X)
    GameEntity westWall;
    westWall.id = "arena_wall_west";
    westWall.type = "arena_wall";
    westWall.position = QVector3D(-radius, wallHeight / 2, 0);
    westWall.dimensions = QVector3D(0.2, wallHeight, radius * 2);
    westWall.isStatic = true;
    addEntity(westWall);
}

bool GameScene::isInsideArena(const QVector3D &position) const {
    // For a rectangular arena, just check if the position is within the bounds
    return (position.x() >= -arenaRadius && position.x() <= arenaRadius &&
            position.z() >= -arenaRadius && position.z() <= arenaRadius);
}

bool GameScene::areEntitiesColliding(const GameEntity &entityA, const GameEntity &entityB) const {
    // Skip static-static collisions (walls colliding with walls)
    if (entityA.isStatic && entityB.isStatic) {
        return false;
    }
    
    // Extra safety checks for null dimensions
    float aWidth = entityA.dimensions.x() > 0 ? entityA.dimensions.x() : 1.0f;
    float aHeight = entityA.dimensions.y() > 0 ? entityA.dimensions.y() : 1.0f;
    float aDepth = entityA.dimensions.z() > 0 ? entityA.dimensions.z() : 1.0f;
    
    float bWidth = entityB.dimensions.x() > 0 ? entityB.dimensions.x() : 1.0f;
    float bHeight = entityB.dimensions.y() > 0 ? entityB.dimensions.y() : 1.0f;
    float bDepth = entityB.dimensions.z() > 0 ? entityB.dimensions.z() : 1.0f;
    
    // Calculate half sizes for collision check
    float aHalfWidth = aWidth / 2.0f;
    float aHalfHeight = aHeight / 2.0f;
    float aHalfDepth = aDepth / 2.0f;
    
    float bHalfWidth = bWidth / 2.0f;
    float bHalfHeight = bHeight / 2.0f;
    float bHalfDepth = bDepth / 2.0f;
    
    // Calculate distance between centers
    float dx = qAbs(entityA.position.x() - entityB.position.x());
    float dy = qAbs(entityA.position.y() - entityB.position.y());
    float dz = qAbs(entityA.position.z() - entityB.position.z());
    
    // For voxels, make the collision box slightly smaller to allow movement between voxels
    if (entityB.type == "voxel") {
        bHalfWidth *= 0.9f;
        bHalfDepth *= 0.9f;
    }
    
    // Check if entities overlap on all three axes
    bool xOverlap = dx < (aHalfWidth + bHalfWidth);
    bool yOverlap = dy < (aHalfHeight + bHalfHeight);
    bool zOverlap = dz < (aHalfDepth + bHalfDepth);
    
    return xOverlap && yOverlap && zOverlap;
}