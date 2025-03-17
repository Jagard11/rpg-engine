// src/memory/memory_system.cpp
#include "../include/memory/memory_system.h"
#include "../include/character/character_persistence.h"

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

// Calculate entity overlap for memory relevance
double calculateEntityOverlap(const Memory &memory, 
                            const QStringList &currentEntities,
                            const QString &currentContext) {
    double entityScore = 0.0;
    for (const QString &entity : memory.entities) {
        // Direct entity match
        if (currentEntities.contains(entity, Qt::CaseInsensitive)) {
            entityScore += 1.0;
        }
        
        // Entity mentioned in context
        if (currentContext.contains(entity, Qt::CaseInsensitive)) {
            entityScore += 0.5;
        }
    }
    
    // Normalize
    if (!memory.entities.isEmpty()) {
        entityScore /= memory.entities.size();
    }
    
    return entityScore;
}

// Calculate location overlap for memory relevance
double calculateLocationOverlap(const Memory &memory, 
                               const QStringList &currentLocations,
                               const QString &currentContext) {
    double locationScore = 0.0;
    for (const QString &location : memory.locations) {
        // Direct location match
        if (currentLocations.contains(location, Qt::CaseInsensitive)) {
            locationScore += 1.0;
        }
        
        // Location mentioned in context
        if (currentContext.contains(location, Qt::CaseInsensitive)) {
            locationScore += 0.5;
        }
    }
    
    // Normalize
    if (!memory.locations.isEmpty()) {
        locationScore /= memory.locations.size();
    }
    
    return locationScore;
}

// Calculate keyword/tag overlap for memory relevance
double calculateKeywordOverlap(const Memory &memory, const QString &currentContext) {
    double keywordScore = 0.0;
    for (const QString &tag : memory.tags) {
        if (currentContext.contains(tag, Qt::CaseInsensitive)) {
            keywordScore += 1.0;
        }
    }
    
    // Normalize
    if (!memory.tags.isEmpty()) {
        keywordScore /= memory.tags.size();
    }
    
    return keywordScore;
}

// Calculate recency score - more recent memories are more accessible
double calculateRecencyScore(const Memory &memory) {
    QDateTime now = QDateTime::currentDateTime();
    qint64 daysSinceEvent = memory.timestamp.daysTo(now);
    
    // Recency decay function (hyperbolic)
    if (daysSinceEvent > 0) {
        return 1.0 / (1.0 + log(daysSinceEvent));
    }
    
    return 1.0;
}

// Calculate recall frequency score - frequently recalled memories remain accessible
double calculateRecallFrequencyScore(const Memory &memory) {
    // Cap at 1.0 (10 or more recalls)
    return qMin(1.0, memory.recallCount / 10.0);
}

