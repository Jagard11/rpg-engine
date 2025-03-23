// src/llm/api/api_caller.cpp
#include "../../../include/llm/oobabooga_bridge.h"
#include "../../../include/character/core/character_persistence.h"
#include "../../../include/character/memory/memory_system.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QDebug>
#include <QRegExp>
#include <QFile>
#include <QDir>
#include <QRandomGenerator> // Add missing include

// Constructor
OobaboogaBridge::OobaboogaBridge(CharacterManager *charManager, QObject *parent) 
    : QObject(parent), characterManager(charManager) {
    // Initialize network manager for API calls
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &OobaboogaBridge::handleNetworkReply);
}

// Set the Oobabooga API URL
void OobaboogaBridge::setApiUrl(const QString &url) {
    // Store the raw URL (without http://) for display purposes
    rawApiUrl = url.trimmed();
    
    // Create a properly formatted URL with http:// protocol for actual requests
    // Remove any existing protocol
    apiUrl = rawApiUrl;
    if (apiUrl.startsWith("http://")) {
        apiUrl.remove(0, 7);
    } else if (apiUrl.startsWith("https://")) {
        apiUrl.remove(0, 8);
    }
    
    // Always use http:// protocol for actual requests
    apiUrl = "http://" + apiUrl;
    
    qDebug() << "Raw API URL: " << rawApiUrl;
    qDebug() << "Formatted API URL for requests: " << apiUrl;
    
    emit apiUrlChanged(rawApiUrl); // Emit the raw URL for display
}

// Get the current API URL
QString OobaboogaBridge::getApiUrl() const {
    return rawApiUrl; // Return the raw URL for display in UI
}

// Set the active character
void OobaboogaBridge::setActiveCharacter(const QString &name) {
    activeCharacter = name;
    emit activeCharacterChanged(name);
}

// Get the active character
QString OobaboogaBridge::getActiveCharacter() const {
    return activeCharacter;
}

