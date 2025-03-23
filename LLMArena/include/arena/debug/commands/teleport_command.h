// include/arena/debug/commands/teleport_command.h
#ifndef TELEPORT_COMMAND_H
#define TELEPORT_COMMAND_H

#include "debug_command.h"

/**
 * @brief Command to teleport the player to specific coordinates
 * 
 * This command teleports the player to the specified coordinates
 * and optionally sets rotation.
 */
class TeleportCommand : public DebugCommand {
    Q_OBJECT
    
public:
    explicit TeleportCommand(QObject* parent = nullptr);
    
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
    
private:
    /**
     * @brief Convert degrees to radians
     * @param degrees Angle in degrees
     * @return Angle in radians
     */
    float degreesToRadians(float degrees) const;
};

#endif // TELEPORT_COMMAND_H