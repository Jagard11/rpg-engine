// src/character_manager.cpp
#include "../include/character_persistence.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>

// Constructor
CharacterManager::CharacterManager(QObject *parent) : QObject(parent) {
    // Set up the base directory for character data
    baseDir = QDir::homePath() + "/.oobabooga_rpg/characters";
    QDir dir;
    if (!dir.exists(baseDir)) {
        dir.mkpath(baseDir);
    }
}

// List all available characters
QStringList CharacterManager::listCharacters() {
    QDir dir(baseDir);
    return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

// Create a new character
bool CharacterManager::createCharacter(const QString &name, const CharacterStats &stats, 
                                    const CharacterPersonality &personality,
                                    const CharacterAppearance &appearance) {
    // Create character directory
    QString charDir = baseDir + "/" + name;
    QDir dir;
    if (!dir.exists(charDir)) {
        dir.mkpath(charDir);
    }
    
    // Create memories directory
    dir.mkpath(charDir + "/memories");
    
    // Save character stats
    if (!saveCharacterStats(name, stats)) {
        return false;
    }
    
    // Save character personality
    if (!saveCharacterPersonality(name, personality)) {
        return false;
    }
    
    // Save character appearance
    if (!saveCharacterAppearance(name, appearance)) {
        return false;
    }
    
    return true;
}

// Load character stats
CharacterStats CharacterManager::loadCharacterStats(const QString &name) {
    QString filePath = baseDir + "/" + name + "/stats.json";
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open file for reading:" << filePath;
        return CharacterStats();
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON in file:" << filePath;
        return CharacterStats();
    }
    
    return CharacterStats::fromJson(doc.object());
}

// Save character stats
bool CharacterManager::saveCharacterStats(const QString &name, const CharacterStats &stats) {
    QString filePath = baseDir + "/" + name + "/stats.json";
    QFile file(filePath);
    
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open file for writing:" << filePath;
        return false;
    }
    
    QJsonDocument doc(stats.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    return true;
}

// Load character personality
CharacterPersonality CharacterManager::loadCharacterPersonality(const QString &name) {
    QString filePath = baseDir + "/" + name + "/personality.json";
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open file for reading:" << filePath;
        return CharacterPersonality();
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON in file:" << filePath;
        return CharacterPersonality();
    }
    
    return CharacterPersonality::fromJson(doc.object());
}

// Save character personality
bool CharacterManager::saveCharacterPersonality(const QString &name, const CharacterPersonality &personality) {
    QString filePath = baseDir + "/" + name + "/personality.json";
    QFile file(filePath);
    
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open file for writing:" << filePath;
        return false;
    }
    
    QJsonDocument doc(personality.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    return true;
}

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

// Load all memories for a character
QVector<Memory> CharacterManager::loadMemories(const QString &characterName) {
    QString filePath = baseDir + "/" + characterName + "/memories/events.json";
    QFile file(filePath);
    
    QVector<Memory> memories;
    
    if (!file.exists()) {
        // No memories file yet, return empty list
        return memories;
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open memories file for reading:" << filePath;
        return memories;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isArray()) {
        qWarning() << "Invalid JSON in memories file:" << filePath;
        return memories;
    }
    
    QJsonArray array = doc.array();
    for (const QJsonValue &value : array) {
        if (value.isObject()) {
            memories.append(Memory::fromJson(value.toObject()));
        }
    }
    
    return memories;
}

