// src/character/core/character_manager.cpp
#include "../../../include/character/manager/character_manager.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

// Load character appearance
CharacterAppearance CharacterManager::loadCharacterAppearance(const QString &name) {
    QString filePath = baseDir + "/" + name + "/appearance.json";
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open file for reading:" << filePath;
        return CharacterAppearance();
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON in file:" << filePath;
        return CharacterAppearance();
    }
    
    return CharacterAppearance::fromJson(doc.object());
}

// Save character appearance
bool CharacterManager::saveCharacterAppearance(const QString &name, const CharacterAppearance &appearance) {
    QString filePath = baseDir + "/" + name + "/appearance.json";
    QFile file(filePath);
    
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open file for writing:" << filePath;
        return false;
    }
    
    QJsonDocument doc(appearance.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    return true;
}

// Set character sprite
bool CharacterManager::setCharacterSprite(const QString &name, const QString &spritePath) {
    // Load current appearance
    CharacterAppearance appearance = loadCharacterAppearance(name);
    
    // Update sprite path
    appearance.spritePath = spritePath;
    
    // Save updated appearance
    bool result = saveCharacterAppearance(name, appearance);
    
    // Emit signal if successful
    if (result) {
        emit characterSpriteChanged(name, spritePath);
    }
    
    return result;
}

// Set character collision geometry
bool CharacterManager::setCharacterCollisionGeometry(const QString &name, 
                                                const CharacterCollisionGeometry &geometry) {
    // Load current appearance
    CharacterAppearance appearance = loadCharacterAppearance(name);
    
    // Update collision geometry
    appearance.collision = geometry;
    
    // Save updated appearance
    bool result = saveCharacterAppearance(name, appearance);
    
    // Emit signal if successful
    if (result) {
        emit characterCollisionGeometryChanged(name, geometry);
    }
    
    return result;
}