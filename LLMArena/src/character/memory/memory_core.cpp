// src/character/memory/memory_core.cpp
// Implementations of non-CharacterManager memory system functions
#include "../../../include/character/memory/memory_system.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>
#include <QRandomGenerator>
#include <cmath>
#include <algorithm>

// Memory consolidation functions
MemoryConsolidator::MemoryConsolidator(CharacterManager *manager)
    : characterManager(manager) {
}

// Consolidate similar memories into summary memories
bool MemoryConsolidator::consolidateMemories(const QString &characterName) {
    QVector<Memory> memories = characterManager->loadMemories(characterName);
    
    // Group memories by related entities
    QMap<QString, QVector<Memory>> entityGroups = groupMemoriesByEntity(memories);
    
    // For groups with many similar memories, create consolidated summaries
    QVector<Memory> consolidatedMemories;
    
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
            consolidatedMemories.append(consolidated);
        }
    }
    
    // Add consolidated memories to the original set
    memories.append(consolidatedMemories);
    
    // Save all memories including consolidated ones
    return characterManager->saveMemories(characterName, memories);
}

// Generate a summary description from a set of related memories
QString MemoryConsolidator::generateConsolidatedDescription(const QVector<Memory> &memories) {
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

// Group memories by related entities
QMap<QString, QVector<Memory>> MemoryConsolidator::groupMemoriesByEntity(const QVector<Memory> &memories) {
    QMap<QString, QVector<Memory>> entityGroups;
    
    for (const Memory &memory : memories) {
        for (const QString &entity : memory.entities) {
            entityGroups[entity].append(memory);
        }
    }
    
    return entityGroups;
}

// Group memories by location
QMap<QString, QVector<Memory>> MemoryConsolidator::groupMemoriesByLocation(const QVector<Memory> &memories) {
    QMap<QString, QVector<Memory>> locationGroups;
    
    for (const Memory &memory : memories) {
        for (const QString &location : memory.locations) {
            locationGroups[location].append(memory);
        }
    }
    
    return locationGroups;
}

// Group memories by time period
QMap<QString, QVector<Memory>> MemoryConsolidator::groupMemoriesByTimePeriod(const QVector<Memory> &memories) {
    QMap<QString, QVector<Memory>> timeGroups;
    
    for (const Memory &memory : memories) {
        QString yearMonth = memory.timestamp.toString("yyyy-MM");
        timeGroups[yearMonth].append(memory);
    }
    
    return timeGroups;
}

// Memory journaling functions
MemoryJournal::MemoryJournal(CharacterManager *manager)
    : characterManager(manager) {
}

// Generate a character journal for a specific time period
QString MemoryJournal::generateCharacterJournal(const QString &characterName, 
                                             const QDate &startDate,
                                             const QDate &endDate) {
    QVector<Memory> memories = characterManager->loadMemories(characterName);
    
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

// Format a memory for journal presentation
QString MemoryJournal::formatMemoryForJournal(const Memory &memory) {
    QString formatted = "#### " + memory.title + "\n\n";
    formatted += memory.description + "\n\n";
    
    if (!memory.emotions.isEmpty()) {
        formatted += "*Emotions: " + memory.emotions.join(", ") + "*\n\n";
    }
    
    if (!memory.locations.isEmpty()) {
        formatted += "*Location: " + memory.locations.join(", ") + "*\n\n";
    }
    
    return formatted;
}

// Export journal to file
bool MemoryJournal::exportJournalToFile(const QString &characterName,
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

// Memory search and indexing
MemoryIndex::MemoryIndex(CharacterManager *manager)
    : characterManager(manager) {
}

// Build indexes for faster memory retrieval
void MemoryIndex::buildMemoryIndex(const QString &characterName) {
    QVector<Memory> memories = characterManager->loadMemories(characterName);
    
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
    
    // Save indices
    saveMemoryIndices(characterName, entityIndex, locationIndex);
}

// Save memory indices to file
bool MemoryIndex::saveMemoryIndices(const QString &characterName,
                                 const QMap<QString, QVector<QString>> &entityIndex,
                                 const QMap<QString, QVector<QString>> &locationIndex) {
    // Get the path for the character's memory index directory
    QString indexDir = QDir::homePath() + "/.oobabooga_rpg/characters/" + characterName + "/memories/index";
    QDir dir;
    if (!dir.exists(indexDir)) {
        dir.mkpath(indexDir);
    }
    
    // Save entity index
    QJsonObject entityObj;
    for (auto it = entityIndex.constBegin(); it != entityIndex.constEnd(); ++it) {
        // Convert QVector<QString> to QJsonArray
        QJsonArray jsonArray;
        for (const QString &str : it.value()) {
            jsonArray.append(str);
        }
        entityObj[it.key()] = jsonArray;
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
        // Convert QVector<QString> to QJsonArray
        QJsonArray jsonArray;
        for (const QString &str : it.value()) {
            jsonArray.append(str);
        }
        locationObj[it.key()] = jsonArray;
    }
    
    QFile locationFile(indexDir + "/locations.json");
    if (!locationFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open location index file for writing";
        return false;
    }
    QJsonDocument locationDoc(locationObj);
    locationFile.write(locationDoc.toJson(QJsonDocument::Indented));
    locationFile.close();
    
    return true;
}

// Load memory indices from file
bool MemoryIndex::loadMemoryIndices(const QString &characterName,
                                 QMap<QString, QVector<QString>> &entityIndex,
                                 QMap<QString, QVector<QString>> &locationIndex) {
    // Get the path for the character's memory index directory
    QString indexDir = QDir::homePath() + "/.oobabooga_rpg/characters/" + characterName + "/memories/index";
    
    // Clear existing indices
    entityIndex.clear();
    locationIndex.clear();
    
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
    
    // Return true if at least one index was loaded
    return !entityIndex.isEmpty() || !locationIndex.isEmpty();
}

// Fast memory lookup by entity
QVector<Memory> MemoryIndex::findMemoriesByEntity(const QString &characterName,
                                              const QString &entity) {
    QVector<Memory> result;
    
    // Load indices
    QMap<QString, QVector<QString>> entityIndex;
    QMap<QString, QVector<QString>> locationIndex;
    
    if (!loadMemoryIndices(characterName, entityIndex, locationIndex)) {
        // If indices not available, rebuild them
        buildMemoryIndex(characterName);
        loadMemoryIndices(characterName, entityIndex, locationIndex);
    }
    
    // Find memory IDs for the entity
    QVector<QString> memoryIds;
    QString lowerEntity = entity.toLower();
    
    if (entityIndex.contains(lowerEntity)) {
        memoryIds = entityIndex[lowerEntity];
    }
    
    // Load all memories
    QVector<Memory> allMemories = characterManager->loadMemories(characterName);
    
    // Filter for matching IDs
    for (const Memory &memory : allMemories) {
        if (memoryIds.contains(memory.id)) {
            result.append(memory);
        }
    }
    
    return result;
}

// Fast memory lookup by location
QVector<Memory> MemoryIndex::findMemoriesByLocation(const QString &characterName,
                                                const QString &location) {
    QVector<Memory> result;
    
    // Load indices
    QMap<QString, QVector<QString>> entityIndex;
    QMap<QString, QVector<QString>> locationIndex;
    
    if (!loadMemoryIndices(characterName, entityIndex, locationIndex)) {
        // If indices not available, rebuild them
        buildMemoryIndex(characterName);
        loadMemoryIndices(characterName, entityIndex, locationIndex);
    }
    
    // Find memory IDs for the location
    QVector<QString> memoryIds;
    QString lowerLocation = location.toLower();
    
    if (locationIndex.contains(lowerLocation)) {
        memoryIds = locationIndex[lowerLocation];
    }
    
    // Load all memories
    QVector<Memory> allMemories = characterManager->loadMemories(characterName);
    
    // Filter for matching IDs
    for (const Memory &memory : allMemories) {
        if (memoryIds.contains(memory.id)) {
            result.append(memory);
        }
    }
    
    return result;
}