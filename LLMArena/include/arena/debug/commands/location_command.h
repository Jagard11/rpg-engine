// include/arena/debug/commands/location_command.h
#ifndef LOCATION_COMMAND_H
#define LOCATION_COMMAND_H

#include "debug_command.h"

/**
 * @brief Command to display the player's current location
 * 
 * This command prints the player's current coordinates and rotation
 * to the debug console.
 */
class LocationCommand : public DebugCommand {
    Q_OBJECT
    
public:
    explicit LocationCommand(QObject* parent = nullptr);
    
    /**
     * @brief Execute the command with the given arguments
     * @param args Arguments passed to the command
     * @param gameScene Reference to the game scene
     * @param playerController Reference to the player controller
     * @return Result message to display in the console
     */
    QString execute(const QStringList& args, GameScene* gameScene, 
                    PlayerController* playerController) override;
    
    /**
     * @brief Get the command name (what the user types to execute it)
     * @return Command name
     */
    QString getName() const override;
    
    /**
     * @brief Get the command description for help text
     * @return Command description
     */
    QString getDescription() const override;
    
    /**
     * @brief Get the command usage
     * @return Command usage info
     */
    QString getUsage() const override;
};

#endif // LOCATION_COMMAND_H