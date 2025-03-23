// src/arena/ui/gl_widgets/gl_arena_widget_stub.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"

// Set active character
void GLArenaWidget::setActiveCharacter(const QString& name) {
    m_activeCharacter = name;
}

// Load character sprite with sprite path
void GLArenaWidget::loadCharacterSprite(const QString& characterName, const QString& texturePath) {
    // Check if the character sprite already exists
    if (m_characterSprites.contains(characterName)) {
        CharacterSprite* oldSprite = m_characterSprites[characterName];
        if (oldSprite) {
            // Check if context is valid before deleting
            QOpenGLContext* ctx = QOpenGLContext::currentContext();
            if (ctx && ctx->isValid()) {
                try {
                    delete oldSprite;
                } catch (...) {
                    // Silent exception handling
                }
            }
        }
        m_characterSprites.remove(characterName);
    }
    
    // Create a new character sprite
    CharacterSprite* sprite = nullptr;
    
    try {
        sprite = new CharacterSprite();
        
        // Initialize it if we have a valid OpenGL context
        QOpenGLContext* ctx = QOpenGLContext::currentContext();
        if (ctx && ctx->isValid() && m_initialized) {
            sprite->init(ctx, texturePath, 1.0, 2.0, 1.0);
        }
        
        // Store the sprite
        m_characterSprites[characterName] = sprite;
        
        // Make sure the character exists in the game scene
        if (m_gameScene) {
            GameEntity entity = m_gameScene->getEntity(characterName);
            
            if (entity.id.isEmpty()) {
                GameEntity newEntity;
                newEntity.id = characterName;
                newEntity.type = "character";
                newEntity.position = QVector3D(0.0f, 0.0f, 0.0f); // Default position
                newEntity.dimensions = QVector3D(1.0f, 2.0f, 1.0f); // Default dimensions
                newEntity.isStatic = false;
                
                m_gameScene->addEntity(newEntity);
            }
        }
    } catch (const std::exception& e) {
        qWarning() << "Error creating character sprite for" << characterName << ":" << e.what();
        // Clean up if needed
        delete sprite;
    } catch (...) {
        qWarning() << "Unknown error creating character sprite for" << characterName;
        // Clean up if needed
        delete sprite;
    }
}

// Update character position
void GLArenaWidget::updateCharacterPosition(const QString& characterName, float x, float y, float z) {
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
        }
    }
    
    emit characterPositionUpdated(characterName, x, y, z);
}