// src/game_scene.cpp - Fixed version
#include "../include/game/game_scene.h"
#include <QDebug>
#include <cmath>

GameScene::GameScene(QObject *parent) 
    : QObject(parent) {
    qDebug() << "Creating GameScene for voxel world";
}

void GameScene::addEntity(const GameEntity &entity) {
    // Check if entity already exists and remove it first
    if (entities.contains(entity.id)) {
        removeEntity(entity.id);
    }
    
    entities[entity.id] = entity;
    qDebug() << "Added entity:" << entity.id << "at position" 
             << entity.position.x() << entity.position.y() << entity.position.z();
    emit entityAdded(entity);
}

void GameScene::removeEntity(const QString &id) {
    if (entities.contains(id)) {
        qDebug() << "Removed entity:" << id;
        entities.remove(id);
        emit entityRemoved(id);
    }
}

void GameScene::updateEntityPosition(const QString &id, const QVector3D &position) {
    if (entities.contains(id)) {
        GameEntity &entity = entities[id];
        
        // Debug position update if it's significant
        QVector3D delta = position - entity.position;
        if (delta.length() > 0.01) { // Only log if position changed significantly
            qDebug() << "Entity" << id << "moved from" 
                    << entity.position.x() << entity.position.y() << entity.position.z()
                    << "to" << position.x() << position.y() << position.z();
        }
        
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
    if (!entities.contains(entityId)) {
        return false;
    }
    
    GameEntity &entity = entities[entityId];
    
    // Check collisions with other entities
    for (auto it = entities.constBegin(); it != entities.constEnd(); ++it) {
        // Skip self and non-collidable entities, and static-static collisions
        if (it.key() == entityId || 
            (entity.isStatic && it.value().isStatic)) {
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

bool GameScene::areEntitiesColliding(const GameEntity &entityA, const GameEntity &entityB) const {
    // Skip static-static collisions (walls colliding with walls)
    if (entityA.isStatic && entityB.isStatic) {
        return false;
    }
    
    // Simple AABB collision check with relaxed tolerances
    bool xOverlap = fabs(entityA.position.x() - entityB.position.x()) < 
        (entityA.dimensions.x() / 2 + entityB.dimensions.x() / 2) * 0.9;
        
    bool yOverlap = fabs(entityA.position.y() - entityB.position.y()) < 
        (entityA.dimensions.y() / 2 + entityB.dimensions.y() / 2) * 0.9;
        
    bool zOverlap = fabs(entityA.position.z() - entityB.position.z()) < 
        (entityA.dimensions.z() / 2 + entityB.dimensions.z() / 2) * 0.9;
    
    return xOverlap && yOverlap && zOverlap;
}