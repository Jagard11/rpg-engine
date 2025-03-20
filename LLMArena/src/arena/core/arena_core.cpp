// src/arena/core/arena_core.cpp
#include "../include/game/game_scene.h"
#include <QDebug>
#include <QDateTime>
#include <cmath>

GameScene::GameScene(QObject *parent) 
    : QObject(parent), arenaRadius(10.0), arenaWallHeight(2.0) {
}

void GameScene::addEntity(const GameEntity &entity) {
    // Check if entity already exists and remove it first
    if (entities.contains(entity.id)) {
        removeEntity(entity.id);
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
    
    // Get entity half-dimensions for collision checks
    float halfWidth = entity.dimensions.x() / 2.0f;
    float halfHeight = entity.dimensions.y() / 2.0f;
    float halfDepth = entity.dimensions.z() / 2.0f;
    
    // Check world boundaries if they are enabled
    if (worldBoundariesEnabled) {
        // Simple rectangular boundary check
        if (newPosition.x() - halfWidth < -arenaRadius || 
            newPosition.x() + halfWidth > arenaRadius ||
            newPosition.z() - halfDepth < -arenaRadius || 
            newPosition.z() + halfDepth > arenaRadius) {
            
            return true; // Collision with world boundary
        }
    }
    
    // For voxel-based worlds, we now check all solid entities
    int collisionCount = 0;
    
    // Check collisions with other entities
    for (auto it = entities.constBegin(); it != entities.constEnd(); ++it) {
        // Skip self and non-solid objects
        if (it.key() == entityId || !isCollidable(it.value().type)) {
            continue;
        }
        
        // Skip static-static collisions
        if (entity.isStatic && it.value().isStatic) {
            continue;
        }
        
        // Create a temporary entity with the new position for collision testing
        GameEntity tempEntity = entity;
        tempEntity.position = newPosition;
        
        if (areEntitiesColliding(tempEntity, it.value())) {
            emit collisionDetected(entityId, it.key());
            collisionCount++;
            
            // Only log first few collisions to avoid spamming
            if (collisionCount <= 3 && entityId == "player") {
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
    worldBoundariesEnabled = true;
    
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

void GameScene::setWorldBoundaries(bool enabled) {
    worldBoundariesEnabled = enabled;
}

bool GameScene::isInsideArena(const QVector3D &position) const {
    // For a rectangular arena, just check if the position is within the bounds
    return (position.x() >= -arenaRadius && position.x() <= arenaRadius &&
            position.z() >= -arenaRadius && position.z() <= arenaRadius);
}

bool GameScene::isCollidable(const QString &entityType) const {
    // List of entity types that can be collided with
    static const QSet<QString> collidableTypes = {
        "voxel", "arena_wall", "character", "object", "block", "solid"
    };
    
    return collidableTypes.contains(entityType);
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