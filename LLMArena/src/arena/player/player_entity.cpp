// src/arena/player/player_entity.cpp
#include "../../../include/arena/player/player_entity.h"
#include "../../../include/arena/core/arena_core.h"
#include <QDebug>
#include <QtMath>
#include <QElapsedTimer>

// Static variable for tracking delta time
static QElapsedTimer s_deltaTimer;
static qint64 s_lastFrameTime = 0;

PlayerEntity::PlayerEntity(GameScene* gameScene, QObject* parent)
    : QObject(parent),
      m_gameScene(gameScene),
      m_movementState(PlayerMovementState::Standing),
      m_movingForward(false),
      m_movingBackward(false),
      m_movingLeft(false),
      m_movingRight(false),
      m_jumping(false),
      m_sprinting(false),
      m_crouching(false),
      m_prone(false),
      m_velocity(0, 0, 0),
      m_acceleration(0, 0, 0),
      m_maxWalkSpeed(5.0f),
      m_maxRunSpeed(8.0f),
      m_maxCrouchSpeed(2.5f),
      m_maxProneSpeed(1.0f),
      m_jumpForce(8.0f),
      m_gravity(20.0f),
      m_mouseSensitivity(0.003f),
      m_screenWidth(800),
      m_screenHeight(600)
{
    // Create components
    m_camera = std::make_unique<CameraComponent>(this);
    
    // Connect component signals
    connect(m_camera.get(), &CameraComponent::positionChanged,
            this, &PlayerEntity::onCameraPositionChanged);
    
    connect(m_camera.get(), &CameraComponent::rotationChanged,
            this, &PlayerEntity::onCameraRotationChanged);
    
    connect(m_camera.get(), &CameraComponent::viewMatrixChanged,
            this, &PlayerEntity::viewMatrixChanged);
    
    connect(m_camera.get(), &CameraComponent::projectionMatrixChanged,
            this, &PlayerEntity::projectionMatrixChanged);
    
    // Set up update timer
    m_updateTimer.setInterval(16); // ~60 FPS
    connect(&m_updateTimer, &QTimer::timeout, this, &PlayerEntity::update);
    
    // Initialize delta timer for physics
    s_deltaTimer.start();
    s_lastFrameTime = s_deltaTimer.elapsed();
}

PlayerEntity::~PlayerEntity() = default;

void PlayerEntity::initialize() {
    // Initialize camera component
    m_camera->initialize();
    
    // Set initial position (higher above the ground)
    setPosition(QVector3D(0, 5.0, 0));
    
    // Set initial rotation (looking forward)
    setRotation(0, 0);
    
    // Create player entity in game scene
    if (m_gameScene) {
        GameEntity playerEntity;
        playerEntity.id = "player";
        playerEntity.type = "player";
        playerEntity.position = getPosition();
        playerEntity.dimensions = QVector3D(0.6f, 1.8f, 0.6f); // Standard player size
        playerEntity.isStatic = false;
        
        // Remove existing player entity if it exists
        try {
            GameEntity existingPlayer = m_gameScene->getEntity("player");
            if (!existingPlayer.id.isEmpty()) {
                m_gameScene->removeEntity("player");
            }
        } catch (...) {
            // Ignore errors
        }
        
        // Add player entity to scene
        m_gameScene->addEntity(playerEntity);
    }
}

void PlayerEntity::startUpdates() {
    m_updateTimer.start();
    s_lastFrameTime = s_deltaTimer.elapsed(); // Reset time delta
}

void PlayerEntity::stopUpdates() {
    m_updateTimer.stop();
}

