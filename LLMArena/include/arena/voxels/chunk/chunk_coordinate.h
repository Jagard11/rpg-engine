// include/arena/voxels/chunk/chunk_coordinate.h
#ifndef CHUNK_COORDINATE_H
#define CHUNK_COORDINATE_H

#include <QVector3D>
#include <functional>
#include <QDebug>

/**
 * @brief Represents the 3D coordinate of a chunk in the world.
 * 
 * A chunk is a 16x16x16 section of the world. This class stores the coordinate
 * of a chunk in the chunk grid. It also provides methods for converting between
 * chunk coordinates and world coordinates.
 */
class ChunkCoordinate {
public:
    // Chunk size constants (should match the octree settings)
    static constexpr int CHUNK_SIZE = 16;
    
    /**
     * @brief Construct a chunk coordinate
     * @param x X coordinate in the chunk grid
     * @param y Y coordinate in the chunk grid
     * @param z Z coordinate in the chunk grid
     */
    ChunkCoordinate(int x = 0, int y = 0, int z = 0) : m_x(x), m_y(y), m_z(z) {}
    
    /**
     * @brief Construct a chunk coordinate from a world position
     * @param worldPosition Position in world coordinates
     */
    static ChunkCoordinate fromWorldPosition(const QVector3D& worldPosition);
    
    /**
     * @brief Convert chunk local coordinates to world coordinates
     * @param localX Local X coordinate (0-15)
     * @param localY Local Y coordinate (0-15)
     * @param localZ Local Z coordinate (0-15)
     * @return Position in world coordinates
     */
    QVector3D toWorldPosition(int localX, int localY, int localZ) const;
    
    /**
     * @brief Get the minimum corner of this chunk in world coordinates
     * @return Minimum corner position
     */
    QVector3D getMinCorner() const;
    
    /**
     * @brief Get the maximum corner of this chunk in world coordinates
     * @return Maximum corner position
     */
    QVector3D getMaxCorner() const;
    
    /**
     * @brief Get the center of this chunk in world coordinates
     * @return Center position
     */
    QVector3D getCenter() const;
    
    /**
     * @brief Calculate the distance to another chunk
     * @param other Other chunk coordinate
     * @return Distance in chunk units
     */
    float distanceTo(const ChunkCoordinate& other) const;
    
    /**
     * @brief Calculate the squared distance to another chunk
     * @param other Other chunk coordinate
     * @return Squared distance in chunk units
     */
    float distanceSquaredTo(const ChunkCoordinate& other) const;
    
    /**
     * @brief Calculate Manhattan distance to another chunk
     * @param other Other chunk coordinate
     * @return Manhattan distance in chunk units
     */
    int manhattanDistanceTo(const ChunkCoordinate& other) const;
    
    /**
     * @brief Check if another chunk is a neighbor (adjacent)
     * @param other Other chunk coordinate
     * @return True if adjacent, false otherwise
     */
    bool isNeighbor(const ChunkCoordinate& other) const;
    
    /**
     * @brief Get a chunk coordinate offset by the given amounts
     * @param dx X offset in chunks
     * @param dy Y offset in chunks
     * @param dz Z offset in chunks
     * @return New chunk coordinate
     */
    ChunkCoordinate offset(int dx, int dy, int dz) const;
    
    /**
     * @brief Get all 26 neighboring chunk coordinates
     * @return Vector of all neighboring coordinates
     */
    std::vector<ChunkCoordinate> getAllNeighbors() const;
    
    /**
     * @brief Get the 6 face-adjacent neighboring chunk coordinates
     * @return Vector of face-adjacent coordinates
     */
    std::vector<ChunkCoordinate> getFaceNeighbors() const;
    
    // Getters
    int getX() const { return m_x; }
    int getY() const { return m_y; }
    int getZ() const { return m_z; }
    
    // Operators
    bool operator==(const ChunkCoordinate& other) const {
        return m_x == other.m_x && m_y == other.m_y && m_z == other.m_z;
    }
    
    bool operator!=(const ChunkCoordinate& other) const {
        return !(*this == other);
    }
    
    // String representation for debugging
    QString toString() const {
        return QString("Chunk(%1,%2,%3)").arg(m_x).arg(m_y).arg(m_z);
    }
    
private:
    int m_x;
    int m_y;
    int m_z;
};

// Hash function for using ChunkCoordinate as key in hash maps
namespace std {
    template<>
    struct hash<ChunkCoordinate> {
        std::size_t operator()(const ChunkCoordinate& k) const {
            // Combine the hash of the three coordinates
            std::size_t h1 = std::hash<int>{}(k.getX());
            std::size_t h2 = std::hash<int>{}(k.getY());
            std::size_t h3 = std::hash<int>{}(k.getZ());
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}

// Debug stream operator
inline QDebug operator<<(QDebug debug, const ChunkCoordinate& c) {
    QDebugStateSaver saver(debug);
    debug.nospace() << c.toString();
    return debug;
}

#endif // CHUNK_COORDINATE_H