// Test the API connection using the /v1/models endpoint
void OobaboogaBridge::testApiConnection() {
    if (apiUrl.isEmpty()) {
        emit errorOccurred("API URL is not set");
        return;
    }

    // Use the /v1/models endpoint which returns a list of available models
    // This is a standard OpenAI API endpoint that OobaboogaAPI supports
    QString endpoint = apiUrl + "/v1/models";
    qDebug() << "Testing connection to URL:" << endpoint;
    
    // Create a QUrl object with the formatted URL
    QUrl url(endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // Send GET request
    QNetworkReply *reply = networkManager->get(request);
    activeReplies[reply] = "test";
}

// Send a message to the Oobabooga API with character context using the OpenAI API format
void OobaboogaBridge::sendMessageToLLM(const QString &message, const QString &gameContext) {
    if (apiUrl.isEmpty()) {
        emit errorOccurred("API URL is not set");
        return;
    }
    
    // Prepare system prompt with character information
    QString systemPrompt;
    
    if (!activeCharacter.isEmpty()) {
        systemPrompt = prepareSystemPrompt(activeCharacter, message, gameContext);
        systemPrompt = optimizeForContextWindow(systemPrompt);
        systemPrompt = addConsistencyGuidance(systemPrompt, activeCharacter);
    } else {
        // Basic system prompt if no character is selected
        systemPrompt = "You are a helpful AI assistant in an RPG game.\n";
        
        // Add game context if available
        if (!gameContext.isEmpty()) {
            systemPrompt += "\nGAME CONTEXT:\n" + gameContext + "\n";
        }
    }
    
    // Create the request body using the OpenAI chat completion format
    QJsonObject jsonObject;
    
    // Create the messages array
    QJsonArray messagesArray;
    
    // Add system message
    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = systemPrompt;
    messagesArray.append(systemMessage);
    
    // Add user message
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = message;
    messagesArray.append(userMessage);
    
    // Add messages array to request
    jsonObject["messages"] = messagesArray;
    
    // Add other parameters
    jsonObject["model"] = "default"; // Use whatever model is currently loaded
    jsonObject["temperature"] = 0.7;
    jsonObject["max_tokens"] = 500;
    jsonObject["top_p"] = 0.9;
    
    // For character mode
    if (!activeCharacter.isEmpty()) {
        jsonObject["mode"] = "chat";
        jsonObject["character"] = activeCharacter;
    } else {
        jsonObject["mode"] = "instruct"; // Default to instruct mode when not using a character
    }
    
    QJsonDocument doc(jsonObject);
    QByteArray data = doc.toJson();

    // Use the /v1/chat/completions endpoint for chat
    QString endpoint = apiUrl + "/v1/chat/completions";
    qDebug() << "Sending request to:" << endpoint;
    qDebug() << "Request data:" << data;
    
    // Create request with the API endpoint
    QUrl url(endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Send POST request
    QNetworkReply *reply = networkManager->post(request, data);
    activeReplies[reply] = "generate";
    
    // Store last message for memory creation
    lastMessageContext = message;
}

// Save configuration
void OobaboogaBridge::saveConfig(const QString &apiUrl) {
    QSettings settings("OobaboogaRPG", "ArenaApp");
    settings.setValue("apiUrl", apiUrl);
    settings.setValue("lastCharacter", activeCharacter);
    settings.sync();
}

// Load configuration
void OobaboogaBridge::loadConfig() {
    QSettings settings("OobaboogaRPG", "ArenaApp");
    rawApiUrl = settings.value("apiUrl", "").toString();
    activeCharacter = settings.value("lastCharacter", "").toString();
    
    // If we have a saved URL, format it properly
    if (!rawApiUrl.isEmpty()) {
        setApiUrl(rawApiUrl);
    }
}

// Add current interaction as a memory for the active character
void OobaboogaBridge::addMemoryFromInteraction(const QString &userMessage, const QString &aiResponse, 
                                            int emotionalIntensity) {
    if (activeCharacter.isEmpty()) {
        emit errorOccurred("No active character selected");
        return;
    }
    
    // Create a new memory from the interaction
    Memory memory;
    memory.id = QDateTime::currentDateTime().toString("yyyyMMddhhmmss") + 
              QString::number(QRandomGenerator::global()->bounded(1000));
    memory.timestamp = QDateTime::currentDateTime();
    memory.type = "conversation";
    
    // Create a title from the first few words of the user message
    QString shortMessage = userMessage;
    if (shortMessage.length() > 30) {
        shortMessage = shortMessage.left(30) + "...";
    }
    memory.title = "Conversation: " + shortMessage;
    
    // Create description from both messages
    memory.description = "User said: \"" + userMessage + "\"\n";
    memory.description += "Character responded: \"" + aiResponse + "\"";
    
    memory.emotionalIntensity = emotionalIntensity;
    
    // Extract entities and emotions
    memory.entities = extractEntities(userMessage);
    memory.entities.append(extractEntities(aiResponse));
    
    // Extract potential emotions
    QStringList emotionalKeywords = {"love", "hate", "afraid", "excited", "worried", "happy", "sad", "angry"};
    for (const QString &keyword : emotionalKeywords) {
        if (aiResponse.contains(keyword, Qt::CaseInsensitive)) {
            memory.emotions.append(keyword);
        }
    }
    
    // Extract locations
    QStringList knownLocations = characterManager->getKnownLocations(activeCharacter);
    memory.locations = extractLocations(aiResponse + " " + userMessage, knownLocations);
    
    // Save the memory
    characterManager->addMemory(activeCharacter, memory);
    
    emit statusMessage("Memory added to " + activeCharacter);
}

// Get a list of available characters
QStringList OobaboogaBridge::getAvailableCharacters() {
    return characterManager->listCharacters();
}

// Handle network replies
void OobaboogaBridge::handleNetworkReply(QNetworkReply *reply) {
    // Get the reply type from the active replies map
    QString replyType = activeReplies.value(reply, "");
    
    // Debug information
    qDebug() << "Network reply received for type:" << replyType;
    qDebug() << "Error:" << reply->error();
    qDebug() << "Error string:" << reply->errorString();
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        qDebug() << "Response data (first 200 chars):" << data.left(200);
        
        if (replyType == "generate") {
            // Parse JSON response
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();
                
                // Handle OpenAI API response format for chat completions
                if (obj.contains("choices") && obj["choices"].isArray()) {
                    QJsonArray choices = obj["choices"].toArray();
                    if (choices.size() > 0 && choices[0].isObject()) {
                        QJsonObject choice = choices[0].toObject();
                        if (choice.contains("message") && choice["message"].isObject()) {
                            QString responseText = choice["message"].toObject()["content"].toString();
                            
                            // Clean up the response
                            responseText = responseText.trimmed();
                            
                            // Store response for potential memory creation
                            lastResponseText = responseText;
                            
                            // Process for automatic memory creation if active character exists
                            if (!activeCharacter.isEmpty()) {
                                processForMemoryCreation(lastMessageContext, responseText);
                            }
                            
                            emit responseReceived(responseText);
                        } else {
                            emit errorOccurred("Invalid response format: missing message content");
                        }
                    } else {
                        emit errorOccurred("Invalid response format: choices array empty or invalid");
                    }
                } else {
                    emit errorOccurred("Invalid response format: missing choices array. Response: " + 
                                      QString(data.left(200)) + "...");
                }
            } else {
                emit errorOccurred("Failed to parse API response: not valid JSON");
            }
        } else if (replyType == "test") {
            // For test connection, try to parse the models list response
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isNull() && doc.isObject() && doc.object().contains("data")) {
                // Successful models list API response
                emit statusMessage("API connection successful! Server running at " + rawApiUrl);
            } else {
                // Got a response but not in the expected format
                emit statusMessage("Connected to server at " + rawApiUrl + ", but unexpected response format.");
            }
        }
    } else {
        // Handle error conditions
        if (replyType == "test") {
            // Special handling for test connection
            emit errorOccurred("Connection error: " + reply->errorString() + 
                              "\n\nTry these formats:\n" +
                              "- 0.0.0.0:5000 (recommended)\n" +
                              "- 127.0.0.1:5000\n" +
                              "- localhost:5000");
        } else {
            emit errorOccurred("Network error: " + reply->errorString());
        }
    }
    
    activeReplies.remove(reply);
    reply->deleteLater();
}

