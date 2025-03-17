// include/character/models/personality.h
#ifndef CHARACTER_MODELS_PERSONALITY_H
#define CHARACTER_MODELS_PERSONALITY_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>

// Structure to represent a character's personality
struct CharacterPersonality {
    QString archetype;
    QStringList traits;
    QStringList values;
    QStringList fears;
    QStringList desires;
    QString background;
    QString motivation;
    QString quirks;
    QString speechPattern;
    
    // Create a JSON object from this personality
    QJsonObject toJson() const {
        QJsonObject json;
        json["archetype"] = archetype;
        json["traits"] = QJsonArray::fromStringList(traits);
        json["values"] = QJsonArray::fromStringList(values);
        json["fears"] = QJsonArray::fromStringList(fears);
        json["desires"] = QJsonArray::fromStringList(desires);
        json["background"] = background;
        json["motivation"] = motivation;
        json["quirks"] = quirks;
        json["speechPattern"] = speechPattern;
        return json;
    }
    
    // Create a personality from a JSON object
    static CharacterPersonality fromJson(const QJsonObject &json) {
        CharacterPersonality personality;
        personality.archetype = json["archetype"].toString();
        
        QJsonArray traitsArray = json["traits"].toArray();
        for (const QJsonValue &value : traitsArray) {
            personality.traits.append(value.toString());
        }
        
        QJsonArray valuesArray = json["values"].toArray();
        for (const QJsonValue &value : valuesArray) {
            personality.values.append(value.toString());
        }
        
        QJsonArray fearsArray = json["fears"].toArray();
        for (const QJsonValue &value : fearsArray) {
            personality.fears.append(value.toString());
        }
        
        QJsonArray desiresArray = json["desires"].toArray();
        for (const QJsonValue &value : desiresArray) {
            personality.desires.append(value.toString());
        }
        
        personality.background = json["background"].toString();
        personality.motivation = json["motivation"].toString();
        personality.quirks = json["quirks"].toString();
        personality.speechPattern = json["speechPattern"].toString();
        
        return personality;
    }
};

#endif // CHARACTER_MODELS_PERSONALITY_H