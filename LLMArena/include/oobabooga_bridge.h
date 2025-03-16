// include/oobabooga_bridge.h
#ifndef OOBABOOGA_BRIDGE_H
#define OOBABOOGA_BRIDGE_H

#include <QObject>
#include <QString>
#include <QMap>

class QNetworkAccessManager;
class QNetworkReply;
class CharacterManager;

// Class to handle communication between C++ and JavaScript with character support
class OobaboogaBridge : public QObject {
    Q_OBJECT

public:
    // Constructor with character manager
    OobaboogaBridge(CharacterManager *charManager, QObject *parent = nullptr);
    
    // Set the Oobabooga API URL
    Q_INVOKABLE void setApiUrl(const QString &url);

    // Get the current API URL
    Q_INVOKABLE QString getApiUrl() const;
    
    // Set the active character
    Q_INVOKABLE void setActiveCharacter(const QString &name);
    
    // Get the active character
    Q_INVOKABLE QString getActiveCharacter() const;

    // Send a message to the Oobabooga API with character context
    Q_INVOKABLE void sendMessageToLLM(const QString &message, const QString &gameContext);

    // Test the API connection
    Q_INVOKABLE void testApiConnection();

    // Save configuration
    Q_INVOKABLE void saveConfig(const QString &apiUrl);

    // Load configuration
    Q_INVOKABLE void loadConfig();
    
    // Add current interaction as a memory for the active character
    Q_INVOKABLE void addMemoryFromInteraction(const QString &userMessage, 
                                            const QString &aiResponse, 
                                            int emotionalIntensity = 5);
    
    // Get a list of available characters
    Q_INVOKABLE QStringList getAvailableCharacters();
    
    // Prepare system prompt with character information
    QString prepareSystemPrompt(const QString &characterName, 
                              const QString &userMessage,
                              const QString &gameContext);
    
    // Optimize system prompt for context window
    QString optimizeForContextWindow(const QString &systemPrompt, int maxTokens = 2048);
    
    // Add consistency guidance for better character responses
    QString addConsistencyGuidance(const QString &systemPrompt, const QString &characterName);
    
    // Process a response for potential memory creation
    void processForMemoryCreation(const QString &userMessage, const QString &aiResponse);

signals:
    // Signal for when an LLM response is received
    void responseReceived(const QString &response);
    
    // Signal for API URL changes
    void apiUrlChanged(const QString &url);
    
    // Signal for errors
    void errorOccurred(const QString &error);
    
    // Signal for status messages
    void statusMessage(const QString &message);
    
    // Signal for active character changed
    void activeCharacterChanged(const QString &characterName);

private slots:
    // Handle network replies
    void handleNetworkReply(QNetworkReply *reply);

private:
    QString apiUrl;  // Formatted URL with http:// protocol
    QString rawApiUrl; // Raw URL as entered by user
    QString activeCharacter;
    QNetworkAccessManager *networkManager;
    QMap<QNetworkReply*, QString> activeReplies;
    CharacterManager *characterManager;
    QString lastMessageContext;
    QString lastResponseText;
    
    // Get recent interactions for consistency
    QVector<QPair<QString, QString>> loadRecentInteractions(const QString &characterName, int count = 5);
    
    // Extract conversation topics for memory relevance
    QStringList extractTopics(const QString &text);
    
    // Determine API endpoint based on prompt size
    QString selectModelEndpoint(const QString &systemPrompt);
};

#endif // OOBABOOGA_BRIDGE_H