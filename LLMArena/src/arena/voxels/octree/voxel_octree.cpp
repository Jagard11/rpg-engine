// src/arena/voxels/octree/voxel_octree.cpp
#include "../../../include/arena/voxels/octree/voxel_octree.h"
#include <queue>
#include <stack>

//-------------------------------------------------------------------------
// VoxelOctree implementation
//-------------------------------------------------------------------------

VoxelOctree::VoxelOctree() {
    // Create the root node
    m_rootNode = std::make_unique<VoxelOctreeNode>();
}

VoxelOctree::~VoxelOctree() {
    // Smart pointers handle cleanup automatically
}

Voxel VoxelOctree::getVoxel(int x, int y, int z) const {
    // Validate coordinates
    if (x < 0 || x >= 16 || y < 0 || y >= 16 || z < 0 || z >= 16) {
        return Voxel(); // Return air for out-of-bounds
    }
    
    // Start at the root node
    VoxelOctreeNode* currentNode = m_rootNode.get();
    
    // Traverse the octree, level by level
    for (int level = 0; level < MAX_OCTREE_DEPTH; level++) {
        // If we've reached a leaf node, return its voxel
        if (currentNode->isLeaf()) {
            return currentNode->getVoxel();
        }
        
        // Calculate which child contains our coordinate
        int childSize = 16 >> (level + 1); // Size of each child at this level
        int childIndex = ((x >= childSize) << 0) | 
                        ((y >= childSize) << 1) | 
                        ((z >= childSize) << 2);
        
        // Adjust coordinates for the child node
        if (x >= childSize) x -= childSize;
        if (y >= childSize) y -= childSize;
        if (z >= childSize) z -= childSize;
        
        // Move to the child node
        currentNode = currentNode->getChild(childIndex);
        
        // If the child doesn't exist, return air
        if (!currentNode) {
            return Voxel();
        }
    }
    
    // At maximum depth, return the voxel
    return currentNode->getVoxel();
}

bool VoxelOctree::setVoxel(int x, int y, int z, const Voxel& voxel) {
    // Validate coordinates
    if (x < 0 || x >= 16 || y < 0 || y >= 16 || z < 0 || z >= 16) {
        return false; // Fail for out-of-bounds
    }
    
    // Start at the root node
    VoxelOctreeNode* currentNode = m_rootNode.get();
    
    // Save parents for potential optimization later
    std::stack<VoxelOctreeNode*> parents;
    
    // Traverse the octree, level by level
    for (int level = 0; level < MAX_OCTREE_DEPTH; level++) {
        // If current node is a leaf and we're not at max depth, 
        // split it if the new voxel is different
        if (currentNode->isLeaf() && level < MAX_OCTREE_DEPTH - 1) {
            // If the current voxel is the same as what we're setting, we're done
            if (currentNode->getVoxel().type == voxel.type && 
                currentNode->getVoxel().color == voxel.color) {
                return false; // No change needed
            }
            
            // Split the node
            currentNode->split();
        }
        
        // If we've reached maximum depth, set the voxel
        if (level == MAX_OCTREE_DEPTH - 1) {
            // Check if voxel is already what we're setting
            if (currentNode->getVoxel().type == voxel.type &&
                currentNode->getVoxel().color == voxel.color) {
                return false; // No change needed
            }
            
            // Set the voxel
            currentNode->setVoxel(voxel);
            
            // Attempt to optimize the tree bottom-up
            bool changed = true;
            while (!parents.empty() && changed) {
                VoxelOctreeNode* parent = parents.top();
                parents.pop();
                changed = parent->tryMerge();
            }
            
            return true;
        }
        
        // Calculate which child contains our coordinate
        int childSize = 16 >> (level + 1); // Size of each child at this level
        int childIndex = ((x >= childSize) << 0) | 
                        ((y >= childSize) << 1) | 
                        ((z >= childSize) << 2);
        
        // Adjust coordinates for the child node
        if (x >= childSize) x -= childSize;
        if (y >= childSize) y -= childSize;
        if (z >= childSize) z -= childSize;
        
        // Save the current node for optimization
        parents.push(currentNode);
        
        // Move to the child node
        currentNode = currentNode->getChild(childIndex);
    }
    
    // Set the voxel on the leaf node
    if (currentNode->getVoxel().type == voxel.type &&
        currentNode->getVoxel().color == voxel.color) {
        return false; // No change needed
    }
    
    currentNode->setVoxel(voxel);
    return true;
}