void PlayerEntity::handleKeyPress(QKeyEvent* event) {
    if (!event) return;
    
    switch (event->key()) {
        case Qt::Key_W:
            m_movingForward = true;
            break;
        case Qt::Key_S:
            m_movingBackward = true;
            break;
        case Qt::Key_A:
            m_movingLeft = true;
            break;
        case Qt::Key_D:
            m_movingRight = true;
            break;
        case Qt::Key_Space:
            if (m_movementState == PlayerMovementState::Standing || 
                m_movementState == PlayerMovementState::Walking || 
                m_movementState == PlayerMovementState::Running) {
                m_jumping = true;
            }
            break;
        case Qt::Key_Shift:
            m_sprinting = true;
            break;
        case Qt::Key_C:
            m_crouching = !m_crouching;
            m_prone = false; // Can't be prone and crouching
            break;
        case Qt::Key_Z:
            m_prone = !m_prone;
            m_crouching = false; // Can't be crouching and prone
            break;
    }
}

void PlayerEntity::handleKeyRelease(QKeyEvent* event) {
    if (!event) return;
    
    switch (event->key()) {
        case Qt::Key_W:
            m_movingForward = false;
            break;
        case Qt::Key_S:
            m_movingBackward = false;
            break;
        case Qt::Key_A:
            m_movingLeft = false;
            break;
        case Qt::Key_D:
            m_movingRight = false;
            break;
        case Qt::Key_Shift:
            m_sprinting = false;
            break;
    }
}

void PlayerEntity::handleMouseMove(QMouseEvent* event) {
    if (!event) return;
    
    // Calculate mouse movement delta
    int centerX = m_screenWidth / 2;
    int centerY = m_screenHeight / 2;
    int deltaX = event->x() - centerX;
    int deltaY = event->y() - centerY;
    
    // Only process significant movement
    if (deltaX != 0 || deltaY != 0) {
        // Get current rotation
        QVector3D rotation = getRotation();
        float yaw = rotation.x();
        float pitch = rotation.y();
        
        // Apply mouse sensitivity (note the orientation)
        yaw += deltaX * m_mouseSensitivity;
        pitch -= deltaY * m_mouseSensitivity; // Inverted for natural movement
        
        // Clamp pitch to avoid gimbal lock issues
        const float MAX_PITCH = 89.0f * M_PI / 180.0f;
        pitch = qBound(-MAX_PITCH, pitch, MAX_PITCH);
        
        // Normalize yaw to [0, 2π)
        while (yaw < 0) yaw += 2 * M_PI;
        while (yaw >= 2 * M_PI) yaw -= 2 * M_PI;
        
        // Set new rotation
        setRotation(yaw, pitch);
    }
}

void PlayerEntity::handleMousePress(QMouseEvent* event) {
    // Handle mouse button presses (e.g., for shooting, interaction)
}

void PlayerEntity::handleMouseRelease(QMouseEvent* event) {
    // Handle mouse button releases
}

void PlayerEntity::setPosition(const QVector3D& position) {
    // Update camera position (entity position follows camera)
    m_camera->setPosition(position);
    
    // Update game scene entity
    if (m_gameScene) {
        try {
            m_gameScene->updateEntityPosition("player", position);
        } catch (...) {
            // Ignore errors
        }
    }
    
    // Emit position changed signal
    emit positionChanged(position);
}

void PlayerEntity::setRotation(float yaw, float pitch) {
    // Update camera rotation
    m_camera->setRotation(yaw, pitch);
    
    // Emit rotation changed signal
    emit rotationChanged(yaw, pitch);
}

QVector3D PlayerEntity::getPosition() const {
    return m_camera->getPosition();
}

QVector3D PlayerEntity::getRotation() const {
    return m_camera->getRotation();
}

float PlayerEntity::getEyeHeight() const {
    // Return eye height based on stance
    switch (m_movementState) {
        case PlayerMovementState::Standing:
        case PlayerMovementState::Walking:
        case PlayerMovementState::Running:
        case PlayerMovementState::Jumping:
        case PlayerMovementState::Falling:
            return 1.7f; // Standard eye height for standing
            
        case PlayerMovementState::Crouching:
            return 1.0f; // Lower eye height for crouching
            
        case PlayerMovementState::Prone:
            return 0.3f; // Very low eye height for prone
            
        default:
            return 1.7f;
    }
}

