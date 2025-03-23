// src/arena/debug/commands/location_command.cpp
#include "../../../../include/arena/debug/commands/location_command.h"
#include "../../../../include/arena/game/player_controller.h"
#include "../../../../include/arena/core/arena_core.h"
#include <QDebug>
#include <QtMath>

LocationCommand::LocationCommand(QObject* parent)
    : DebugCommand(parent)
{
}

QString LocationCommand::execute(const QStringList& args, GameScene* gameScene, 
                                PlayerController* playerController)
{
    Q_UNUSED(args);
    Q_UNUSED(gameScene);
    
    if (!playerController) {
        return "Error: Player controller not available";
    }
    
    // Get player position
    QVector3D position = playerController->getPosition();
    
    // Get player rotation (convert to degrees for better readability)
    float rotationRad = playerController->getRotation();
    float pitchRad = playerController->getPitch();
    
    float rotationDeg = rotationRad * 180.0f / M_PI;
    float pitchDeg = pitchRad * 180.0f / M_PI;
    
    // Format the result message
    QString result = QString("Position: X=%.2f, Y=%.2f, Z=%.2f\n")
                    .arg(position.x())
                    .arg(position.y())
                    .arg(position.z());
                    
    result += QString("Rotation: %.2f° (%.2f rad)\nPitch: %.2f° (%.2f rad)")
             .arg(rotationDeg)
             .arg(rotationRad)
             .arg(pitchDeg)
             .arg(pitchRad);
    
    return result;
}

QString LocationCommand::getName() const
{
    return "loc";
}

QString LocationCommand::getDescription() const
{
    return "Display the player's current position and rotation";
}

QString LocationCommand::getUsage() const
{
    return "loc";
}