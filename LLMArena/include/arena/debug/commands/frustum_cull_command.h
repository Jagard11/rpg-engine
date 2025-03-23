// include/arena/debug/commands/frustum_cull_command.h
#ifndef FRUSTUM_CULL_COMMAND_H
#define FRUSTUM_CULL_COMMAND_H

#include "debug_command.h"

// Forward declarations
class FrustumVisualizer;

/**
 * @brief Command to toggle frustum culling visualization
 * 
 * This command toggles the visibility of the frustum culling bounding box
 * to help developers understand how frustum culling works.
 */
class FrustumCullCommand : public DebugCommand {
    Q_OBJECT
    
public:
    explicit FrustumCullCommand(FrustumVisualizer* visualizer, QObject* parent = nullptr);
    
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
    FrustumVisualizer* m_visualizer;
};

#endif // FRUSTUM_CULL_COMMAND_H