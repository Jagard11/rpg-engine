// include/character_persistence.h
#ifndef CHARACTER_PERSISTENCE_H
#define CHARACTER_PERSISTENCE_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QTextStream>
#include <QRandomGenerator>
#include <QDebug>
#include <QMap>
#include <QVector>
#include <QUrl>
#include <QtMath>
#include <algorithm>
#include <functional>

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

// Structure to represent a character's appearance
struct CharacterAppearance {
    QString gender;
    QString age;
    QString height;
    QString build;
    QString hairColor;
    QString hairStyle;
    QString eyeColor;
    QString skinTone;
    QString clothing;
    QString distinguishingFeatures;
    QString generalDescription;
    
    // Create a JSON object from this appearance
    QJsonObject toJson() const {
        QJsonObject json;
        json["gender"] = gender;
        json["age"] = age;
        json["height"] = height;
        json["build"] = build;
        json["hairColor"] = hairColor;
        json["hairStyle"] = hairStyle;
        json["eyeColor"] = eyeColor;
        json["skinTone"] = skinTone;
        json["clothing"] = clothing;
        json["distinguishingFeatures"] = distinguishingFeatures;
        json["generalDescription"] = generalDescription;
        return json;
    }
    
    // Create an appearance from a JSON object
    static CharacterAppearance fromJson(const QJsonObject &json) {
        CharacterAppearance appearance;
        appearance.gender = json["gender"].toString();
        appearance.age = json["age"].toString();
        appearance.height = json["height"].toString();
        appearance.build = json["build"].toString();
        appearance.hairColor = json["hairColor"].toString();
        appearance.hairStyle = json["hairStyle"].toString();
        appearance.eyeColor = json["eyeColor"].toString();
        appearance.skinTone = json["skinTone"].toString();
        appearance.clothing = json["clothing"].toString();
        appearance.distinguishingFeatures = json["distinguishingFeatures"].toString();
        appearance.generalDescription = json["generalDescription"].toString();
        return appearance;
    }
};

// Class to manage character profiles
class CharacterManager : public QObject {
    Q_OBJECT

public:
    CharacterManager(QObject *parent = nullptr);
    
    // List all available characters
    Q_INVOKABLE QStringList listCharacters();
    
    // Create a new character
    Q_INVOKABLE bool createCharacter(const QString &name, const CharacterStats &stats, 
                                  const CharacterPersonality &personality,
                                  const CharacterAppearance &appearance);
    
    // Load character stats
    Q_INVOKABLE CharacterStats loadCharacterStats(const QString &name);
    
    // Save character stats
    Q_INVOKABLE bool saveCharacterStats(const QString &name, const CharacterStats &stats);
    
    // Load character personality
    Q_INVOKABLE CharacterPersonality loadCharacterPersonality(const QString &name);
    
    // Save character personality
    Q_INVOKABLE bool saveCharacterPersonality(const QString &name, const CharacterPersonality &personality);
    
    // Load character appearance
    Q_INVOKABLE CharacterAppearance loadCharacterAppearance(const QString &name);
    
    // Save character appearance
    Q_INVOKABLE bool saveCharacterAppearance(const QString &name, const CharacterAppearance &appearance);
    
    // Add a memory to a character
    Q_INVOKABLE bool addMemory(const QString &characterName, const Memory &memory);
    
    // Load all memories for a character
    Q_INVOKABLE QVector<Memory> loadMemories(const QString &characterName);
    
    // Save all memories for a character
    Q_INVOKABLE bool saveMemories(const QString &characterName, const QVector<Memory> &memories);
    
    // Retrieve memories relevant to the current context
    Q_INVOKABLE QVector<Memory> retrieveRelevantMemories(
            const QString &characterName, 
            const QString &currentContext, 
            const QStringList &currentEntities,
            const QStringList &currentLocations,
            int maxMemories = 5);
    
    // Update recall information for a memory
    Q_INVOKABLE bool updateMemoryRecallInfo(const QString &characterName, const Memory &memory);
    
    // Generate a formatted string of relevant memories for the LLM context
    Q_INVOKABLE QString generateMemoriesContext(const QString &characterName, 
                                             const QString &currentContext,
                                             const QStringList &currentEntities,
                                             const QStringList &currentLocations,
                                             int maxMemories = 5);
    
    // Generate a character profile for the LLM context
    Q_INVOKABLE QString generateCharacterProfile(const QString &characterName);
    
    // Delete a character
    Q_INVOKABLE bool deleteCharacter(const QString &name);
    
    // Get a list of all known locations across memories
    Q_INVOKABLE QStringList getKnownLocations(const QString &characterName);
    
    // Generate a character journal for a specific time period
    Q_INVOKABLE QString generateCharacterJournal(const QString &characterName, 
                                              const QDate &startDate,
                                              const QDate &endDate);
    
    // Export journal to file
    Q_INVOKABLE bool exportJournalToFile(const QString &characterName,
                                       const QString &journal,
                                       const QString &filePath);
    
    // Process text for possible memory creation
    Q_INVOKABLE void processForMemoryCreation(const QString &userMessage, 
                                            const QString &aiResponse,
                                            const QString &characterName);
    
    // Build memory indices for faster retrieval
    Q_INVOKABLE void buildMemoryIndex(const QString &characterName);
    
    // Save memory indices
    Q_INVOKABLE bool saveMemoryIndices(const QString &characterName,
                                     const QMap<QString, QVector<QString>> &entityIndex,
                                     const QMap<QString, QVector<QString>> &locationIndex,
                                     const QMap<QString, QVector<QString>> &emotionIndex);
    
    // Load memory indices
    Q_INVOKABLE bool loadMemoryIndices(const QString &characterName,
                                     QMap<QString, QVector<QString>> &entityIndex,
                                     QMap<QString, QVector<QString>> &locationIndex,
                                     QMap<QString, QVector<QString>> &emotionIndex);
    
    // Find memories by entity using index
    Q_INVOKABLE QVector<Memory> findMemoriesByEntity(const QString &characterName,
                                                   const QString &entity);
    
    // Find memories by location using index
    Q_INVOKABLE QVector<Memory> findMemoriesByLocation(const QString &characterName,
                                                     const QString &location);
    
    // Find memories by emotion using index
    Q_INVOKABLE QVector<Memory> findMemoriesByEmotion(const QString &characterName,
                                                    const QString &emotion);
    
    // Consolidate memories for a character
    Q_INVOKABLE bool consolidateMemories(const QString &characterName);
    
    // Generate a consolidated description from a set of memories
    Q_INVOKABLE QString generateConsolidatedDescription(const QVector<Memory> &memories);
    
    // Calculate the relevance score of a memory to the current context
    double calculateRelevanceScore(const Memory &memory, 
                                const QString &currentContext,
                                const QStringList &currentEntities,
                                const QStringList &currentLocations);
    
    // Determine the type of context (emotional, combat, exploration, etc.)
    QString determineContextType(const QString &context);
    
private:
    QString baseDir;
    
    // Context type weights for different types of contexts
    QMap<QString, QMap<QString, double>> contextTypeWeights;
    
    // Helper method to truncate text to a maximum length
    QString truncateText(const QString &text, int maxLength);
    
    // Helper function to extract entities from text
    QStringList extractEntities(const QString &text);
    
    // Helper function to extract locations from text
    QStringList extractLocations(const QString &text, const QStringList &knownLocations);
    
    // Calculate emotional intensity based on text content
    int calculateEmotionalIntensity(const QString &text);
};

#endif // CHARACTER_PERSISTENCE_H