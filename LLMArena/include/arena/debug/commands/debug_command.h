// include/arena/debug/commands/debug_command.h
#ifndef DEBUG_COMMAND_H
#define DEBUG_COMMAND_H

#include <QString>
#include <QStringList>
#include <QObject>

// Forward declarations
class GameScene;
class PlayerController;

/**
 * @brief Base interface for all debug commands
 * 
 * This abstract class defines the interface for all debug commands
 * that can be executed from the debug console.
 */
class DebugCommand : public QObject {
    Q_OBJECT
    
public:
    explicit DebugCommand(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~DebugCommand() = default;
    
    /**
     * @brief Execute the command with the given arguments
     * @param args Arguments passed to the command
     * @param gameScene Reference to the game scene
     * @param playerController Reference to the player controller
     * @return Result message to display in the console
     */
    virtual QString execute(const QStringList& args, GameScene* gameScene, 
                            PlayerController* playerController) = 0;
    
    /**
     * @brief Get the command name (what the user types to execute it)
     * @return Command name
     */
    virtual QString getName() const = 0;
    
    /**
     * @brief Get the command description for help text
     * @return Command description
     */
    virtual QString getDescription() const = 0;
    
    /**
     * @brief Get the command usage
     * @return Command usage info
     */
    virtual QString getUsage() const = 0;
};

#endif // DEBUG_COMMAND_H