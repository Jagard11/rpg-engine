// src/character/core/character_manager_context.cpp
#include "../../../include/character/manager/character_manager.h"
#include "../../../include/memory/memory_system.h"

#include <QVector>
#include <QString>
#include <QDebug>
#include <QRandomGenerator>

// Generate a formatted string of relevant memories for the LLM context
QString CharacterManager::generateMemoriesContext(const QString &characterName, 
                                              const QString &currentContext,
                                              const QStringList &currentEntities,
                                              const QStringList &currentLocations,
                                              int maxMemories) {
    // Use class method instead of global function
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
        memory.description = "User said: \"" + userMessage + "\"\n";
        memory.description += "Character responded: \"" + aiResponse + "\"";
        
        // Extract emotions
        for (const QString &keyword : emotionalKeywords) {
            if (aiResponse.contains(keyword, Qt::CaseInsensitive)) {
                memory.emotions.append(keyword);
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