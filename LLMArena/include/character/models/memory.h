// include/character/models/memory.h
#ifndef CHARACTER_MODELS_MEMORY_H
#define CHARACTER_MODELS_MEMORY_H

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>

// Structure to represent a memory
struct Memory {
    QString id;
    QDateTime timestamp;
    QString type;          // "event", "conversation", "discovery", etc.
    QString title;
    QString description;
    QStringList emotions;
    int emotionalIntensity; // 1-10 scale
    QStringList locations;
    QStringList entities;
    QStringList tags;
    QStringList relationships;
    QDateTime lastRecalled;
    int recallCount;
    
    // Create a JSON object from this memory
    QJsonObject toJson() const {
        QJsonObject json;
        json["id"] = id;
        json["timestamp"] = timestamp.toString(Qt::ISODate);
        json["type"] = type;
        json["title"] = title;
        json["description"] = description;
        json["emotions"] = QJsonArray::fromStringList(emotions);
        json["emotionalIntensity"] = emotionalIntensity;
        json["locations"] = QJsonArray::fromStringList(locations);
        json["entities"] = QJsonArray::fromStringList(entities);
        json["tags"] = QJsonArray::fromStringList(tags);
        json["relationships"] = QJsonArray::fromStringList(relationships);
        json["lastRecalled"] = lastRecalled.toString(Qt::ISODate);
        json["recallCount"] = recallCount;
        return json;
    }
    
    // Create a memory from a JSON object
    static Memory fromJson(const QJsonObject &json) {
        Memory memory;
        memory.id = json["id"].toString();
        memory.timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);
        memory.type = json["type"].toString();
        memory.title = json["title"].toString();
        memory.description = json["description"].toString();
        
        QJsonArray emotionsArray = json["emotions"].toArray();
        for (const QJsonValue &value : emotionsArray) {
            memory.emotions.append(value.toString());
        }
        
        memory.emotionalIntensity = json["emotionalIntensity"].toInt();
        
        QJsonArray locationsArray = json["locations"].toArray();
        for (const QJsonValue &value : locationsArray) {
            memory.locations.append(value.toString());
        }
        
        QJsonArray entitiesArray = json["entities"].toArray();
        for (const QJsonValue &value : entitiesArray) {
            memory.entities.append(value.toString());
        }
        
        QJsonArray tagsArray = json["tags"].toArray();
        for (const QJsonValue &value : tagsArray) {
            memory.tags.append(value.toString());
        }
        
        QJsonArray relationshipsArray = json["relationships"].toArray();
        for (const QJsonValue &value : relationshipsArray) {
            memory.relationships.append(value.toString());
        }
        
        memory.lastRecalled = QDateTime::fromString(json["lastRecalled"].toString(), Qt::ISODate);
        memory.recallCount = json["recallCount"].toInt();
        
        return memory;
    }
};

#endif // CHARACTER_MODELS_MEMORY_H