// Helper function to extract entities from text
QStringList extractEntities(const QString &text) {
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
QStringList extractLocations(const QString &text, const QStringList &knownLocations) {
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

// Calculate emotional intensity based on text content
int calculateEmotionalIntensity(const QString &text) {
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

// Implementation of the relevance scoring from CharacterManager
double CharacterManager::calculateRelevanceScore(const Memory &memory, 
                                              const QString &currentContext,
                                              const QStringList &currentEntities,
                                              const QStringList &currentLocations) {
    // Calculate individual scores
    double entityScore = calculateEntityOverlap(memory, currentEntities, currentContext);
    double locationScore = calculateLocationOverlap(memory, currentLocations, currentContext);
    double keywordScore = calculateKeywordOverlap(memory, currentContext);
    double emotionalScore = memory.emotionalIntensity / 10.0; // Normalize to 0-1
    double recencyScore = calculateRecencyScore(memory);
    double frequencyScore = calculateRecallFrequencyScore(memory);
    
    // Get context type weights (or use defaults)
    double entityWeight = 0.3;
    double locationWeight = 0.2;
    double keywordWeight = 0.15;
    double emotionalWeight = 0.15;
    double recencyWeight = 0.1;
    double frequencyWeight = 0.1;
    
    // Check if we have specific weights for the current context type
    QString contextType = determineContextType(currentContext);
    if (!contextType.isEmpty() && contextTypeWeights.contains(contextType)) {
        const auto &weights = contextTypeWeights[contextType];
        entityWeight = weights.value("entityScore", entityWeight);
        locationWeight = weights.value("locationScore", locationWeight);
        keywordWeight = weights.value("keywordScore", keywordWeight);
        emotionalWeight = weights.value("emotionalScore", emotionalWeight);
        recencyWeight = weights.value("recencyScore", recencyWeight);
        frequencyWeight = weights.value("frequencyScore", frequencyWeight);
    }
    
    // Final weighted score
    double score = (entityScore * entityWeight) +
                  (locationScore * locationWeight) +
                  (keywordScore * keywordWeight) +
                  (emotionalScore * emotionalWeight) +
                  (recencyScore * recencyWeight) +
                  (frequencyScore * frequencyWeight);
    
    return score;
}

// Determine the type of context (emotional, combat, exploration, etc.)
QString CharacterManager::determineContextType(const QString &context) {
    // Lists of keywords for different context types
    QStringList combatKeywords = {"attack", "fight", "battle", "defend", "weapon", "enemy", "hit", "damage"};
    QStringList emotionalKeywords = {"feel", "happy", "sad", "angry", "afraid", "love", "hate", "worry"};
    QStringList explorationKeywords = {"explore", "discover", "find", "search", "path", "journey", "map"};
    QStringList socialKeywords = {"talk", "speak", "conversation", "friend", "ally", "enemy", "relationship"};
    
    // Count keyword matches for each type
    int combatCount = 0;
    int emotionalCount = 0;
    int explorationCount = 0;
    int socialCount = 0;
    
    for (const QString &keyword : combatKeywords) {
        if (context.contains(keyword, Qt::CaseInsensitive)) {
            combatCount++;
        }
    }
    
    for (const QString &keyword : emotionalKeywords) {
        if (context.contains(keyword, Qt::CaseInsensitive)) {
            emotionalCount++;
        }
    }
    
    for (const QString &keyword : explorationKeywords) {
        if (context.contains(keyword, Qt::CaseInsensitive)) {
            explorationCount++;
        }
    }
    
    for (const QString &keyword : socialKeywords) {
        if (context.contains(keyword, Qt::CaseInsensitive)) {
            socialCount++;
        }
    }
    
    // Determine the dominant type
    int maxCount = qMax(qMax(combatCount, emotionalCount), qMax(explorationCount, socialCount));
    
    if (maxCount > 0) {
        if (maxCount == combatCount) {
            return "combat";
        } else if (maxCount == emotionalCount) {
            return "emotional";
        } else if (maxCount == explorationCount) {
            return "exploration";
        } else if (maxCount == socialCount) {
            return "social";
        }
    }
    
    // Default to general if no clear type
    return "general";
}

// Retrieve memories relevant to the current context
QVector<Memory> CharacterManager::retrieveRelevantMemories(
        const QString &characterName, 
        const QString &currentContext, 
        const QStringList &currentEntities,
        const QStringList &currentLocations,
        int maxMemories) {
    
    // Load all memories
    QVector<Memory> allMemories = loadMemories(characterName);
    if (allMemories.isEmpty()) {
        return QVector<Memory>();
    }
    
    // Calculate relevance score for each memory
    QVector<QPair<double, Memory>> scoredMemories;
    
    for (const Memory &memory : allMemories) {
        double score = calculateRelevanceScore(
            memory, 
            currentContext, 
            currentEntities, 
            currentLocations
        );
        
        scoredMemories.append(qMakePair(score, memory));
    }
    
    // Sort by score (highest first)
    std::sort(scoredMemories.begin(), scoredMemories.end(), 
        [](const QPair<double, Memory> &a, const QPair<double, Memory> &b) {
            return a.first > b.first;
        }
    );
    
    // Update recall information for retrieved memories
    QVector<Memory> relevantMemories;
    for (int i = 0; i < qMin(maxMemories, scoredMemories.size()); ++i) {
        Memory memory = scoredMemories[i].second;
        
        // Only include memories that have a minimum relevance
        if (scoredMemories[i].first < 0.1) {
            break;
        }
        
        // Update recall info
        memory.lastRecalled = QDateTime::currentDateTime();
        memory.recallCount++;
        
        // Add to relevant memories
        relevantMemories.append(memory);
        
        // Update memory in storage
        updateMemoryRecallInfo(characterName, memory);
    }
    
    return relevantMemories;
}

// Generate a formatted string of relevant memories for the LLM context
QString CharacterManager::generateMemoriesContext(const QString &characterName, 
                                              const QString &currentContext,
                                              const QStringList &currentEntities,
                                              const QStringList &currentLocations,
                                              int maxMemories) {
    QVector<Memory> relevantMemories = retrieveRelevantMemories(
        characterName, 
        currentContext, 
        currentEntities, 
        currentLocations, 
        maxMemories
    );
    
    if (relevantMemories.isEmpty()) {
        return "";
    }
    
    QString context = "CHARACTER MEMORIES:\n";
    
    for (const Memory &memory : relevantMemories) {
        context += QString("- %1 (%2): %3\n")
            .arg(memory.title)
            .arg(memory.timestamp.toString("yyyy-MM-dd"))
            .arg(memory.description);
    }
    
    return context;
}

// Process text for possible memory creation
void CharacterManager::processForMemoryCreation(const QString &userMessage, 
                                             const QString &aiResponse,
                                             const QString &characterName) {
    // Skip trivial exchanges
    if (userMessage.length() < 10 || aiResponse.length() < 20) {
        return;
    }
    
    // Detect potentially memorable content
    bool isSignificant = false;
    
    // Check for emotional content using keywords
    QStringList emotionalKeywords = {"love", "hate", "afraid", "excited", "worried", "happy", "sad", "angry"};
    for (const QString &keyword : emotionalKeywords) {
        if (aiResponse.contains(keyword, Qt::CaseInsensitive)) {
            isSignificant = true;
            break;
        }
    }
    
    // Check for significant events
    QStringList eventKeywords = {"never forget", "remember", "first time", "important", "significant"};
    for (const QString &keyword : eventKeywords) {
        if (aiResponse.contains(keyword, Qt::CaseInsensitive)) {
            isSignificant = true;
            break;
        }
    }
    
    // Create memory if significant
    if (isSignificant) {
        Memory memory;
        memory.id = QDateTime::currentDateTime().toString("yyyyMMddhhmmss") + 
                  QString::number(QRandomGenerator::global()->bounded(1000));
        memory.timestamp = QDateTime::currentDateTime();
        memory.type = "conversation";
        memory.title = "Significant Exchange: " + truncateText(userMessage, 30);
        memory.description = "User said: \"" + userMessage + "\"\nCharacter: " + aiResponse;
        
        // Extract emotions
        for (const QString &emotion : emotionalKeywords) {
            if (aiResponse.contains(emotion, Qt::CaseInsensitive)) {
                memory.emotions.append(emotion);
            }
        }
        
        // Set emotional intensity based on language intensity
        memory.emotionalIntensity = calculateEmotionalIntensity(aiResponse);
        
        // Extract entities and locations
        memory.entities = extractEntities(aiResponse);
        memory.entities.append(extractEntities(userMessage));
        
        // Use known locations or try to extract from context
        QStringList knownLocations = getKnownLocations(characterName);
        memory.locations = extractLocations(aiResponse + " " + userMessage, knownLocations);
        
        // Add memory
        addMemory(characterName, memory);
    }
}

// Truncate text to a maximum length
QString CharacterManager::truncateText(const QString &text, int maxLength) {
    if (text.length() <= maxLength) {
        return text;
    }
    
    return text.left(maxLength) + "...";
}

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