// Prepare system prompt with character information
QString OobaboogaBridge::prepareSystemPrompt(const QString &characterName, 
                                          const QString &userMessage,
                                          const QString &gameContext) {
    // Get character profile
    QString characterProfile = characterManager->generateCharacterProfile(characterName);
    
    // Extract entities and locations from game context and user message
    QStringList currentEntities = extractEntities(gameContext + " " + userMessage);
    
    // Get known locations from character manager
    QStringList knownLocations = characterManager->getKnownLocations(characterName);
    QStringList currentLocations = extractLocations(gameContext + " " + userMessage, knownLocations);
    
    // Get relevant memories
    QString memories = characterManager->generateMemoriesContext(
        characterName, userMessage, currentEntities, currentLocations, 5);
    
    // Construct system prompt
    QString systemPrompt = "You are roleplaying as the following character.\n\n";
    systemPrompt += characterProfile;
    
    if (!memories.isEmpty()) {
        systemPrompt += "\n" + memories;
    }
    
    systemPrompt += "\nYou must stay in character at all times and respond as this character would.\n";
    
    // Add game context if available
    if (!gameContext.isEmpty()) {
        systemPrompt += "\nGAME CONTEXT:\n" + gameContext + "\n";
    }
    
    // Add roleplay instructions
    systemPrompt += "\nROLEPLAY INSTRUCTIONS:\n";
    systemPrompt += "1. Respond in first person as the character\n";
    systemPrompt += "2. Express emotions and reactions consistent with the character's personality\n";
    systemPrompt += "3. Reference relevant memories when appropriate\n";
    systemPrompt += "4. Be consistent with past interactions\n";
    systemPrompt += "5. Don't break the fourth wall or discuss that you are an AI\n";
    
    return systemPrompt;
}

// Optimize system prompt for context window
QString OobaboogaBridge::optimizeForContextWindow(const QString &systemPrompt, int maxTokens) {
    // Estimate current token count (rough approximation)
    int estimatedTokens = systemPrompt.split(QRegExp("\\s+")).size() * 1.3;
    
    if (estimatedTokens <= maxTokens) {
        return systemPrompt;
    }
    
    // Need to reduce size - start with least important parts
    QString optimizedPrompt = systemPrompt;
    
    // 1. Reduce number of memories
    if (optimizedPrompt.contains("CHARACTER MEMORIES:")) {
        // Extract and reduce memories section
        QRegExp memoriesRegex("CHARACTER MEMORIES:\\n(.*?)\\n\\n");
        memoriesRegex.setMinimal(true);  // Make it non-greedy
        if (memoriesRegex.indexIn(optimizedPrompt) != -1) {
            QString memoriesSection = memoriesRegex.cap(1);
            QStringList memories = memoriesSection.split("\n- ");
            
            // Keep only the 3 most important memories
            if (memories.size() > 3) {
                QString reducedMemories = "CHARACTER MEMORIES:\n- " + 
                    memories.mid(0, 3).join("\n- ") + "\n\n";
                optimizedPrompt.replace(memoriesRegex, reducedMemories);
            }
        }
    }
    
    // 2. Reduce background information if still too large
    estimatedTokens = optimizedPrompt.split(QRegExp("\\s+")).size() * 1.3;
    if (estimatedTokens > maxTokens && optimizedPrompt.contains("BACKGROUND:")) {
        QRegExp backgroundRegex("BACKGROUND:\\n(.*?)\\n\\n");
        backgroundRegex.setMinimal(true);  // Make it non-greedy
        if (backgroundRegex.indexIn(optimizedPrompt) != -1) {
            QString background = backgroundRegex.cap(1);
            
            // Truncate background to about half its size
            int halfLength = background.length() / 2;
            QString truncatedBackground = background.left(halfLength) + "...";
            
            QString reducedBackground = "BACKGROUND:\n" + truncatedBackground + "\n\n";
            optimizedPrompt.replace(backgroundRegex, reducedBackground);
        }
    }
    
    // 3. Simplify description
    estimatedTokens = optimizedPrompt.split(QRegExp("\\s+")).size() * 1.3;
    if (estimatedTokens > maxTokens && optimizedPrompt.contains("General Description:")) {
        QRegExp descRegex("General Description: (.*?)\\n");
        descRegex.setMinimal(true);  // Make it non-greedy
        if (descRegex.indexIn(optimizedPrompt) != -1) {
            QString fullDesc = descRegex.cap(1);
            
            // Truncate description
            if (fullDesc.length() > 100) {
                QString shortDesc = fullDesc.left(100) + "...";
                optimizedPrompt.replace(descRegex, "General Description: " + shortDesc + "\n");
            }
        }
    }
    
    return optimizedPrompt;
}