// Generate a character profile for the LLM context
QString CharacterManager::generateCharacterProfile(const QString &characterName) {
    CharacterStats stats = loadCharacterStats(characterName);
    CharacterPersonality personality = loadCharacterPersonality(characterName);
    CharacterAppearance appearance = loadCharacterAppearance(characterName);
    
    QString profile = "CHARACTER PROFILE:\n";
    
    // Basic info
    profile += QString("Name: %1\n").arg(stats.name);
    profile += QString("Race: %1\n").arg(stats.race);
    profile += QString("Class: %1\n").arg(stats.characterClass);
    profile += QString("Level: %1\n").arg(stats.level);
    
    // Appearance
    profile += "\nAPPEARANCE:\n";
    profile += QString("Gender: %1\n").arg(appearance.gender);
    profile += QString("Age: %1\n").arg(appearance.age);
    profile += QString("Height: %1\n").arg(appearance.height);
    profile += QString("Build: %1\n").arg(appearance.build);
    profile += QString("Hair: %1 %2\n").arg(appearance.hairColor).arg(appearance.hairStyle);
    profile += QString("Eyes: %1\n").arg(appearance.eyeColor);
    profile += QString("Skin: %1\n").arg(appearance.skinTone);
    profile += QString("Clothing: %1\n").arg(appearance.clothing);
    if (!appearance.distinguishingFeatures.isEmpty()) {
        profile += QString("Distinguishing Features: %1\n").arg(appearance.distinguishingFeatures);
    }
    
    // Personality
    profile += "\nPERSONALITY:\n";
    profile += QString("Archetype: %1\n").arg(personality.archetype);
    
    if (!personality.traits.isEmpty()) {
        profile += "Traits: " + personality.traits.join(", ") + "\n";
    }
    
    if (!personality.values.isEmpty()) {
        profile += "Values: " + personality.values.join(", ") + "\n";
    }
    
    if (!personality.fears.isEmpty()) {
        profile += "Fears: " + personality.fears.join(", ") + "\n";
    }
    
    if (!personality.desires.isEmpty()) {
        profile += "Desires: " + personality.desires.join(", ") + "\n";
    }
    
    if (!personality.quirks.isEmpty()) {
        profile += "Quirks: " + personality.quirks + "\n";
    }
    
    if (!personality.speechPattern.isEmpty()) {
        profile += "Speech Pattern: " + personality.speechPattern + "\n";
    }
    
    // Background
    if (!personality.background.isEmpty()) {
        profile += "\nBACKGROUND:\n" + personality.background + "\n";
    }
    
    // Motivation
    if (!personality.motivation.isEmpty()) {
        profile += "\nMOTIVATION:\n" + personality.motivation + "\n";
    }
    
    return profile;
}

// Delete a character
bool CharacterManager::deleteCharacter(const QString &name) {
    QDir charDir(baseDir + "/" + name);
    if (!charDir.exists()) {
        qWarning() << "Character directory does not exist:" << name;
        return false;
    }
    
    return charDir.removeRecursively();
}

// Get a list of all known locations across memories
QStringList CharacterManager::getKnownLocations(const QString &characterName) {
    QStringList knownLocations;
    
    // Load all memories
    QVector<Memory> memories = loadMemories(characterName);
    
    // Extract all unique locations
    for (const Memory &memory : memories) {
        for (const QString &location : memory.locations) {
            if (!knownLocations.contains(location)) {
                knownLocations.append(location);
            }
        }
    }
    
    return knownLocations;
}

// Generate a character journal for a specific time period
QString CharacterManager::generateCharacterJournal(const QString &characterName, 
                                              const QDate &startDate,
                                              const QDate &endDate) {
    QVector<Memory> memories = loadMemories(characterName);
    
    // Filter memories in date range
    QVector<Memory> rangeMemories;
    for (const Memory &memory : memories) {
        QDate memoryDate = memory.timestamp.date();
        if (memoryDate >= startDate && memoryDate <= endDate) {
            rangeMemories.append(memory);
        }
    }
    
    // Sort by date
    std::sort(rangeMemories.begin(), rangeMemories.end(), 
              [](const Memory &a, const Memory &b) {
                  return a.timestamp < b.timestamp;
              });
    
    // Generate journal
    QString journal = "# Character Journal: " + characterName + "\n";
    journal += "## Period: " + startDate.toString("yyyy-MM-dd") + " to " + 
              endDate.toString("yyyy-MM-dd") + "\n\n";
    
    QDate currentDate;
    for (const Memory &memory : rangeMemories) {
        QDate memoryDate = memory.timestamp.date();
        
        if (memoryDate != currentDate) {
            currentDate = memoryDate;
            journal += "### " + currentDate.toString("yyyy-MM-dd") + "\n\n";
        }
        
        journal += "#### " + memory.title + "\n";
        journal += memory.description + "\n\n";
        
        if (!memory.emotions.isEmpty()) {
            journal += "*Emotions: " + memory.emotions.join(", ") + "*\n\n";
        }
    }
    
    return journal;
}

