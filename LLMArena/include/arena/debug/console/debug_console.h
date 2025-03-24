// include/arena/debug/console/debug_console.h
#ifndef DEBUG_CONSOLE_H
#define DEBUG_CONSOLE_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QVector>
#include <QFont>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <memory>

// Forward declarations
class DebugCommand;
class GameScene;
class PlayerController;
class QPaintDevice;

/**
 * @brief Debug console for in-game commands and debugging
 * 
 * This class manages the debug console UI and command execution.
 * It allows toggling the console with the backtick (`) key and
 * executing commands for debugging purposes.
 */
class DebugConsole : public QObject, protected QOpenGLFunctions {
    Q_OBJECT
    
public:
    explicit DebugConsole(GameScene* scene, PlayerController* player, QObject* parent = nullptr);
    ~DebugConsole();
    
    /**
     * @brief Initialize the console
     */
    void initialize();
    
    /**
     * @brief Render the console
     * @param screenWidth Width of the screen
     * @param screenHeight Height of the screen
     */
    void render(int screenWidth, int screenHeight);
    
    /**
     * @brief Handle keyboard input
     * @param key Key code
     * @param text Text of the key
     * @return True if the input was handled
     */
    bool handleKeyPress(int key, const QString& text);
    
    /**
     * @brief Register a command with the console
     * @param command Command to register
     */
    void registerCommand(DebugCommand* command);
    
    /**
     * @brief Check if the console is visible
     * @return True if visible
     */
    bool isVisible() const { return m_visible; }
    
    /**
     * @brief Set console visibility
     * @param visible Visibility state
     */
    void setVisible(bool visible);
    
    /**
     * @brief Get a reference to a command by name
     * @param name Command name
     * @return Pointer to command or nullptr if not found
     */
    DebugCommand* getCommand(const QString& name) const;
    
signals:
    /**
     * @brief Signal emitted when console visibility changes
     * @param visible New visibility state
     */
    void visibilityChanged(bool visible);
    
    /**
     * @brief Signal emitted when a command is executed
     * @param command Command name
     * @param result Result message
     */
    void commandExecuted(const QString& command, const QString& result);
    
private:
    // Game references
    GameScene* m_gameScene;
    PlayerController* m_playerController;
    
    // Console state
    bool m_visible;
    QString m_inputText;
    QVector<QString> m_outputLines;
    QVector<QString> m_commandHistory;
    int m_historyIndex;
    
    // Command registry
    QMap<QString, DebugCommand*> m_commands;
    
    // OpenGL rendering resources
    QOpenGLShaderProgram* m_consoleShader;
    QOpenGLBuffer m_quadVBO;
    QOpenGLVertexArrayObject m_quadVAO;
    
    // UI properties
    QFont m_consoleFont;
    int m_maxOutputLines;
    float m_consoleHeight;
    float m_consoleOpacity;
    
    // Execute a command
    void executeCommand(const QString& commandText);
    
    // Add output to the console
    void addOutput(const QString& text);
    
    // Draw text at position
    void drawText(const QString& text, float x, float y, const QColor& color);
    
    // Initialize OpenGL resources
    void initializeGL();
    
    // Create and initialize shader program
    void createShaders();
    
    // Create quad geometry for console background
    void createQuadGeometry();
    
    // Helper method to draw console text with QPainter
    void drawConsoleText(int screenWidth, int screenHeight, float consoleHeight);
};

#endif // DEBUG_CONSOLE_H