bool PlayerEntity::isPointVisible(const QVector3D& point) const {
    return m_camera->isPointInFrustum(point);
}

bool PlayerEntity::isSphereVisible(const QVector3D& center, float radius) const {
    return m_camera->isSphereInFrustum(center, radius);
}

bool PlayerEntity::isBoxVisible(const QVector3D& min, const QVector3D& max) const {
    return m_camera->isBoxInFrustum(min, max);
}

void PlayerEntity::setScreenDimensions(int width, int height) {
    m_screenWidth = width;
    m_screenHeight = height;
    
    // Update camera aspect ratio
    if (height > 0) {
        m_camera->setAspectRatio(static_cast<float>(width) / height);
    }
}

void PlayerEntity::update() {
    // Calculate delta time
    qint64 currentTime = s_deltaTimer.elapsed();
    float deltaTime = (currentTime - s_lastFrameTime) / 1000.0f; // Convert to seconds
    s_lastFrameTime = currentTime;
    
    // Clamp delta time to avoid large jumps
    deltaTime = qMin(deltaTime, 0.1f);
    
    // Update physics with delta time
    updatePhysics(deltaTime);
    
    // Update camera
    m_camera->update();
}

void PlayerEntity::onCameraPositionChanged(const QVector3D& position) {
    // Player position follows camera, emit signal
    emit positionChanged(position);
}

void PlayerEntity::onCameraRotationChanged(float yaw, float pitch, float roll) {
    // Player rotation follows camera, emit signal
    emit rotationChanged(yaw, pitch);
}

