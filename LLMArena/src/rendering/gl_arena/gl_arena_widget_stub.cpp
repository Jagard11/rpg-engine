// src/rendering/gl_arena/gl_arena_widget_stub.cpp
#include "../../../include/rendering/gl_arena_widget.h"
#include <QDebug>

// Set active character
void GLArenaWidget::setActiveCharacter(const QString& name) {
    qDebug() << "Setting active character to:" << name;
    m_activeCharacter = name;
}

// Load character sprite with sprite path
void GLArenaWidget::loadCharacterSprite(const QString& characterName, const QString& texturePath) {
    qDebug() << "Loading character sprite:" << characterName << "with texture:" << texturePath;
    
    // Check if the character sprite already exists
    if (m_characterSprites.contains(characterName)) {
        qDebug() << "SEGFAULT-CHECK: Removing existing sprite for:" << characterName;
        CharacterSprite* oldSprite = m_characterSprites[characterName];
        if (oldSprite) {
            // Check if context is valid before deleting
            QOpenGLContext* ctx = QOpenGLContext::currentContext();
            if (!ctx || !ctx->isValid()) {
                qCritical() << "SEGFAULT-CHECK: No valid context when deleting sprite:" << characterName;
            }
            try {
                delete oldSprite;
                qDebug() << "SEGFAULT-CHECK: Successfully deleted old sprite for:" << characterName;
            } catch (const std::exception& e) {
                qCritical() << "SEGFAULT-CHECK: Exception deleting old sprite:" << e.what();
            } catch (...) {
                qCritical() << "SEGFAULT-CHECK: Unknown exception deleting old sprite";
            }
        }
        m_characterSprites.remove(characterName);
    }
    
    // Create a new character sprite
    qDebug() << "Creating new CharacterSprite for:" << characterName;
    CharacterSprite* sprite = new CharacterSprite();
    
    // Initialize it if we have a valid OpenGL context
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (ctx && ctx->isValid() && m_initialized) {
        qDebug() << "Initializing sprite with context for:" << characterName;
        sprite->init(ctx, texturePath, 1.0, 2.0, 1.0);
    } else {
        qWarning() << "Cannot initialize sprite - no valid OpenGL context";
    }
    
    // Store the sprite
    qDebug() << "Storing sprite for:" << characterName;
    m_characterSprites[characterName] = sprite;
    
    // Make sure the character exists in the game scene
    if (m_gameScene) {
        qDebug() << "Checking if entity exists in game scene:" << characterName;
        GameEntity entity = m_gameScene->getEntity(characterName);
        
        if (entity.id.isEmpty()) {
            qDebug() << "Creating new entity in game scene for:" << characterName;
            GameEntity newEntity;
            newEntity.id = characterName;
            newEntity.type = "character";
            newEntity.position = QVector3D(0.0f, 0.0f, 0.0f); // Default position
            newEntity.dimensions = QVector3D(1.0f, 2.0f, 1.0f); // Default dimensions
            newEntity.isStatic = false;
            
            m_gameScene->addEntity(newEntity);
            qDebug() << "Added entity to game scene:" << characterName;
        } else {
            qDebug() << "Entity already exists in game scene:" << characterName;
        }
    } else {
        qWarning() << "Cannot add character to game scene - no game scene available";
    }
}

// Update character position
void GLArenaWidget::updateCharacterPosition(const QString& characterName, float x, float y, float z) {
    qDebug() << "Updating position for" << characterName << "to" << x << y << z;
    
    // Update sprite position if it exists
    if (m_characterSprites.contains(characterName)) {
        CharacterSprite* sprite = m_characterSprites[characterName];
        if (sprite) {
            sprite->updatePosition(x, y, z);
        }
    }
    
    // Update entity in game scene
    if (m_gameScene) {
        GameEntity entity = m_gameScene->getEntity(characterName);
        if (!entity.id.isEmpty()) {
            // GameScene doesn't have updateEntity, so remove and add instead
            m_gameScene->removeEntity(characterName);
            entity.position = QVector3D(x, y, z);
            m_gameScene->addEntity(entity);
            qDebug() << "Updated entity position in game scene for" << characterName;
        }
    }
    
    emit characterPositionUpdated(characterName, x, y, z);
}