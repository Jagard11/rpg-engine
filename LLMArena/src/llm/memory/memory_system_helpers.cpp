// src/llm/memory/memory_system_helpers.cpp
// This file contains only the helper functions needed to resolve linking errors
#include "../include/character/memory/memory_system.h"
#include "../include/character/core/character_persistence.h"

#include <QRegExp>
#include <QSet>
#include <cmath>

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

// Calculate relevance score for memory matching
double CharacterManager::calculateRelevanceScore(const Memory &memory, 
                                             const QString &currentContext,
                                             const QStringList &currentEntities,
                                             const QStringList &currentLocations) {
    // Simplified scoring using direct comparison
    double score = 0.0;
    
    // Entity matching (30%)
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
    
    // Location matching (20%)
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
    
    // Emotional intensity (15%)
    double emotionalScore = memory.emotionalIntensity / 10.0; // Normalize to 0-1
    
    // Recency (20%)
    double recencyScore = 0.0;
    QDateTime now = QDateTime::currentDateTime();
    qint64 daysSinceEvent = memory.timestamp.daysTo(now);
    // Recency decay function
    if (daysSinceEvent > 0) {
        recencyScore = 1.0 / (1.0 + log(daysSinceEvent));
    } else {
        recencyScore = 1.0;
    }
    
    // Recall frequency (15%)
    double frequencyScore = qMin(1.0, memory.recallCount / 10.0);
    
    // Final weighted score
    score = (entityScore * 0.3) +
           (locationScore * 0.2) +
           (emotionalScore * 0.15) +
           (recencyScore * 0.2) +
           (frequencyScore * 0.15);
    
    return score;
}

// Implementation of the missing CharacterManager memory retrieval method
QVector<Memory> CharacterManager::retrieveRelevantMemories(
    const QString &characterName, 
    const QString &currentContext, 
    const QStringList &currentEntities,
    const QStringList &currentLocations,
    int maxMemories) 
{
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