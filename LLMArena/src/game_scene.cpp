// src/game_scene.cpp
#include "../include/game_scene.h"
#include <QDebug>
#include <cmath>

GameScene::GameScene(QObject *parent) 
    : QObject(parent), arenaRadius(10.0), arenaWallHeight(2.0) {
}

void GameScene::addEntity(const GameEntity &entity) {
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

bool GameScene::checkCollision(const QString &entityId, const QVector3D &newPosition) {
    if (!entities.contains(entityId)) {
        return false;
    }
    
    GameEntity &entity = entities[entityId];
    
    // First check if new position is inside the arena
    if (!isInsideArena(newPosition)) {
        return true; // Collision with arena boundary
    }
    
    // Check collisions with other entities
    for (auto it = entities.begin(); it != entities.end(); ++it) {
        // Skip self and non-collidable entities
        if (it.key() == entityId) {
            continue;
        }
        
        // Create a temporary entity with the new position
        GameEntity tempEntity = entity;
        tempEntity.position = newPosition;
        
        if (areEntitiesColliding(tempEntity, it.value())) {
            // Emit collision event
            emit collisionDetected(entityId, it.key());
            return true;
        }
    }
    
    return false;
}

void GameScene::createOctagonalArena(double radius, double wallHeight) {
    arenaRadius = radius;
    arenaWallHeight = wallHeight;
    
    // Remove any existing arena entities
    QVector<QString> arenaEntities;
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
    floor.position = QVector3D(0, 0, 0);
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
        wall.dimensions = QVector3D(wallLength, wallHeight, 0.1);
        wall.isStatic = true;
        
        addEntity(wall);
    }
}

bool GameScene::isInsideArena(const QVector3D &position) const {
    // Simple check: is distance from center less than arena radius
    double distanceFromCenter = sqrt(position.x() * position.x() + position.z() * position.z());
    return distanceFromCenter < (arenaRadius - 0.5); // 0.5 meter buffer
}

bool GameScene::areEntitiesColliding(const GameEntity &entityA, const GameEntity &entityB) const {
    // Skip static-static collisions (walls colliding with walls)
    if (entityA.isStatic && entityB.isStatic) {
        return false;
    }
    
    // Simple AABB collision check
    bool xOverlap = fabs(entityA.position.x() - entityB.position.x()) < 
        (entityA.dimensions.x() / 2 + entityB.dimensions.x() / 2);
        
    bool yOverlap = fabs(entityA.position.y() - entityB.position.y()) < 
        (entityA.dimensions.y() / 2 + entityB.dimensions.y() / 2);
        
    bool zOverlap = fabs(entityA.position.z() - entityB.position.z()) < 
        (entityA.dimensions.z() / 2 + entityB.dimensions.z() / 2);
    
    return xOverlap && yOverlap && zOverlap;
}