// Add consistency guidance for better character responses
QString OobaboogaBridge::addConsistencyGuidance(const QString &systemPrompt, const QString &characterName) {
    // Load recent interactions for consistency checks
    QVector<QPair<QString, QString>> recentInteractions = loadRecentInteractions(characterName, 3);
    
    if (recentInteractions.isEmpty()) {
        return systemPrompt;
    }
    
    QString enhancedPrompt = systemPrompt;
    enhancedPrompt += "\n\nRECENT INTERACTIONS FOR CONSISTENCY:\n";
    
    for (const auto &interaction : recentInteractions) {
        enhancedPrompt += "User: " + interaction.first + "\n";
        enhancedPrompt += "You: " + interaction.second + "\n\n";
    }
    
    enhancedPrompt += "Maintain consistent tone, vocabulary, and personality with these previous responses.\n";
    
    return enhancedPrompt;
}

// Get recent interactions for consistency
QVector<QPair<QString, QString>> OobaboogaBridge::loadRecentInteractions(const QString &characterName, int count) {
    QVector<QPair<QString, QString>> interactions;
    
    // Load recent memories of type "conversation"
    QVector<Memory> memories = characterManager->loadMemories(characterName);
    
    // Filter conversation memories
    QVector<Memory> conversations;
    for (const Memory &memory : memories) {
        if (memory.type == "conversation") {
            conversations.append(memory);
        }
    }
    
    // Sort by timestamp (most recent first)
    std::sort(conversations.begin(), conversations.end(), 
              [](const Memory &a, const Memory &b) {
                  return a.timestamp > b.timestamp;
              });
    
    // Extract user/AI pairs from conversation descriptions
    for (int i = 0; i < qMin(count, conversations.size()); ++i) {
        const Memory &memory = conversations[i];
        
        // Extract user message
        QRegExp userRegex("User said: \"(.*?)\"");
        userRegex.setMinimal(true);  // Make it non-greedy
        QString userMessage;
        if (userRegex.indexIn(memory.description) != -1) {
            userMessage = userRegex.cap(1);
        }
        
        // Extract character response
        QRegExp charRegex("Character responded: \"(.*?)\"");
        charRegex.setMinimal(true);  // Make it non-greedy
        QString characterResponse;
        if (charRegex.indexIn(memory.description) != -1) {
            characterResponse = charRegex.cap(1);
        }
        
        if (!userMessage.isEmpty() && !characterResponse.isEmpty()) {
            interactions.append(qMakePair(userMessage, characterResponse));
        }
    }
    
    return interactions;
}

// Extract conversation topics for memory relevance
QStringList OobaboogaBridge::extractTopics(const QString &text) {
    QStringList topics;
    
    // Extract nouns that might be topics (very simplified approach)
    QStringList words = text.split(QRegExp("\\s+"));
    
    for (const QString &word : words) {
        // Check if word starts with capital letter (might be a proper noun)
        if (!word.isEmpty() && word[0].isUpper() && word.length() > 3) {
            topics.append(word);
        }
        
        // Check for lowercase nouns (simplified)
        if (word.length() > 4 && !word.contains(QRegExp("[.,;:!?]$"))) {
            // Skip common stop words
            QStringList stopWords = {"about", "above", "after", "again", "against", 
                                   "these", "those", "their", "there", "would"};
            if (!stopWords.contains(word.toLower())) {
                topics.append(word.toLower());
            }
        }
    }
    
    // Remove duplicates
    QSet<QString> uniqueTopics(topics.begin(), topics.end());
    return uniqueTopics.values();
}

// Determine API endpoint based on prompt size
QString OobaboogaBridge::selectModelEndpoint(const QString &systemPrompt) {
    // Always use the OpenAI-compatible chat completions endpoint
    return "/v1/chat/completions";
}

// Process a response for potential memory creation
void OobaboogaBridge::processForMemoryCreation(const QString &userMessage, const QString &aiResponse) {
    if (activeCharacter.isEmpty()) return;
    
    characterManager->processForMemoryCreation(userMessage, aiResponse, activeCharacter);
}