bool VoxelOctree::isVoxelVisible(int x, int y, int z) const {
    // Get the voxel we're checking
    Voxel voxel = getVoxel(x, y, z);
    
    // Air voxels are never visible
    if (voxel.type == VoxelType::Air) {
        return false;
    }
    
    // Check all 6 neighboring positions
    static const int dx[6] = { 1, -1, 0, 0, 0, 0 };
    static const int dy[6] = { 0, 0, 1, -1, 0, 0 };
    static const int dz[6] = { 0, 0, 0, 0, 1, -1 };
    
    for (int i = 0; i < 6; i++) {
        // Get coordinates of neighbor
        int nx = x + dx[i];
        int ny = y + dy[i];
        int nz = z + dz[i];
        
        // If out of bounds, consider it as "air" (transparent)
        if (nx < 0 || nx >= 16 || ny < 0 || ny >= 16 || nz < 0 || nz >= 16) {
            return true;  // Visible from chunk boundary
        }
        
        // Check if the neighbor is transparent
        Voxel neighbor = getVoxel(nx, ny, nz);
        if (neighbor.type == VoxelType::Air) {
            return true;  // Has transparent neighbor
        }
    }
    
    // No transparent neighbors found, not visible
    return false;
}

std::vector<VoxelPos> VoxelOctree::getVisibleVoxels() const {
    std::vector<VoxelPos> visibleVoxels;
    
    // Iterate through all possible voxel positions
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            for (int z = 0; z < 16; z++) {
                if (isVoxelVisible(x, y, z)) {
                    visibleVoxels.push_back(VoxelPos(x, y, z));
                }
            }
        }
    }
    
    return visibleVoxels;
}

size_t VoxelOctree::calculateMemoryUsage() const {
    // Start with the size of this object
    size_t memoryUsage = sizeof(VoxelOctree);
    
    // Add the memory usage of the root node and all children
    if (m_rootNode) {
        memoryUsage += m_rootNode->calculateMemoryUsage();
    }
    
    return memoryUsage;
}

void VoxelOctree::clear() {
    // Replace the root node with a new empty one
    m_rootNode = std::make_unique<VoxelOctreeNode>();
}

VoxelRaycastResult VoxelOctree::raycast(const QVector3D& origin, const QVector3D& direction, float maxDistance) const {
    VoxelRaycastResult result;
    
    // Convert origin to voxel space (assuming chunk coordinates)
    float ox = origin.x();
    float oy = origin.y();
    float oz = origin.z();
    
    // Voxel position we're examining
    int x = std::floor(ox);
    int y = std::floor(oy);
    int z = std::floor(oz);
    
    // Direction of movement for each axis
    int stepX = (direction.x() >= 0) ? 1 : -1;
    int stepY = (direction.y() >= 0) ? 1 : -1;
    int stepZ = (direction.z() >= 0) ? 1 : -1;
    
    // Distance to next boundary
    float nextBoundaryX = (stepX > 0) ? (x + 1) : x;
    float nextBoundaryY = (stepY > 0) ? (y + 1) : y;
    float nextBoundaryZ = (stepZ > 0) ? (z + 1) : z;
    
    // Parameter values for next boundaries
    float tMaxX = (direction.x() != 0) ? (nextBoundaryX - ox) / direction.x() : std::numeric_limits<float>::max();
    float tMaxY = (direction.y() != 0) ? (nextBoundaryY - oy) / direction.y() : std::numeric_limits<float>::max();
    float tMaxZ = (direction.z() != 0) ? (nextBoundaryZ - oz) / direction.z() : std::numeric_limits<float>::max();
    
    // Parameter increments
    float tDeltaX = (direction.x() != 0) ? stepX / direction.x() : std::numeric_limits<float>::max();
    float tDeltaY = (direction.y() != 0) ? stepY / direction.y() : std::numeric_limits<float>::max();
    float tDeltaZ = (direction.z() != 0) ? stepZ / direction.z() : std::numeric_limits<float>::max();
    
    // Face index for collision
    int face = -1;
    
    // Distance traveled
    float distance = 0.0f;
    
    // Loop until we hit something or exceed max distance
    while (distance < maxDistance) {
        // Check current voxel if it's in bounds
        if (x >= 0 && x < 16 && y >= 0 && y < 16 && z >= 0 && z < 16) {
            Voxel voxel = getVoxel(x, y, z);
            
            // If not air, we hit something
            if (voxel.type != VoxelType::Air) {
                // Calculate exact hit point
                QVector3D hitPoint = origin + direction * distance;
                
                // Fill result
                result.hit = true;
                result.position = VoxelPos(x, y, z);
                result.face = face;
                result.hitPoint = hitPoint;
                result.distance = distance;
                result.voxel = voxel;
                
                // Set normal based on face
                switch (face) {
                    case 0: result.normal = QVector3D(1, 0, 0); break;  // +X
                    case 1: result.normal = QVector3D(-1, 0, 0); break; // -X
                    case 2: result.normal = QVector3D(0, 1, 0); break;  // +Y
                    case 3: result.normal = QVector3D(0, -1, 0); break; // -Y
                    case 4: result.normal = QVector3D(0, 0, 1); break;  // +Z
                    case 5: result.normal = QVector3D(0, 0, -1); break; // -Z
                }
                
                return result;
            }
        }
        
        // Move to next voxel
        if (tMaxX < tMaxY && tMaxX < tMaxZ) {
            // X axis crossing
            distance = tMaxX;
            tMaxX += tDeltaX;
            x += stepX;
            face = (stepX > 0) ? 1 : 0; // -X or +X face
        } else if (tMaxY < tMaxZ) {
            // Y axis crossing
            distance = tMaxY;
            tMaxY += tDeltaY;
            y += stepY;
            face = (stepY > 0) ? 3 : 2; // -Y or +Y face
        } else {
            // Z axis crossing
            distance = tMaxZ;
            tMaxZ += tDeltaZ;
            z += stepZ;
            face = (stepZ > 0) ? 5 : 4; // -Z or +Z face
        }
        
        // If we've left the chunk bounds, stop
        if (x < 0 || x >= 16 || y < 0 || y >= 16 || z < 0 || z >= 16) {
            break;
        }
    }
    
    // No hit
    return result;
}

