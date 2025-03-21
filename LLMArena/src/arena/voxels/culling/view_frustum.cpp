// src/arena/voxels/culling/view_frustum.cpp
#include "../../../include/arena/voxels/culling/view_frustum.h"
#include <QDebug>

ViewFrustum::ViewFrustum() {
    // Initialize planes to an inside-out frustum
    for (int i = 0; i < PlaneCount; ++i) {
        m_planes[i].normal = QVector3D(0, 0, 0);
        m_planes[i].distance = 0;
    }
}

void ViewFrustum::update(const QMatrix4x4& viewProjection) {
    // Extract frustum planes from the view-projection matrix
    // Method from: http://www8.cs.umu.se/kurser/5DV051/HT12/lab/plane_extraction.pdf
    
    // Left plane
    m_planes[Left].normal.setX(viewProjection(0, 3) + viewProjection(0, 0));
    m_planes[Left].normal.setY(viewProjection(1, 3) + viewProjection(1, 0));
    m_planes[Left].normal.setZ(viewProjection(2, 3) + viewProjection(2, 0));
    m_planes[Left].distance = viewProjection(3, 3) + viewProjection(3, 0);
    
    // Right plane
    m_planes[Right].normal.setX(viewProjection(0, 3) - viewProjection(0, 0));
    m_planes[Right].normal.setY(viewProjection(1, 3) - viewProjection(1, 0));
    m_planes[Right].normal.setZ(viewProjection(2, 3) - viewProjection(2, 0));
    m_planes[Right].distance = viewProjection(3, 3) - viewProjection(3, 0);
    
    // Bottom plane
    m_planes[Bottom].normal.setX(viewProjection(0, 3) + viewProjection(0, 1));
    m_planes[Bottom].normal.setY(viewProjection(1, 3) + viewProjection(1, 1));
    m_planes[Bottom].normal.setZ(viewProjection(2, 3) + viewProjection(2, 1));
    m_planes[Bottom].distance = viewProjection(3, 3) + viewProjection(3, 1);
    
    // Top plane
    m_planes[Top].normal.setX(viewProjection(0, 3) - viewProjection(0, 1));
    m_planes[Top].normal.setY(viewProjection(1, 3) - viewProjection(1, 1));
    m_planes[Top].normal.setZ(viewProjection(2, 3) - viewProjection(2, 1));
    m_planes[Top].distance = viewProjection(3, 3) - viewProjection(3, 1);
    
    // Near plane
    m_planes[Near].normal.setX(viewProjection(0, 3) + viewProjection(0, 2));
    m_planes[Near].normal.setY(viewProjection(1, 3) + viewProjection(1, 2));
    m_planes[Near].normal.setZ(viewProjection(2, 3) + viewProjection(2, 2));
    m_planes[Near].distance = viewProjection(3, 3) + viewProjection(3, 2);
    
    // Far plane
    m_planes[Far].normal.setX(viewProjection(0, 3) - viewProjection(0, 2));
    m_planes[Far].normal.setY(viewProjection(1, 3) - viewProjection(1, 2));
    m_planes[Far].normal.setZ(viewProjection(2, 3) - viewProjection(2, 2));
    m_planes[Far].distance = viewProjection(3, 3) - viewProjection(3, 2);
    
    // Normalize all the plane equations
    for (int i = 0; i < PlaneCount; ++i) {
        float length = m_planes[i].normal.length();
        if (length > 0.00001f) {  // Guard against division by zero
            m_planes[i].normal /= length;
            m_planes[i].distance /= length;
        }
    }
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
        // Check if box is completely outside this plane
        // by finding the point furthest in the direction of the plane normal
        QVector3D positiveVertex(
            m_planes[i].normal.x() >= 0 ? max.x() : min.x(),
            m_planes[i].normal.y() >= 0 ? max.y() : min.y(),
            m_planes[i].normal.z() >= 0 ? max.z() : min.z()
        );
        
        if (m_planes[i].distanceToPoint(positiveVertex) < 0) {
            return false;  // Box is completely outside this plane
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