// Export journal to file
bool CharacterManager::exportJournalToFile(const QString &characterName,
                                       const QString &journal,
                                       const QString &filePath) {
    QFile file(filePath);
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not open file for writing:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    out << journal;
    file.close();
    
    return true;
}

// Build memory indices for faster retrieval
void CharacterManager::buildMemoryIndex(const QString &characterName) {
    QVector<Memory> memories = loadMemories(characterName);
    
    // Build entity index
    QMap<QString, QVector<QString>> entityIndex;
    for (const Memory &memory : memories) {
        for (const QString &entity : memory.entities) {
            entityIndex[entity.toLower()].append(memory.id);
        }
    }
    
    // Build location index
    QMap<QString, QVector<QString>> locationIndex;
    for (const Memory &memory : memories) {
        for (const QString &location : memory.locations) {
            locationIndex[location.toLower()].append(memory.id);
        }
    }
    
    // Build emotion index
    QMap<QString, QVector<QString>> emotionIndex;
    for (const Memory &memory : memories) {
        for (const QString &emotion : memory.emotions) {
            emotionIndex[emotion.toLower()].append(memory.id);
        }
    }
    
    // Save indices
    saveMemoryIndices(characterName, entityIndex, locationIndex, emotionIndex);
}

// Save memory indices
bool CharacterManager::saveMemoryIndices(const QString &characterName,
                                     const QMap<QString, QVector<QString>> &entityIndex,
                                     const QMap<QString, QVector<QString>> &locationIndex,
                                     const QMap<QString, QVector<QString>> &emotionIndex) {
    // Create index directory
    QString indexDir = baseDir + "/" + characterName + "/memories/index";
    QDir dir;
    if (!dir.exists(indexDir)) {
        dir.mkpath(indexDir);
    }
    
    // Save entity index
    QJsonObject entityObj;
    for (auto it = entityIndex.constBegin(); it != entityIndex.constEnd(); ++it) {
        // Convert QVector<QString> to QStringList manually
        QStringList stringList;
        for (const QString &str : it.value()) {
            stringList.append(str);
        }
        entityObj[it.key()] = QJsonArray::fromStringList(stringList);
    }
    
    QFile entityFile(indexDir + "/entities.json");
    if (!entityFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open entity index file for writing";
        return false;
    }
    QJsonDocument entityDoc(entityObj);
    entityFile.write(entityDoc.toJson(QJsonDocument::Indented));
    entityFile.close();
    
    // Save location index
    QJsonObject locationObj;
    for (auto it = locationIndex.constBegin(); it != locationIndex.constEnd(); ++it) {
        // Convert QVector<QString> to QStringList manually
        QStringList stringList;
        for (const QString &str : it.value()) {
            stringList.append(str);
        }
        locationObj[it.key()] = QJsonArray::fromStringList(stringList);
    }
    
    QFile locationFile(indexDir + "/locations.json");
    if (!locationFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open location index file for writing";
        return false;
    }
    QJsonDocument locationDoc(locationObj);
    locationFile.write(locationDoc.toJson(QJsonDocument::Indented));
    locationFile.close();
    
    // Save emotion index
    QJsonObject emotionObj;
    for (auto it = emotionIndex.constBegin(); it != emotionIndex.constEnd(); ++it) {
        // Convert QVector<QString> to QStringList manually
        QStringList stringList;
        for (const QString &str : it.value()) {
            stringList.append(str);
        }
        emotionObj[it.key()] = QJsonArray::fromStringList(stringList);
    }
    
    QFile emotionFile(indexDir + "/emotions.json");
    if (!emotionFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open emotion index file for writing";
        return false;
    }
    QJsonDocument emotionDoc(emotionObj);
    emotionFile.write(emotionDoc.toJson(QJsonDocument::Indented));
    emotionFile.close();
    
    return true;
}

