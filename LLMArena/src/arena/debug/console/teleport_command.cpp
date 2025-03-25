// src/arena/debug/console/teleport_command.cpp
#include "../../../../include/arena/debug/commands/teleport_command.h"
#include "../../../../include/arena/game/player_controller.h"
#include "../../../../include/arena/core/arena_core.h"
#include <QDebug>
#include <QtMath>

TeleportCommand::TeleportCommand(QObject* parent)
    : DebugCommand(parent)
{
}

QString TeleportCommand::execute(const QStringList& args, GameScene* gameScene, 
                                PlayerController* playerController)
{
    Q_UNUSED(gameScene);
    
    if (!playerController) {
        return "Error: Player controller not available";
    }
    
    // Check arguments
    if (args.size() < 3) {
        return "Error: Not enough arguments\nUsage: " + getUsage();
    }
    
    // Parse position coordinates
    bool ok1, ok2, ok3;
    float x = args[0].toFloat(&ok1);
    float y = args[1].toFloat(&ok2);
    float z = args[2].toFloat(&ok3);
    
    if (!ok1 || !ok2 || !ok3) {
        return "Error: Invalid coordinates";
    }
    
    // Create new position vector
    QVector3D newPosition(x, y, z);
    
    // Use the PlayerController's setPosition method to directly teleport the player
    playerController->setPosition(newPosition);
    
    // Handle optional rotation argument
    if (args.size() >= 4) {
        bool rotOk;
        float rotation = args[3].toFloat(&rotOk);
        
        if (rotOk) {
            // Convert from degrees to radians if needed
            float rotationRad = rotation;
            if (qAbs(rotation) > 2 * M_PI) {
                // Assume input is in degrees if > 2π
                rotationRad = degreesToRadians(rotation);
            }
            
            // Set the player's rotation
            playerController->setRotation(rotationRad);
            qDebug() << "Setting rotation to" << rotationRad;
        }
    }
    
    // Handle optional pitch argument
    if (args.size() >= 5) {
        bool pitchOk;
        float pitch = args[4].toFloat(&pitchOk);
        
        if (pitchOk) {
            // Convert from degrees to radians if needed
            float pitchRad = pitch;
            if (qAbs(pitch) > M_PI) {
                // Assume input is in degrees if > π
                pitchRad = degreesToRadians(pitch);
            }
            
            // Clamp pitch to avoid gimbal lock
            const float maxPitch = 89.0f * M_PI / 180.0f;
            pitchRad = qBound(-maxPitch, pitchRad, maxPitch);
            
            // Set the player's pitch
            playerController->setPitch(pitchRad);
            qDebug() << "Setting pitch to" << pitchRad;
        }
    }
    
    // Return success message
    if (args.size() >= 4) {
        if (args.size() >= 5) {
            return QString("Teleported to X=%.2f, Y=%.2f, Z=%.2f with rotation=%.2f and pitch=%.2f")
                   .arg(x).arg(y).arg(z).arg(args[3].toFloat()).arg(args[4].toFloat());
        } else {
            return QString("Teleported to X=%.2f, Y=%.2f, Z=%.2f with rotation=%.2f")
                   .arg(x).arg(y).arg(z).arg(args[3].toFloat());
        }
    } else {
        return QString("Teleported to X=%.2f, Y=%.2f, Z=%.2f")
               .arg(x).arg(y).arg(z);
    }
}

QString TeleportCommand::getName() const
{
    return "tp";
}

QString TeleportCommand::getDescription() const
{
    return "Teleport player to specified coordinates";
}

QString TeleportCommand::getUsage() const
{
    return "tp <x> <y> <z> [rotation] [pitch]";
}

float TeleportCommand::degreesToRadians(float degrees) const
{
    return degrees * M_PI / 180.0f;
}