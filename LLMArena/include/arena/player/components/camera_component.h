// include/arena/player/components/camera_component.h
#ifndef CAMERA_COMPONENT_H
#define CAMERA_COMPONENT_H

#include <QObject>
#include <QVector3D>
#include <QMatrix4x4>
#include <QQuaternion>
#include <memory>

// Forward declaration for ViewFrustum
class ViewFrustum;

/**
 * @brief Camera component for the player entity
 * 
 * This component handles the player's perspective, including view and
 * projection matrices, frustum culling, and camera movement.
 */
class CameraComponent : public QObject {
    Q_OBJECT
    
public:
    explicit CameraComponent(QObject* parent = nullptr);
    ~CameraComponent();
    
    // Initialize the camera
    void initialize();
    
    // Set camera position
    void setPosition(const QVector3D& position);
    
    // Set camera rotation (in radians)
    void setRotation(float yaw, float pitch, float roll = 0.0f);
    
    // Set camera field of view (in degrees)
    void setFieldOfView(float fov);
    
    // Set camera aspect ratio
    void setAspectRatio(float aspect);
    
    // Set near and far clip planes
    void setClipPlanes(float nearPlane, float farPlane);
    
    // Get camera position
    QVector3D getPosition() const { return m_position; }
    
    // Get camera rotation (yaw, pitch, roll)
    QVector3D getRotation() const { return QVector3D(m_yaw, m_pitch, m_roll); }
    
    // Get forward vector
    QVector3D getForwardVector() const;
    
    // Get right vector
    QVector3D getRightVector() const;
    
    // Get up vector
    QVector3D getUpVector() const;
    
    // Get view matrix
    const QMatrix4x4& getViewMatrix() const { return m_viewMatrix; }
    
    // Get projection matrix
    const QMatrix4x4& getProjectionMatrix() const { return m_projectionMatrix; }
    
    // Get combined view-projection matrix
    QMatrix4x4 getViewProjectionMatrix() const;
    
    // Check if a point is in the camera's view frustum
    bool isPointInFrustum(const QVector3D& point) const;
    
    // Check if a sphere is in the camera's view frustum
    bool isSphereInFrustum(const QVector3D& center, float radius) const;
    
    // Check if a box is in the camera's view frustum
    bool isBoxInFrustum(const QVector3D& min, const QVector3D& max) const;
    
    // Update the camera (recalculate matrices)
    void update();
    
signals:
    void positionChanged(const QVector3D& position);
    void rotationChanged(float yaw, float pitch, float roll);
    void viewMatrixChanged(const QMatrix4x4& viewMatrix);
    void projectionMatrixChanged(const QMatrix4x4& projectionMatrix);
    
private:
    // Camera properties
    QVector3D m_position;
    float m_yaw;   // Left/right rotation (around Y-axis)
    float m_pitch; // Up/down rotation (around X-axis)
    float m_roll;  // Roll (around Z-axis)
    
    // Perspective settings
    float m_fieldOfView;
    float m_aspectRatio;
    float m_nearPlane;
    float m_farPlane;
    
    // Matrices
    QMatrix4x4 m_viewMatrix;
    QMatrix4x4 m_projectionMatrix;
    
    // View frustum for culling
    std::unique_ptr<ViewFrustum> m_viewFrustum;
    
    // Recalculate view matrix
    void updateViewMatrix();
    
    // Recalculate projection matrix
    void updateProjectionMatrix();
};

#endif // CAMERA_COMPONENT_H