// Load memory indices
bool CharacterManager::loadMemoryIndices(const QString &characterName,
                                     QMap<QString, QVector<QString>> &entityIndex,
                                     QMap<QString, QVector<QString>> &locationIndex,
                                     QMap<QString, QVector<QString>> &emotionIndex) {
    QString indexDir = baseDir + "/" + characterName + "/memories/index";
    
    // Clear existing indices
    entityIndex.clear();
    locationIndex.clear();
    emotionIndex.clear();
    
    // Load entity index
    QFile entityFile(indexDir + "/entities.json");
    if (entityFile.exists() && entityFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(entityFile.readAll());
        entityFile.close();
        
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                QJsonArray array = it.value().toArray();
                QVector<QString> ids;
                for (const QJsonValue &val : array) {
                    ids.append(val.toString());
                }
                entityIndex[it.key()] = ids;
            }
        }
    }
    
    // Load location index
    QFile locationFile(indexDir + "/locations.json");
    if (locationFile.exists() && locationFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(locationFile.readAll());
        locationFile.close();
        
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                QJsonArray array = it.value().toArray();
                QVector<QString> ids;
                for (const QJsonValue &val : array) {
                    ids.append(val.toString());
                }
                locationIndex[it.key()] = ids;
            }
        }
    }
    
    // Load emotion index
    QFile emotionFile(indexDir + "/emotions.json");
    if (emotionFile.exists() && emotionFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(emotionFile.readAll());
        emotionFile.close();
        
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                QJsonArray array = it.value().toArray();
                QVector<QString> ids;
                for (const QJsonValue &val : array) {
                    ids.append(val.toString());
                }
                emotionIndex[it.key()] = ids;
            }
        }
    }
    
    // Return true if at least one index was loaded
    return !entityIndex.isEmpty() || !locationIndex.isEmpty() || !emotionIndex.isEmpty();
}

// Find memories by entity using index
QVector<Memory> CharacterManager::findMemoriesByEntity(const QString &characterName,
                                                   const QString &entity) {
    QVector<Memory> result;
    
    // Load indices
    QMap<QString, QVector<QString>> entityIndex;
    QMap<QString, QVector<QString>> locationIndex;
    QMap<QString, QVector<QString>> emotionIndex;
    
    if (!loadMemoryIndices(characterName, entityIndex, locationIndex, emotionIndex)) {
        // If indices not available, rebuild them
        buildMemoryIndex(characterName);
        loadMemoryIndices(characterName, entityIndex, locationIndex, emotionIndex);
    }
    
    // Find memory IDs for the entity
    QVector<QString> memoryIds;
    QString lowerEntity = entity.toLower();
    
    if (entityIndex.contains(lowerEntity)) {
        memoryIds = entityIndex[lowerEntity];
    }
    
    // Load all memories
    QVector<Memory> allMemories = loadMemories(characterName);
    
    // Filter for matching IDs
    for (const Memory &memory : allMemories) {
        if (memoryIds.contains(memory.id)) {
            result.append(memory);
        }
    }
    
    return result;
}

// Find memories by location using index
QVector<Memory> CharacterManager::findMemoriesByLocation(const QString &characterName,
                                                     const QString &location) {
    QVector<Memory> result;
    
    // Load indices
    QMap<QString, QVector<QString>> entityIndex;
    QMap<QString, QVector<QString>> locationIndex;
    QMap<QString, QVector<QString>> emotionIndex;
    
    if (!loadMemoryIndices(characterName, entityIndex, locationIndex, emotionIndex)) {
        // If indices not available, rebuild them
        buildMemoryIndex(characterName);
        loadMemoryIndices(characterName, entityIndex, locationIndex, emotionIndex);
    }
    
    // Find memory IDs for the location
    QVector<QString> memoryIds;
    QString lowerLocation = location.toLower();
    
    if (locationIndex.contains(lowerLocation)) {
        memoryIds = locationIndex[lowerLocation];
    }
    
    // Load all memories
    QVector<Memory> allMemories = loadMemories(characterName);
    
    // Filter for matching IDs
    for (const Memory &memory : allMemories) {
        if (memoryIds.contains(memory.id)) {
            result.append(memory);
        }
    }
    
    return result;
}

