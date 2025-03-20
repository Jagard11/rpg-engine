// src/character/manager/character_manager_memory.cpp
#include "../../../include/character/manager/character_manager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

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

// Add a memory to a character
bool CharacterManager::addMemory(const QString &characterName, const Memory &memory) {
    // Load existing memories
    QVector<Memory> memories = loadMemories(characterName);
    
    // Add new memory
    memories.append(memory);
    
    // Save all memories
    return saveMemories(characterName, memories);
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

// Helper method to truncate text to a maximum length
QString CharacterManager::truncateText(const QString &text, int maxLength) {
    if (text.length() <= maxLength) {
        return text;
    }
    
    return text.left(maxLength) + "...";
}