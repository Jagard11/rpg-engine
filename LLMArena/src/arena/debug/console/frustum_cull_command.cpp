// src/arena/debug/commands/frustum_cull_command.cpp
#include "../../../../include/arena/debug/commands/frustum_cull_command.h"
#include "../../../../include/arena/debug/visualizers/frustum_visualizer.h"
#include "../../../../include/arena/game/player_controller.h"
#include "../../../../include/arena/core/arena_core.h"
#include <QDebug>

FrustumCullCommand::FrustumCullCommand(FrustumVisualizer* visualizer, QObject* parent)
    : DebugCommand(parent),
      m_visualizer(visualizer)
{
    // Ensure we have a valid visualizer
    if (!m_visualizer) {
        qWarning() << "FrustumCullCommand created with null visualizer";
    }
}

QString FrustumCullCommand::execute(const QStringList& args, GameScene* gameScene, 
                                   PlayerController* playerController)
{
    Q_UNUSED(gameScene);
    Q_UNUSED(playerController);
    
    if (!m_visualizer) {
        return "Error: Frustum visualizer not available";
    }
    
    // Handle arguments
    if (args.isEmpty()) {
        // Toggle visualization
        bool newState = !m_visualizer->isEnabled();
        m_visualizer->setEnabled(newState);
        
        qDebug() << "Frustum culling visualization toggled to:" << newState;
        
        return QString("Frustum culling visualization %1").arg(newState ? "enabled" : "disabled");
    } else {
        // Set specific state
        bool ok;
        int state = args[0].toInt(&ok);
        
        if (!ok || (state != 0 && state != 1)) {
            return "Error: Invalid argument. Use 0 to disable or 1 to enable";
        }
        
        m_visualizer->setEnabled(state == 1);
        
        qDebug() << "Frustum culling visualization set to:" << (state == 1);
        
        return QString("Frustum culling visualization %1").arg(state == 1 ? "enabled" : "disabled");
    }
}

QString FrustumCullCommand::getName() const
{
    return "FrustumCullBox";
}

QString FrustumCullCommand::getDescription() const
{
    return "Toggle/set frustum culling bounding box visualization";
}

QString FrustumCullCommand::getUsage() const
{
    return "FrustumCullBox [0|1]";
}