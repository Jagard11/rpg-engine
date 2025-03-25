// src/arena/voxels/culling/view_frustum.cpp
#include "../../../../include/arena/voxels/culling/view_frustum.h"
#include <QDebug>

ViewFrustum::ViewFrustum() {
    // Initialize planes to an inside-out frustum
    for (int i = 0; i < PlaneCount; ++i) {
        m_planes[i].normal = QVector3D(0, 0, 0);
        m_planes[i].distance = 0;
    }
    
    // Initialize corners to origin
    for (int i = 0; i < 8; ++i) {
        m_corners[i] = QVector3D(0, 0, 0);
    }
}

void ViewFrustum::update(const QMatrix4x4& viewProjection) {
    // Extract frustum planes from the view-projection matrix
    // Based on the technique from "Fast Extraction of Viewing Frustum Planes from the WorldView-Projection Matrix"
    
    // Extract rows from the matrix
    QVector4D row0(viewProjection(0, 0), viewProjection(0, 1), viewProjection(0, 2), viewProjection(0, 3));
    QVector4D row1(viewProjection(1, 0), viewProjection(1, 1), viewProjection(1, 2), viewProjection(1, 3));
    QVector4D row2(viewProjection(2, 0), viewProjection(2, 1), viewProjection(2, 2), viewProjection(2, 3));
    QVector4D row3(viewProjection(3, 0), viewProjection(3, 1), viewProjection(3, 2), viewProjection(3, 3));
    
    // Left plane = row3 + row0
    QVector4D leftPlane = row3 + row0;
    m_planes[Left].normal = QVector3D(leftPlane.x(), leftPlane.y(), leftPlane.z());
    m_planes[Left].distance = leftPlane.w();
    
    // Right plane = row3 - row0
    QVector4D rightPlane = row3 - row0;
    m_planes[Right].normal = QVector3D(rightPlane.x(), rightPlane.y(), rightPlane.z());
    m_planes[Right].distance = rightPlane.w();
    
    // Bottom plane = row3 + row1
    QVector4D bottomPlane = row3 + row1;
    m_planes[Bottom].normal = QVector3D(bottomPlane.x(), bottomPlane.y(), bottomPlane.z());
    m_planes[Bottom].distance = bottomPlane.w();
    
    // Top plane = row3 - row1
    QVector4D topPlane = row3 - row1;
    m_planes[Top].normal = QVector3D(topPlane.x(), topPlane.y(), topPlane.z());
    m_planes[Top].distance = topPlane.w();
    
    // Near plane = row3 + row2
    QVector4D nearPlane = row3 + row2;
    m_planes[Near].normal = QVector3D(nearPlane.x(), nearPlane.y(), nearPlane.z());
    m_planes[Near].distance = nearPlane.w();
    
    // Far plane = row3 - row2
    QVector4D farPlane = row3 - row2;
    m_planes[Far].normal = QVector3D(farPlane.x(), farPlane.y(), farPlane.z());
    m_planes[Far].distance = farPlane.w();
    
    // Normalize all the plane equations
    for (int i = 0; i < PlaneCount; ++i) {
        float length = m_planes[i].normal.length();
        if (length > 0.00001f) {  // Guard against division by zero
            m_planes[i].normal /= length;
            m_planes[i].distance /= length;
        }
    }
    
    // Try/catch to handle potential errors during frustum corner calculation
    try {
        // Calculate the frustum corners for debugging
        calculateFrustumCorners(viewProjection.inverted());
        
        // Debug output (periodically)
        static int debugCounter = 0;
        if (debugCounter++ % 300 == 0) {
            qDebug() << "View Frustum updated:";
            qDebug() << "  - Near corners: " << m_corners[0] << m_corners[1];
            qDebug() << "  - Far corners: " << m_corners[4] << m_corners[5];
        }
    } catch (const std::exception& e) {
        qWarning() << "Error calculating frustum corners:" << e.what();
    } catch (...) {
        qWarning() << "Unknown error calculating frustum corners";
    }
}

