#ifndef PLAYER_CONTROLLER_H
#define PLAYER_CONTROLLER_H

#include <QObject>
#include <QKeyEvent>
#include <QVector3D>
#include <QTimer>

class GameScene;

// Player stance enum
enum class PlayerStance {
    Standing,
    Crouching,
    Prone,
    Jumping
};

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
    
    // Get player stance
    PlayerStance getStance() const { return stance; }
    
    // Get player's eye height
    float getEyeHeight() const;
    
    // Start movement updates
    void startUpdates();
    
    // Stop movement updates
    void stopUpdates();
    
    // Handle key press events
    void handleKeyPress(QKeyEvent *event);
    
    // Handle key release events
    void handleKeyRelease(QKeyEvent *event);
    
    // Handle mouse movement for look control
    void handleMouseMove(QMouseEvent *event);
    
    // Create the player entity in the scene
    void createPlayerEntity();

public slots:
    // Update player position based on current movement
    void updatePosition();
    
    // Handle stance transition delays
    void completeStanceTransition();

signals:
    // Signal for when player position changes
    void positionChanged(const QVector3D &newPosition);
    
    // Signal for when player rotation changes
    void rotationChanged(float newRotation);
    
    // Signal for when player stance changes
    void stanceChanged(PlayerStance newStance);

private:
    GameScene *gameScene;
    QVector3D position;
    QVector3D velocity;       // Current velocity vector 
    QVector3D targetVelocity; // Target velocity based on input
    float rotation;
    float movementSpeed;
    float rotationSpeed;
    float acceleration;       // How fast to reach target velocity
    float friction;           // Deceleration when not moving
    QTimer updateTimer;
    QTimer stanceTransitionTimer;
    
    // Player stance properties
    PlayerStance stance;
    PlayerStance targetStance;
    bool inStanceTransition;
    
    // Movement flags
    bool movingForward;
    bool movingBackward;
    bool movingLeft;
    bool movingRight;
    bool rotatingLeft;
    bool rotatingRight;
    bool jumping;
    bool sprinting;
    
    // Jump physics
    float jumpVelocity;
    float gravity;
    
    // Apply movement constraints (walls, etc.)
    QVector3D applyConstraints(const QVector3D &newPosition);
    
    // Get current speed multiplier based on stance and sprint
    float getSpeedMultiplier() const;
    
    // Begin transition to a new stance
    void beginStanceTransition(PlayerStance newStance);
};

#endif // PLAYER_CONTROLLER_H