// include/character/manager/character_manager.h
#ifndef CHARACTER_MANAGER_H
#define CHARACTER_MANAGER_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <QStringList>

#include "../models/memory.h"
#include "../models/personality.h"
#include "../models/stats.h"
#include "../models/appearance.h"

// Class to manage character profiles
class CharacterManager : public QObject {
    Q_OBJECT

public:
    CharacterManager(QObject *parent = nullptr);
    
    // Basic character management
    Q_INVOKABLE QStringList listCharacters();
    Q_INVOKABLE bool createCharacter(const QString &name, const CharacterStats &stats, 
                                  const CharacterPersonality &personality,
                                  const CharacterAppearance &appearance);
    Q_INVOKABLE bool deleteCharacter(const QString &name);
    
    // Character stats
    Q_INVOKABLE CharacterStats loadCharacterStats(const QString &name);
    Q_INVOKABLE bool saveCharacterStats(const QString &name, const CharacterStats &stats);
    
    // Character personality
    Q_INVOKABLE CharacterPersonality loadCharacterPersonality(const QString &name);
    Q_INVOKABLE bool saveCharacterPersonality(const QString &name, const CharacterPersonality &personality);
    
    // Character appearance
    Q_INVOKABLE CharacterAppearance loadCharacterAppearance(const QString &name);
    Q_INVOKABLE bool saveCharacterAppearance(const QString &name, const CharacterAppearance &appearance);
    Q_INVOKABLE bool setCharacterSprite(const QString &name, const QString &spritePath);
    Q_INVOKABLE bool setCharacterCollisionGeometry(const QString &name, 
                                                const CharacterCollisionGeometry &geometry);
    
    // Memory management
    Q_INVOKABLE bool addMemory(const QString &characterName, const Memory &memory);
    Q_INVOKABLE QVector<Memory> loadMemories(const QString &characterName);
    Q_INVOKABLE bool saveMemories(const QString &characterName, const QVector<Memory> &memories);
    Q_INVOKABLE bool updateMemoryRecallInfo(const QString &characterName, const Memory &memory);
    
    // Character profile generation
    Q_INVOKABLE QString generateCharacterProfile(const QString &characterName);
    
    // Memory and character context utilities
    Q_INVOKABLE QStringList getKnownLocations(const QString &characterName);
    Q_INVOKABLE QString generateMemoriesContext(const QString &characterName, 
                                             const QString &currentContext,
                                             const QStringList &currentEntities,
                                             const QStringList &currentLocations,
                                             int maxMemories = 5);
    
    // Process text for possible memory creation
    Q_INVOKABLE void processForMemoryCreation(const QString &userMessage, 
                                            const QString &aiResponse,
                                            const QString &characterName);
                                            
    // Memory relevance scoring - missing methods
    double calculateRelevanceScore(const Memory &memory, 
                                 const QString &currentContext,
                                 const QStringList &currentEntities,
                                 const QStringList &currentLocations);
                                 
    QString determineContextType(const QString &context);
    
    QVector<Memory> retrieveRelevantMemories(
        const QString &characterName, 
        const QString &currentContext, 
        const QStringList &currentEntities,
        const QStringList &currentLocations,
        int maxMemories);

signals:
    void characterAppearanceUpdated(const QString &characterName);
    void characterSpriteChanged(const QString &characterName, const QString &spritePath);
    void characterCollisionGeometryChanged(const QString &characterName, 
                                        const CharacterCollisionGeometry &geometry);

private:
    QString baseDir;
    // Add missing member variable for context type weights
    QMap<QString, QMap<QString, double>> contextTypeWeights;
    
    QString truncateText(const QString &text, int maxLength);
};

#endif // CHARACTER_MANAGER_H