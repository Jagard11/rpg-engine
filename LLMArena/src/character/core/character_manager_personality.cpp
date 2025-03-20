// src/character/manager/character_manager_personality.cpp
#include "../../../include/character/manager/character_manager.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

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