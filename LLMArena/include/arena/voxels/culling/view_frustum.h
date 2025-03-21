// include/arena/voxels/culling/view_frustum.h
#ifndef VIEW_FRUSTUM_H
#define VIEW_FRUSTUM_H

#include <QMatrix4x4>
#include <array>
#include "../../voxels/chunk/chunk_coordinate.h"

/**
 * @brief Represents the camera's view frustum for culling.
 * 
 * This class extracts the 6 planes that make up the view frustum from
 * the combined view-projection matrix, and provides methods to test if
 * objects are inside the frustum.
 */
class ViewFrustum {
public:
    // Frustum plane indices
    enum Planes {
        Left = 0,
        Right,
        Bottom,
        Top,
        Near,
        Far,
        PlaneCount
    };
    
    /**
     * @brief Default constructor
     */
    ViewFrustum();
    
    /**
     * @brief Update the frustum planes from a view-projection matrix
     * @param viewProjection The combined view-projection matrix
     */
    void update(const QMatrix4x4& viewProjection);
    
    /**
     * @brief Test if a point is inside the frustum
     * @param point The point to test
     * @return True if the point is inside the frustum
     */
    bool isPointInside(const QVector3D& point) const;
    
    /**
     * @brief Test if a sphere is inside or intersects the frustum
     * @param center The center of the sphere
     * @param radius The radius of the sphere
     * @return True if the sphere is inside or intersects the frustum
     */
    bool isSphereInside(const QVector3D& center, float radius) const;
    
    /**
     * @brief Test if an axis-aligned bounding box is inside or intersects the frustum
     * @param min The minimum corner of the box
     * @param max The maximum corner of the box
     * @return True if the box is inside or intersects the frustum
     */
    bool isBoxInside(const QVector3D& min, const QVector3D& max) const;
    
    /**
     * @brief Test if a chunk is inside or intersects the frustum
     * @param coordinate The chunk coordinate
     * @return True if the chunk is inside or intersects the frustum
     */
    bool isChunkInside(const ChunkCoordinate& coordinate) const;
    
private:
    // Frustum planes in form ax + by + cz + d = 0
    // where normal = (a, b, c), d = distance
    struct Plane {
        QVector3D normal;
        float distance;
        
        Plane() : normal(0, 1, 0), distance(0) {}
        
        void setFromPoints(const QVector3D& p1, const QVector3D& p2, const QVector3D& p3) {
            normal = QVector3D::crossProduct(p2 - p1, p3 - p1).normalized();
            distance = -QVector3D::dotProduct(normal, p1);
        }
        
        float distanceToPoint(const QVector3D& point) const {
            return QVector3D::dotProduct(normal, point) + distance;
        }
    };
    
    // Array of 6 planes (left, right, bottom, top, near, far)
    std::array<Plane, PlaneCount> m_planes;
    
    // Normalized device coordinates corners (for AABB tests)
    std::array<QVector3D, 8> m_corners;
};

#endif // VIEW_FRUSTUM_H