// include/character/memory/memory_system.h
#ifndef MEMORY_SYSTEM_H
#define MEMORY_SYSTEM_H

#include "character_persistence.h"

// Forward declarations
class QJsonObject;
class QDateTime;
class QString;
class QStringList;

// Calculate entity overlap for memory relevance
double calculateEntityOverlap(const Memory &memory, 
                            const QStringList &currentEntities,
                            const QString &currentContext);

// Calculate location overlap for memory relevance
double calculateLocationOverlap(const Memory &memory, 
                               const QStringList &currentLocations,
                               const QString &currentContext);

// Calculate keyword/tag overlap for memory relevance
double calculateKeywordOverlap(const Memory &memory, const QString &currentContext);

// Calculate recency score - more recent memories are more accessible
double calculateRecencyScore(const Memory &memory);

// Calculate recall frequency score - frequently recalled memories remain accessible
double calculateRecallFrequencyScore(const Memory &memory);

// Helper function to extract entities from text
QStringList extractEntities(const QString &text);

// Helper function to extract locations from text
QStringList extractLocations(const QString &text, const QStringList &knownLocations);

// Calculate emotional intensity based on text content
int calculateEmotionalIntensity(const QString &text);

// Memory consolidation functions
class MemoryConsolidator {
public:
    MemoryConsolidator(CharacterManager *manager);
    
    // Consolidate similar memories into summary memories
    bool consolidateMemories(const QString &characterName);
    
    // Generate a summary description from a set of related memories
    QString generateConsolidatedDescription(const QVector<Memory> &memories);
    
    // Group memories by related entities
    QMap<QString, QVector<Memory>> groupMemoriesByEntity(const QVector<Memory> &memories);
    
    // Group memories by location
    QMap<QString, QVector<Memory>> groupMemoriesByLocation(const QVector<Memory> &memories);
    
    // Group memories by time period
    QMap<QString, QVector<Memory>> groupMemoriesByTimePeriod(const QVector<Memory> &memories);
    
private:
    CharacterManager *characterManager;
};

// Memory journaling functions
class MemoryJournal {
public:
    MemoryJournal(CharacterManager *manager);
    
    // Generate a character journal for a specific time period
    QString generateCharacterJournal(const QString &characterName, 
                                   const QDate &startDate,
                                   const QDate &endDate);
    
    // Format a memory for journal presentation
    QString formatMemoryForJournal(const Memory &memory);
    
    // Export journal to file
    bool exportJournalToFile(const QString &characterName,
                           const QString &journal,
                           const QString &filePath);
    
private:
    CharacterManager *characterManager;
};

// Memory search and indexing
class MemoryIndex {
public:
    MemoryIndex(CharacterManager *manager);
    
    // Build indexes for faster memory retrieval
    void buildMemoryIndex(const QString &characterName);
    
    // Save memory indices to file
    bool saveMemoryIndices(const QString &characterName,
                         const QMap<QString, QVector<QString>> &entityIndex,
                         const QMap<QString, QVector<QString>> &locationIndex);
    
    // Load memory indices from file
    bool loadMemoryIndices(const QString &characterName,
                         QMap<QString, QVector<QString>> &entityIndex,
                         QMap<QString, QVector<QString>> &locationIndex);
    
    // Fast memory lookup by entity
    QVector<Memory> findMemoriesByEntity(const QString &characterName,
                                       const QString &entity);
    
    // Fast memory lookup by location
    QVector<Memory> findMemoriesByLocation(const QString &characterName,
                                         const QString &location);
    
private:
    CharacterManager *characterManager;
};

#endif // MEMORY_SYSTEM_H