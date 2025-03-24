// include/arena/debug/debug_system.h
#ifndef DEBUG_SYSTEM_H
#define DEBUG_SYSTEM_H

#include <QObject>
#include <memory>
#include <QVariant>
#include <QMatrix4x4>

// Forward declarations
class GameScene;
class PlayerController;
class DebugConsole;
class LocationCommand;
class TeleportCommand;
class FrustumCullCommand;
class FrustumVisualizer;

/**
 * @brief Main debug system that integrates all debug components
 * 
 * This class manages all debug components including the console,
 * commands, and visualizers. It provides a central point for
 * initializing and accessing the debug functionality.
 */
class DebugSystem : public QObject {
    Q_OBJECT
    
public:
    /**
     * @brief Constructor - creates debug system components
     * @param scene Game scene reference
     * @param player Player controller reference
     * @param parent Parent QObject
     */
    explicit DebugSystem(GameScene* scene, PlayerController* player, QObject* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~DebugSystem() override;
    
    /**
     * @brief Initialize the debug system components
     */
    void initialize();
    
    /**
     * @brief Render debug visuals
     * @param viewMatrix Camera view matrix
     * @param projectionMatrix Camera projection matrix
     * @param screenWidth Screen width
     * @param screenHeight Screen height
     */
    void render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix,
                int screenWidth, int screenHeight);
    
    /**
     * @brief Handle key press events
     * @param key Key code
     * @param text Text of the key
     * @return True if the input was handled
     */
    bool handleKeyPress(int key, const QString& text);
    
    /**
     * @brief Check if the console is visible
     * @return True if visible
     */
    bool isConsoleVisible() const;
    
    /**
     * @brief Toggle console visibility
     */
    void toggleConsoleVisibility();
    
    /**
     * @brief Toggle frustum visualization
     */
    void toggleFrustumVisualization();
    
    /**
     * @brief Set the widget for console rendering
     * @param widget Widget pointer as QVariant
     */
    void setConsoleWidget(const QVariant& widget);
    
private:
    // Game references
    GameScene* m_gameScene;
    PlayerController* m_playerController;
    
    // Debug components
    std::unique_ptr<DebugConsole> m_console;
    std::unique_ptr<FrustumVisualizer> m_frustumVisualizer;
    
    // Debug commands
    std::unique_ptr<LocationCommand> m_locationCommand;
    std::unique_ptr<TeleportCommand> m_teleportCommand;
    std::unique_ptr<FrustumCullCommand> m_frustumCullCommand;
    
    // Register all commands
    void registerCommands();
};

#endif // DEBUG_SYSTEM_H