// Find memories by emotion using index
QVector<Memory> CharacterManager::findMemoriesByEmotion(const QString &characterName,
                                                    const QString &emotion) {
    QVector<Memory> result;
    
    // Load indices
    QMap<QString, QVector<QString>> entityIndex;
    QMap<QString, QVector<QString>> locationIndex;
    QMap<QString, QVector<QString>> emotionIndex;
    
    if (!loadMemoryIndices(characterName, entityIndex, locationIndex, emotionIndex)) {
        // If indices not available, rebuild them
        buildMemoryIndex(characterName);
        loadMemoryIndices(characterName, entityIndex, locationIndex, emotionIndex);
    }
    
    // Find memory IDs for the emotion
    QVector<QString> memoryIds;
    QString lowerEmotion = emotion.toLower();
    
    if (emotionIndex.contains(lowerEmotion)) {
        memoryIds = emotionIndex[lowerEmotion];
    }
    
    // Load all memories
    QVector<Memory> allMemories = loadMemories(characterName);
    
    // Filter for matching IDs
    for (const Memory &memory : allMemories) {
        if (memoryIds.contains(memory.id)) {
            result.append(memory);
        }
    }
    
    return result;
}

// Consolidate memories for a character
bool CharacterManager::consolidateMemories(const QString &characterName) {
    QVector<Memory> memories = loadMemories(characterName);
    
    // Group related memories by entity
    QMap<QString, QVector<Memory>> entityGroups;
    for (const Memory &memory : memories) {
        for (const QString &entity : memory.entities) {
            entityGroups[entity].append(memory);
        }
    }
    
    // For groups with many similar memories, create consolidated summaries
    for (auto it = entityGroups.begin(); it != entityGroups.end(); ++it) {
        if (it.value().size() > 10) {
            Memory consolidated;
            consolidated.id = QDateTime::currentDateTime().toString("yyyyMMddhhmmss") + 
                          QString::number(QRandomGenerator::global()->bounded(1000));
            consolidated.timestamp = QDateTime::currentDateTime();
            consolidated.type = "consolidated";
            consolidated.title = "Memories about " + it.key();
            consolidated.description = generateConsolidatedDescription(it.value());
            consolidated.entities.append(it.key());
            
            // Find common locations and emotions
            QSet<QString> locations;
            QSet<QString> emotions;
            
            for (const Memory &memory : it.value()) {
                for (const QString &location : memory.locations) {
                    locations.insert(location);
                }
                for (const QString &emotion : memory.emotions) {
                    emotions.insert(emotion);
                }
            }
            
            consolidated.locations = locations.values();
            consolidated.emotions = emotions.values();
            consolidated.emotionalIntensity = 5; // Medium intensity for consolidated memories
            
            // Add consolidated memory
            memories.append(consolidated);
        }
    }
    
    // Save all memories including consolidated ones
    return saveMemories(characterName, memories);
}

// Generate a consolidated description from a set of memories
QString CharacterManager::generateConsolidatedDescription(const QVector<Memory> &memories) {
    if (memories.isEmpty()) {
        return "No memories to consolidate.";
    }
    
    // Sort by timestamp
    QVector<Memory> sortedMemories = memories;
    std::sort(sortedMemories.begin(), sortedMemories.end(), 
              [](const Memory &a, const Memory &b) {
                  return a.timestamp < b.timestamp;
              });
    
    // Create a summary
    QString description = "This is a summary of multiple related memories:\n\n";
    
    // Add timespan
    if (sortedMemories.size() > 1) {
        QDateTime firstDate = sortedMemories.first().timestamp;
        QDateTime lastDate = sortedMemories.last().timestamp;
        
        description += QString("From %1 to %2:\n\n").arg(
            firstDate.toString("yyyy-MM-dd"), 
            lastDate.toString("yyyy-MM-dd"));
    }
    
    // Add key events (first 3 and last 2)
    int totalMemories = sortedMemories.size();
    int shownMemories = qMin(5, totalMemories);
    
    for (int i = 0; i < qMin(3, totalMemories); i++) {
        const Memory &memory = sortedMemories[i];
        description += QString("- %1: %2\n").arg(
            memory.timestamp.toString("yyyy-MM-dd"), 
            memory.title);
    }
    
    // Add ellipsis if there are more than 5 memories
    if (totalMemories > 5) {
        description += QString("- ... (%1 more memories) ...\n").arg(totalMemories - 5);
    }
    
    // Add last 2 memories if there are more than 3 total
    if (totalMemories > 3) {
        for (int i = qMax(3, totalMemories - 2); i < totalMemories; i++) {
            const Memory &memory = sortedMemories[i];
            description += QString("- %1: %2\n").arg(
                memory.timestamp.toString("yyyy-MM-dd"), 
                memory.title);
        }
    }
    
    return description;
}

