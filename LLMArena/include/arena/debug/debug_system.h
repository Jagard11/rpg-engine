// include/arena/debug/debug_system.h
#ifndef DEBUG_SYSTEM_H
#define DEBUG_SYSTEM_H

#include <QObject>
#include <memory>

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
    explicit DebugSystem(GameScene* scene, PlayerController* player, QObject* parent = nullptr);
    ~DebugSystem();
    
    /**
     * @brief Initialize the debug system
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
     * @brief Get the debug console
     * @return Pointer to debug console
     */
    DebugConsole* getConsole() const;
    
    /**
     * @brief Get the frustum visualizer
     * @return Pointer to frustum visualizer
     */
    FrustumVisualizer* getFrustumVisualizer() const;
    
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