void PlayerEntity::updatePhysics(float deltaTime) {
    // Determine current movement state based on input
    PlayerMovementState newState = m_movementState;
    
    if (m_jumping) {
        newState = PlayerMovementState::Jumping;
        m_jumping = false;
        m_velocity.setY(m_jumpForce); // Apply initial jump velocity
    } else if (m_velocity.y() < -0.1f) {
        newState = PlayerMovementState::Falling;
    } else if (m_prone) {
        newState = PlayerMovementState::Prone;
    } else if (m_crouching) {
        newState = PlayerMovementState::Crouching;
    } else if (m_sprinting && (m_movingForward || m_movingBackward || m_movingLeft || m_movingRight)) {
        newState = PlayerMovementState::Running;
    } else if (m_movingForward || m_movingBackward || m_movingLeft || m_movingRight) {
        newState = PlayerMovementState::Walking;
    } else {
        newState = PlayerMovementState::Standing;
    }
    
    // Update movement state if changed
    if (newState != m_movementState) {
        setMovementState(newState);
    }
    
    // Calculate movement direction
    QVector3D moveDirection(0, 0, 0);
    
    if (m_movingForward || m_movingBackward || m_movingLeft || m_movingRight) {
        // Get camera orientation vectors
        QVector3D forward = m_camera->getForwardVector();
        QVector3D right = m_camera->getRightVector();
        
        // Zero out Y component for ground movement
        forward.setY(0);
        right.setY(0);
        
        // Normalize if not zero
        if (forward.length() > 0.001f) forward.normalize();
        if (right.length() > 0.001f) right.normalize();
        
        // Combine movement inputs
        if (m_movingForward) moveDirection += forward;
        if (m_movingBackward) moveDirection -= forward;
        if (m_movingRight) moveDirection += right;
        if (m_movingLeft) moveDirection -= right;
        
        // Normalize if not zero
        if (moveDirection.length() > 0.001f) {
            moveDirection.normalize();
        }
    }
    
    // Apply movement forces
    float maxSpeed;
    switch (m_movementState) {
        case PlayerMovementState::Running:
            maxSpeed = m_maxRunSpeed;
            break;
        case PlayerMovementState::Crouching:
            maxSpeed = m_maxCrouchSpeed;
            break;
        case PlayerMovementState::Prone:
            maxSpeed = m_maxProneSpeed;
            break;
        default:
            maxSpeed = m_maxWalkSpeed;
            break;
    }
    
    // Calculate target velocity
    QVector3D targetVelocity = moveDirection * maxSpeed;
    
    // Apply acceleration to horizontal velocity (smooth movement)
    float accelRate = (moveDirection.length() > 0.001f) ? 10.0f : 15.0f; // Faster deceleration
    QVector3D horizVelocity(m_velocity.x(), 0, m_velocity.z());
    QVector3D targetHorizVelocity(targetVelocity.x(), 0, targetVelocity.z());
    
    // Interpolate towards target velocity
    horizVelocity = horizVelocity + (targetHorizVelocity - horizVelocity) * qMin(accelRate * deltaTime, 1.0f);
    
    // Update horizontal velocity
    m_velocity.setX(horizVelocity.x());
    m_velocity.setZ(horizVelocity.z());
    
    // Apply gravity
    if (m_movementState == PlayerMovementState::Jumping || 
        m_movementState == PlayerMovementState::Falling) {
        m_velocity.setY(m_velocity.y() - m_gravity * deltaTime);
    } else {
        m_velocity.setY(0); // Grounded
    }
    
    // Calculate new position
    QVector3D newPosition = getPosition() + m_velocity * deltaTime;
    
    // Check for collisions
    if (!checkCollision(newPosition)) {
        // No collision, update position
        setPosition(newPosition);
    } else {
        // Collision detected, attempt to slide along surfaces
        
        // Try X movement only
        QVector3D xMovement = getPosition();
        xMovement.setX(newPosition.x());
        
        if (!checkCollision(xMovement)) {
            setPosition(xMovement);
            m_velocity.setZ(0); // Zero out Z velocity on X collision
        }
        
        // Try Z movement only
        QVector3D zMovement = getPosition();
        zMovement.setZ(newPosition.z());
        
        if (!checkCollision(zMovement)) {
            setPosition(zMovement);
            m_velocity.setX(0); // Zero out X velocity on Z collision
        }
        
        // If we're jumping/falling, check Y collision
        if (m_movementState == PlayerMovementState::Jumping || 
            m_movementState == PlayerMovementState::Falling) {
            
            QVector3D yMovement = getPosition();
            yMovement.setY(newPosition.y());
            
            if (!checkCollision(yMovement)) {
                setPosition(yMovement);
            } else {
                // Hit ground or ceiling
                m_velocity.setY(0);
                
                // If hitting ground, change state
                if (m_velocity.y() < 0) {
                    setMovementState(PlayerMovementState::Standing);
                }
            }
        }
    }
    
    // Update eye height based on stance
    float currentY = getPosition().y();
    float targetY = getEyeHeight();
    
    // Only adjust eye height if not jumping/falling
    if (m_movementState != PlayerMovementState::Jumping && 
        m_movementState != PlayerMovementState::Falling) {
        
        // Smoothly interpolate to target eye height
        float newY = currentY + (targetY - currentY) * qMin(10.0f * deltaTime, 1.0f);
        
        QVector3D adjustedPos = getPosition();
        adjustedPos.setY(newY);
        setPosition(adjustedPos);
    }
}

bool PlayerEntity::checkCollision(const QVector3D& newPosition) {
    if (!m_gameScene) {
        return false;
    }
    
    // Create a GameEntity for collision testing
    GameEntity playerEntity;
    playerEntity.id = "player";
    playerEntity.type = "player";
    playerEntity.position = newPosition;
    
    // Size depends on stance
    float height;
    switch (m_movementState) {
        case PlayerMovementState::Crouching:
            height = 1.2f;
            break;
        case PlayerMovementState::Prone:
            height = 0.5f;
            break;
        default:
            height = 1.8f;
            break;
    }
    
    playerEntity.dimensions = QVector3D(0.6f, height, 0.6f);
    
    // Check collision with game scene
    return m_gameScene->checkCollision("player", newPosition);
}

void PlayerEntity::setMovementState(PlayerMovementState state) {
    if (m_movementState != state) {
        m_movementState = state;
        emit movementStateChanged(m_movementState);
    }
}