// Save all memories for a character
bool CharacterManager::saveMemories(const QString &characterName, const QVector<Memory> &memories) {
    QString filePath = baseDir + "/" + characterName + "/memories/events.json";
    QFile file(filePath);
    
    // Make sure the directory exists
    QFileInfo fileInfo(filePath);
    QDir().mkpath(fileInfo.absolutePath());
    
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open memories file for writing:" << filePath;
        return false;
    }
    
    QJsonArray array;
    for (const Memory &memory : memories) {
        array.append(memory.toJson());
    }
    
    QJsonDocument doc(array);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    return true;
}

// Update recall information for a memory
bool CharacterManager::updateMemoryRecallInfo(const QString &characterName, const Memory &memory) {
    // Load all memories
    QVector<Memory> memories = loadMemories(characterName);
    
    // Find and update the specified memory
    for (int i = 0; i < memories.size(); ++i) {
        if (memories[i].id == memory.id) {
            memories[i].lastRecalled = memory.lastRecalled;
            memories[i].recallCount = memory.recallCount;
            
            // Save updated memories
            return saveMemories(characterName, memories);
        }
    }
    
    qWarning() << "Memory not found:" << memory.id;
    return false;
}

// Calculate emotional intensity based on text content
int CharacterManager::calculateEmotionalIntensity(const QString &text) {
    // Base intensity
    int intensity = 3;
    
    // Check for intensifiers
    QStringList intensifiers = {"very", "extremely", "incredibly", "absolutely", "deeply"};
    for (const QString &intensifier : intensifiers) {
        if (text.contains(intensifier, Qt::CaseInsensitive)) {
            intensity += 1;
        }
    }
    
    // Check for strong emotional words
    QStringList strongEmotions = {"furious", "ecstatic", "heartbroken", "terrified", "adore"};
    for (const QString &emotion : strongEmotions) {
        if (text.contains(emotion, Qt::CaseInsensitive)) {
            intensity += 2;
        }
    }
    
    // Check for exclamation marks
    intensity += text.count('!');
    
    // Cap at 10
    return qMin(intensity, 10);
}

// Helper function to extract entities from text
QStringList CharacterManager::extractEntities(const QString &text) {
    QStringList potentialEntities;
    QStringList words = text.split(QRegExp("\\s+"));
    
    // Simple approach: words that start with capital letters might be entities
    for (const QString &word : words) {
        QString cleanWord = word.trimmed();
        if (cleanWord.length() > 1 && cleanWord[0].isUpper()) {
            // Remove punctuation
            cleanWord.remove(QRegExp("[,\\.\\?!;:\"]"));
            if (!cleanWord.isEmpty()) {
                potentialEntities.append(cleanWord);
            }
        }
    }
    
    // Remove duplicates
    QSet<QString> uniqueEntities(potentialEntities.begin(), potentialEntities.end());
    return uniqueEntities.values();
}

// Helper function to extract locations from text
QStringList CharacterManager::extractLocations(const QString &text, const QStringList &knownLocations) {
    QStringList foundLocations;
    
    // Check for known locations
    for (const QString &location : knownLocations) {
        if (text.contains(location, Qt::CaseInsensitive)) {
            foundLocations.append(location);
        }
    }
    
    // Look for location indicators
    QRegExp locationRegex("\\b(at|in|near|to) the ([A-Z][a-z]+(\\s+[A-Z][a-z]+)*)\\b");
    int pos = 0;
    while ((pos = locationRegex.indexIn(text, pos)) != -1) {
        foundLocations.append(locationRegex.cap(2));
        pos += locationRegex.matchedLength();
    }
    
    // Remove duplicates
    QSet<QString> uniqueLocations(foundLocations.begin(), foundLocations.end());
    return uniqueLocations.values();
}

// Add a memory to a character
bool CharacterManager::addMemory(const QString &characterName, const Memory &memory) {
    // Load existing memories
    QVector<Memory> memories = loadMemories(characterName);
    
    // Add new memory
    memories.append(memory);
    
    // Save all memories
    return saveMemories(characterName, memories);
}
