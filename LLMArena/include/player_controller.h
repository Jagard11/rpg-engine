// include/player_controller.h
#ifndef PLAYER_CONTROLLER_H
#define PLAYER_CONTROLLER_H

#include <QObject>
#include <QKeyEvent>
#include <QVector3D>
#include <QTimer>

class GameScene;

// Class to handle player input and movement
class PlayerController : public QObject {
    Q_OBJECT

public:
    explicit PlayerController(GameScene *scene, QObject *parent = nullptr);
    
    // Set movement speed
    void setMovementSpeed(float speed) { movementSpeed = speed; }
    
    // Set rotation speed
    void setRotationSpeed(float speed) { rotationSpeed = speed; }
    
    // Get player position
    QVector3D getPosition() const { return position; }
    
    // Get player rotation (in radians)
    float getRotation() const { return rotation; }
    
    // Start movement updates
    void startUpdates();
    
    // Stop movement updates
    void stopUpdates();
    
    // Handle key press events
    void handleKeyPress(QKeyEvent *event);
    
    // Handle key release events
    void handleKeyRelease(QKeyEvent *event);
    
    // Create the player entity in the scene
    void createPlayerEntity();

public slots:
    // Update player position based on current movement
    void updatePosition();

signals:
    // Signal for when player position changes
    void positionChanged(const QVector3D &newPosition);
    
    // Signal for when player rotation changes
    void rotationChanged(float newRotation);

private:
    GameScene *gameScene;
    QVector3D position;
    float rotation;
    float movementSpeed;
    float rotationSpeed;
    QTimer updateTimer;
    
    // Movement flags
    bool movingForward;
    bool movingBackward;
    bool movingLeft;
    bool movingRight;
    bool rotatingLeft;
    bool rotatingRight;
    
    // Apply movement constraints (walls, etc.)
    QVector3D applyConstraints(const QVector3D &newPosition);
};

#endif // PLAYER_CONTROLLER_H