bool VoxelOctree::isEmpty() const {
    // The tree is empty if the root node is a leaf and contains air
    return m_rootNode->isEmpty();
}

void VoxelOctree::optimize() {
    // Start bottom-up merging from the root
    m_rootNode->tryMerge();
}

int VoxelOctree::posToIndex(int x, int y, int z, int level) const {
    // Calculate child size at this level
    int childSize = 16 >> (level + 1);
    
    // Compute relative indices
    int ix = x / childSize;
    int iy = y / childSize;
    int iz = z / childSize;
    
    // Combine into octant index
    return (ix << 0) | (iy << 1) | (iz << 2);
}

void VoxelOctree::indexToPos(int index, int level, int& x, int& y, int& z) const {
    // Calculate child size at this level
    int childSize = 16 >> (level + 1);
    
    // Extract individual flags
    x = (index & 1) ? childSize : 0;
    y = (index & 2) ? childSize : 0;
    z = (index & 4) ? childSize : 0;
}

//-------------------------------------------------------------------------
// VoxelOctreeNode implementation
//-------------------------------------------------------------------------

VoxelOctreeNode::VoxelOctreeNode() 
    : m_isLeaf(true), m_voxel(VoxelType::Air, QColor(0, 0, 0, 0)) {
    // Initialize as an empty leaf node
}

VoxelOctreeNode::~VoxelOctreeNode() {
    // Smart pointers handle cleanup automatically
}

Voxel VoxelOctreeNode::getVoxel() const {
    return m_voxel;
}

void VoxelOctreeNode::setVoxel(const Voxel& voxel) {
    m_voxel = voxel;
}

VoxelOctreeNode* VoxelOctreeNode::getChild(int index) const {
    // Validate index
    if (index < 0 || index >= 8 || m_isLeaf) {
        return nullptr;
    }
    
    return m_children[index].get();
}

void VoxelOctreeNode::split() {
    // Only split leaf nodes
    if (!m_isLeaf) {
        return;
    }
    
    // Create 8 child nodes with the same voxel as this one
    for (int i = 0; i < 8; i++) {
        m_children[i] = std::make_unique<VoxelOctreeNode>();
        m_children[i]->setVoxel(m_voxel);
    }
    
    // This is no longer a leaf
    m_isLeaf = false;
}

bool VoxelOctreeNode::tryMerge() {
    // Only branch nodes can be merged
    if (m_isLeaf) {
        return false;
    }
    
    // Check if all children are leaves with the same voxel
    Voxel firstVoxel;
    bool allSame = true;
    
    for (int i = 0; i < 8; i++) {
        // Child must exist and be a leaf
        if (!m_children[i] || !m_children[i]->isLeaf()) {
            allSame = false;
            break;
        }
        
        // Check voxel type and color
        if (i == 0) {
            firstVoxel = m_children[i]->getVoxel();
        } else if (m_children[i]->getVoxel().type != firstVoxel.type ||
                  m_children[i]->getVoxel().color != firstVoxel.color) {
            allSame = false;
            break;
        }
    }
    
    // If all children are the same, merge
    if (allSame) {
        m_voxel = firstVoxel;
        
        // Reset all children
        for (int i = 0; i < 8; i++) {
            m_children[i].reset();
        }
        
        m_isLeaf = true;
        return true;
    }
    
    // Try merging each child recursively
    bool anyChanged = false;
    for (int i = 0; i < 8; i++) {
        if (m_children[i] && !m_children[i]->isLeaf()) {
            anyChanged |= m_children[i]->tryMerge();
        }
    }
    
    return anyChanged;
}

size_t VoxelOctreeNode::calculateMemoryUsage() const {
    // Start with the size of this node
    size_t memoryUsage = sizeof(VoxelOctreeNode);
    
    // If not a leaf, add the memory usage of all children
    if (!m_isLeaf) {
        for (int i = 0; i < 8; i++) {
            if (m_children[i]) {
                memoryUsage += m_children[i]->calculateMemoryUsage();
            }
        }
    }
    
    return memoryUsage;
}

bool VoxelOctreeNode::isEmpty() const {
    // If leaf, check if it's air
    if (m_isLeaf) {
        return m_voxel.type == VoxelType::Air;
    }
    
    // If branch, check if all children are empty
    for (int i = 0; i < 8; i++) {
        if (m_children[i] && !m_children[i]->isEmpty()) {
            return false;
        }
    }
    
    return true;
}