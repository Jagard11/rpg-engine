// src/arena/player/components/camera_component.cpp
#include "../../../../include/arena/player/components/camera_component.h"
#include "../../../../include/arena/voxels/culling/view_frustum.h" // Include the full definition
#include <QtMath>
#include <QDebug>

CameraComponent::CameraComponent(QObject* parent)
    : QObject(parent),
      m_position(0, 0, 0),
      m_yaw(0),
      m_pitch(0),
      m_roll(0),
      m_fieldOfView(70.0f),
      m_aspectRatio(16.0f/9.0f),
      m_nearPlane(0.1f),
      m_farPlane(1000.0f)
{
    // Create view frustum
    m_viewFrustum = std::make_unique<ViewFrustum>();
    
    // Initialize matrices
    updateViewMatrix();
    updateProjectionMatrix();
}

CameraComponent::~CameraComponent() = default;

void CameraComponent::initialize() {
    // Initial calculation of matrices
    updateViewMatrix();
    updateProjectionMatrix();
    
    // Initialize frustum
    m_viewFrustum->update(getViewProjectionMatrix());
}

void CameraComponent::setPosition(const QVector3D& position) {
    if (m_position != position) {
        m_position = position;
        updateViewMatrix();
        emit positionChanged(m_position);
    }
}

void CameraComponent::setRotation(float yaw, float pitch, float roll) {
    bool changed = false;
    
    // Limit pitch to prevent camera flipping
    // Fix the type mismatch by using consistent float types
    float maxPitch = 89.0f * M_PI / 180.0f;
    float minPitch = -maxPitch;
    pitch = qBound(minPitch, pitch, maxPitch);
    
    if (m_yaw != yaw || m_pitch != pitch || m_roll != roll) {
        m_yaw = yaw;
        m_pitch = pitch;
        m_roll = roll;
        updateViewMatrix();
        changed = true;
    }
    
    if (changed) {
        emit rotationChanged(m_yaw, m_pitch, m_roll);
    }
}

void CameraComponent::setFieldOfView(float fov) {
    if (m_fieldOfView != fov) {
        m_fieldOfView = fov;
        updateProjectionMatrix();
    }
}

void CameraComponent::setAspectRatio(float aspect) {
    if (m_aspectRatio != aspect) {
        m_aspectRatio = aspect;
        updateProjectionMatrix();
    }
}

void CameraComponent::setClipPlanes(float nearPlane, float farPlane) {
    if (m_nearPlane != nearPlane || m_farPlane != farPlane) {
        m_nearPlane = nearPlane;
        m_farPlane = farPlane;
        updateProjectionMatrix();
    }
}

QVector3D CameraComponent::getForwardVector() const {
    // Calculate forward vector from yaw and pitch
    return QVector3D(
        cos(m_yaw) * cos(m_pitch),
        sin(m_pitch),
        sin(m_yaw) * cos(m_pitch)
    ).normalized();
}

QVector3D CameraComponent::getRightVector() const {
    // Right vector is perpendicular to forward and global up
    QVector3D forward = getForwardVector();
    QVector3D worldUp(0, 1, 0);
    return QVector3D::crossProduct(worldUp, forward).normalized();
}

QVector3D CameraComponent::getUpVector() const {
    // Up vector completes the orthonormal basis
    QVector3D forward = getForwardVector();
    QVector3D right = getRightVector();
    return QVector3D::crossProduct(forward, right).normalized();
}

QMatrix4x4 CameraComponent::getViewProjectionMatrix() const {
    return m_projectionMatrix * m_viewMatrix;
}

bool CameraComponent::isPointInFrustum(const QVector3D& point) const {
    return m_viewFrustum->isPointInside(point);
}

bool CameraComponent::isSphereInFrustum(const QVector3D& center, float radius) const {
    return m_viewFrustum->isSphereInside(center, radius);
}

bool CameraComponent::isBoxInFrustum(const QVector3D& min, const QVector3D& max) const {
    return m_viewFrustum->isBoxInside(min, max);
}

void CameraComponent::update() {
    // Update matrices (usually called once per frame)
    updateViewMatrix();
    
    // Update frustum based on current matrices
    m_viewFrustum->update(getViewProjectionMatrix());
}

void CameraComponent::updateViewMatrix() {
    // Calculate view matrix based on position and rotation
    QMatrix4x4 viewMatrix;
    
    // Start with identity
    viewMatrix.setToIdentity();
    
    // Apply rotation
    QQuaternion rotation = QQuaternion::fromEulerAngles(
        m_pitch * 180.0f / M_PI, 
        m_yaw * 180.0f / M_PI, 
        m_roll * 180.0f / M_PI
    );
    viewMatrix.rotate(rotation);
    
    // Apply translation
    viewMatrix.translate(-m_position);
    
    // Set view matrix
    if (m_viewMatrix != viewMatrix) {
        m_viewMatrix = viewMatrix;
        
        // Update frustum whenever view matrix changes
        if (m_viewFrustum) {
            m_viewFrustum->update(getViewProjectionMatrix());
        }
        
        emit viewMatrixChanged(m_viewMatrix);
    }
}

void CameraComponent::updateProjectionMatrix() {
    // Calculate projection matrix based on perspective settings
    QMatrix4x4 projectionMatrix;
    projectionMatrix.setToIdentity();
    projectionMatrix.perspective(m_fieldOfView, m_aspectRatio, m_nearPlane, m_farPlane);
    
    if (m_projectionMatrix != projectionMatrix) {
        m_projectionMatrix = projectionMatrix;
        
        // Update frustum whenever projection matrix changes
        if (m_viewFrustum) {
            m_viewFrustum->update(getViewProjectionMatrix());
        }
        
        emit projectionMatrixChanged(m_projectionMatrix);
    }
}