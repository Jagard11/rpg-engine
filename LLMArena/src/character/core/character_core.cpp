// src/character/manager/character_manager_core.cpp
#include "../../../include/character/manager/character_manager.h"

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

// Delete a character
bool CharacterManager::deleteCharacter(const QString &name) {
    QDir charDir(baseDir + "/" + name);
    if (!charDir.exists()) {
        qWarning() << "Character directory does not exist:" << name;
        return false;
    }
    
    return charDir.removeRecursively();
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