void ViewFrustum::calculateFrustumCorners(const QMatrix4x4& invViewProjection) {
    // Safety check
    if (invViewProjection.isIdentity()) {
        qWarning() << "Invalid view-projection matrix for frustum corner calculation";
        return;
    }
    
    // Define the 8 corners of the frustum in NDC space (-1 to 1 cube)
    // Near face
    m_corners[0] = unprojectPoint(QVector3D(-1, -1, -1), invViewProjection); // Near bottom left
    m_corners[1] = unprojectPoint(QVector3D( 1, -1, -1), invViewProjection); // Near bottom right
    m_corners[2] = unprojectPoint(QVector3D(-1,  1, -1), invViewProjection); // Near top left
    m_corners[3] = unprojectPoint(QVector3D( 1,  1, -1), invViewProjection); // Near top right
    
    // Far face
    m_corners[4] = unprojectPoint(QVector3D(-1, -1,  1), invViewProjection); // Far bottom left
    m_corners[5] = unprojectPoint(QVector3D( 1, -1,  1), invViewProjection); // Far bottom right
    m_corners[6] = unprojectPoint(QVector3D(-1,  1,  1), invViewProjection); // Far top left
    m_corners[7] = unprojectPoint(QVector3D( 1,  1,  1), invViewProjection); // Far top right
}

QVector3D ViewFrustum::unprojectPoint(const QVector3D& ndc, const QMatrix4x4& invViewProjection) {
    // Convert from NDC to world space
    QVector4D worldSpace = invViewProjection * QVector4D(ndc, 1.0f);
    if (qAbs(worldSpace.w()) > 0.00001f) { // Use qAbs for more robust comparison
        worldSpace /= worldSpace.w();
    } else {
        // If w is too close to zero, just return the xyz components without division
        qWarning() << "Division by zero avoided in unprojectPoint";
    }
    return worldSpace.toVector3D();
}

bool ViewFrustum::isPointInside(const QVector3D& point) const {
    // Check if the point is on the positive side of all planes
    for (int i = 0; i < PlaneCount; ++i) {
        if (m_planes[i].distanceToPoint(point) < 0) {
            return false;
        }
    }
    return true;
}

bool ViewFrustum::isSphereInside(const QVector3D& center, float radius) const {
    // Safety check for invalid radius
    if (radius <= 0.0f) {
        radius = 0.1f; // Fallback to a minimum radius
    }
    
    // Check if the sphere is on the positive side of all planes
    // or if it intersects any plane
    for (int i = 0; i < PlaneCount; ++i) {
        float distance = m_planes[i].distanceToPoint(center);
        if (distance < -radius) {
            return false;  // Sphere is completely outside this plane
        }
    }
    return true;  // Sphere is inside or intersects the frustum
}

bool ViewFrustum::isBoxInside(const QVector3D& min, const QVector3D& max) const {
    // Check each plane
    for (int i = 0; i < PlaneCount; ++i) {
        // Find the points of the box with the minimum and maximum distances to the plane
        QVector3D positiveVertex;
        QVector3D negativeVertex;
        
        // Determine which corner of the box is facing the plane normal (max distance point)
        positiveVertex.setX(m_planes[i].normal.x() >= 0 ? max.x() : min.x());
        positiveVertex.setY(m_planes[i].normal.y() >= 0 ? max.y() : min.y());
        positiveVertex.setZ(m_planes[i].normal.z() >= 0 ? max.z() : min.z());
        
        // If the positive vertex is outside, the box is outside
        if (m_planes[i].distanceToPoint(positiveVertex) < 0) {
            return false;
        }
    }
    
    return true;  // Box is at least partially inside the frustum
}

bool ViewFrustum::isChunkInside(const ChunkCoordinate& coordinate) const {
    // Convert chunk coordinates to world-space AABB
    QVector3D min = coordinate.getMinCorner();
    QVector3D max = coordinate.getMaxCorner();
    
    // Use box test
    return isBoxInside(min, max);
}