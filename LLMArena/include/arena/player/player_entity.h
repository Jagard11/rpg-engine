// include/arena/player/player_entity.h
#ifndef PLAYER_ENTITY_H
#define PLAYER_ENTITY_H

#include <QObject>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QVector3D>
#include <memory>

#include "components/camera_component.h"

// Forward declarations
class GameScene;

/**
 * @brief Enum for player movement states
 */
enum class PlayerMovementState {
    Standing,
    Walking,
    Running,
    Crouching,
    Prone,
    Jumping,
    Falling
};

/**
 * @brief Component-based player entity
 * 
 * This class represents the player entity with various components
 * including camera, movement, collision, and interaction.
 */
class PlayerEntity : public QObject {
    Q_OBJECT
    
public:
    explicit PlayerEntity(GameScene* gameScene, QObject* parent = nullptr);
    ~PlayerEntity();
    
    /**
     * @brief Initialize the player entity
     */
    void initialize();
    
    /**
     * @brief Start updating the player
     */
    void startUpdates();
    
    /**
     * @brief Stop updating the player
     */
    void stopUpdates();
    
    /**
     * @brief Handle key press event
     * @param event Key event
     */
    void handleKeyPress(QKeyEvent* event);
    
    /**
     * @brief Handle key release event
     * @param event Key event
     */
    void handleKeyRelease(QKeyEvent* event);
    
    /**
     * @brief Handle mouse move event
     * @param event Mouse event
     */
    void handleMouseMove(QMouseEvent* event);
    
    /**
     * @brief Handle mouse press event
     * @param event Mouse event
     */
    void handleMousePress(QMouseEvent* event);
    
    /**
     * @brief Handle mouse release event
     * @param event Mouse event
     */
    void handleMouseRelease(QMouseEvent* event);
    
    /**
     * @brief Set player position
     * @param position Position in world coordinates
     */
    void setPosition(const QVector3D& position);
    
    /**
     * @brief Set player rotation (yaw and pitch)
     * @param yaw Horizontal rotation (around Y axis) in radians
     * @param pitch Vertical rotation (around X axis) in radians
     */
    void setRotation(float yaw, float pitch);
    
    /**
     * @brief Get player position
     * @return Position in world coordinates
     */
    QVector3D getPosition() const;
    
    /**
     * @brief Get player rotation
     * @return Vector with yaw, pitch, and roll in radians
     */
    QVector3D getRotation() const;
    
    /**
     * @brief Get player movement state
     * @return Current movement state
     */
    PlayerMovementState getMovementState() const { return m_movementState; }
    
    /**
     * @brief Get player camera component
     * @return Reference to camera component
     */
    CameraComponent* getCamera() const { return m_camera.get(); }
    
    /**
     * @brief Get player eye height based on stance
     * @return Eye height in world units
     */
    float getEyeHeight() const;
    
    /**
     * @brief Check if a point is visible to the player
     * @param point Point in world coordinates
     * @return True if the point is in the player's view frustum
     */
    bool isPointVisible(const QVector3D& point) const;
    
    /**
     * @brief Check if a sphere is visible to the player
     * @param center Sphere center in world coordinates
     * @param radius Sphere radius
     * @return True if the sphere is in the player's view frustum
     */
    bool isSphereVisible(const QVector3D& center, float radius) const;
    
    /**
     * @brief Check if a box is visible to the player
     * @param min Minimum corner in world coordinates
     * @param max Maximum corner in world coordinates
     * @return True if the box is in the player's view frustum
     */
    bool isBoxVisible(const QVector3D& min, const QVector3D& max) const;
    
    /**
     * @brief Set the screen dimensions for mouse input
     * @param width Screen width
     * @param height Screen height
     */
    void setScreenDimensions(int width, int height);
    
signals:
    /**
     * @brief Signal emitted when player position changes
     * @param position New position
     */
    void positionChanged(const QVector3D& position);
    
    /**
     * @brief Signal emitted when player rotation changes
     * @param yaw New yaw angle in radians
     * @param pitch New pitch angle in radians
     */
    void rotationChanged(float yaw, float pitch);
    
    /**
     * @brief Signal emitted when player movement state changes
     * @param state New movement state
     */
    void movementStateChanged(PlayerMovementState state);
    
    /**
     * @brief Signal emitted when view matrix changes
     * @param viewMatrix New view matrix
     */
    void viewMatrixChanged(const QMatrix4x4& viewMatrix);
    
    /**
     * @brief Signal emitted when projection matrix changes
     * @param projectionMatrix New projection matrix
     */
    void projectionMatrixChanged(const QMatrix4x4& projectionMatrix);
    
private slots:
    /**
     * @brief Update player state
     */
    void update();
    
    /**
     * @brief Handle camera position changed
     * @param position New camera position
     */
    void onCameraPositionChanged(const QVector3D& position);
    
    /**
     * @brief Handle camera rotation changed
     * @param yaw New yaw angle in radians
     * @param pitch New pitch angle in radians
     * @param roll New roll angle in radians
     */
    void onCameraRotationChanged(float yaw, float pitch, float roll);
    
private:
    // Game scene reference
    GameScene* m_gameScene;
    
    // Components
    std::unique_ptr<CameraComponent> m_camera;
    
    // Movement state
    PlayerMovementState m_movementState;
    
    // Movement input flags
    bool m_movingForward;
    bool m_movingBackward;
    bool m_movingLeft;
    bool m_movingRight;
    bool m_jumping;
    bool m_sprinting;
    bool m_crouching;
    bool m_prone;
    
    // Physics properties
    QVector3D m_velocity;
    QVector3D m_acceleration;
    float m_maxWalkSpeed;
    float m_maxRunSpeed;
    float m_maxCrouchSpeed;
    float m_maxProneSpeed;
    float m_jumpForce;
    float m_gravity;
    
    // Input settings
    float m_mouseSensitivity;
    
    // Screen dimensions for mouse movement
    int m_screenWidth;
    int m_screenHeight;
    
    // Update timer
    QTimer m_updateTimer;
    
    // Physics update
    void updatePhysics(float deltaTime);
    
    // Handle collision
    bool checkCollision(const QVector3D& newPosition);
    
    // Set movement state
    void setMovementState(PlayerMovementState state);
};

#endif // PLAYER_ENTITY_H