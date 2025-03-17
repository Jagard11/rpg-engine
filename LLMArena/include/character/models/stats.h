// include/character/models/stats.h
#ifndef CHARACTER_MODELS_STATS_H
#define CHARACTER_MODELS_STATS_H

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>

// Structure to represent a character's game stats and abilities
struct CharacterStats {
    // Basic info
    QString name;
    QString race;
    QString characterClass;
    int level;
    
    // Base attributes (starting values)
    QMap<QString, int> baseAttributes; // strength, dexterity, etc.
    
    // Base abilities (not modified by current game state)
    QJsonArray baseAbilities;
    
    // Create a JSON object from these stats
    QJsonObject toJson() const {
        QJsonObject json;
        json["name"] = name;
        json["race"] = race;
        json["class"] = characterClass;
        json["level"] = level;
        
        QJsonObject attributesObj;
        for (auto it = baseAttributes.constBegin(); it != baseAttributes.constEnd(); ++it) {
            attributesObj[it.key()] = it.value();
        }
        json["baseAttributes"] = attributesObj;
        
        json["baseAbilities"] = baseAbilities;
        
        return json;
    }
    
    // Create stats from a JSON object
    static CharacterStats fromJson(const QJsonObject &json) {
        CharacterStats stats;
        stats.name = json["name"].toString();
        stats.race = json["race"].toString();
        stats.characterClass = json["class"].toString();
        stats.level = json["level"].toInt();
        
        QJsonObject attributesObj = json["baseAttributes"].toObject();
        for (auto it = attributesObj.constBegin(); it != attributesObj.constEnd(); ++it) {
            stats.baseAttributes[it.key()] = it.value().toInt();
        }
        
        stats.baseAbilities = json["baseAbilities"].toArray();
        
        return stats;
    }
};

#endif // CHARACTER_MODELS_STATS_H