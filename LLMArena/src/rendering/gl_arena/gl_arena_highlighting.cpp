// src/rendering/gl_arena/gl_arena_highlighting.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include <QDebug>
#include <cmath>
#include <limits> // For std::numeric_limits

// Render a highlight box around the selected voxel face
void GLArenaWidget::renderVoxelHighlight() {
    // Enhanced safety checks
    if (m_highlightedVoxelFace < 0 || m_highlightedVoxelFace >= 6 || 
        !m_voxelSystem || !m_voxelSystem->getWorld() || !m_billboardProgram) {
        return; // No highlighted voxel or invalid state
    }
    
    try {
        // Save OpenGL state
        GLboolean depthWriteEnabled;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteEnabled);
        
        GLint oldBlendSrc, oldBlendDst;
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &oldBlendSrc);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &oldBlendDst);
        
        // Set up OpenGL state for rendering highlights
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE); // Disable depth writing for the highlight
        
        // Set up shader for wireframe rendering
        if (!m_billboardProgram->bind()) {
            qWarning() << "Failed to bind shader for voxel highlight";
            return;
        }
        
        // Set up matrices
        m_billboardProgram->setUniformValue("view", m_viewMatrix);
        m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
        
        // Set model matrix for the voxel position
        QMatrix4x4 modelMatrix;
        modelMatrix.setToIdentity();
        modelMatrix.translate(m_highlightedVoxelPos);
        m_billboardProgram->setUniformValue("modelView", modelMatrix);
        
        // Render wireframe cube for highlight
        // For a full implementation, we would create proper geometry for this
        // This is a simplified implementation using lines
        
        // Define the 8 corners of the cube (slightly larger than voxel)
        const float size = 1.02f;
        const float halfSize = size / 2.0f;
        
        QVector3D corners[8] = {
            // Front face corners
            QVector3D(-halfSize, -halfSize,  halfSize),
            QVector3D( halfSize, -halfSize,  halfSize),
            QVector3D( halfSize,  halfSize,  halfSize),
            QVector3D(-halfSize,  halfSize,  halfSize),
            // Back face corners
            QVector3D(-halfSize, -halfSize, -halfSize),
            QVector3D( halfSize, -halfSize, -halfSize),
            QVector3D( halfSize,  halfSize, -halfSize),
            QVector3D(-halfSize,  halfSize, -halfSize)
        };
        
        // Define the 12 edges of the cube
        const int edges[12][2] = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Front face
            {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Back face
            {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Connecting edges
        };
        
        // Transform the corners to world space
        for (int i = 0; i < 8; i++) {
            corners[i] = modelMatrix * corners[i];
        }
        
        // Set wireframe color (white)
        m_billboardProgram->setUniformValue("color", QVector4D(1.0f, 1.0f, 1.0f, 1.0f));
        
        // Draw edges as lines
        // For a full implementation, we should use VAOs and VBOs
        // This is a simplified direct-mode style drawing for demonstration
        
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        for (int i = 0; i < 12; i++) {
            const QVector3D& start = corners[edges[i][0]];
            const QVector3D& end = corners[edges[i][1]];
            
            glVertex3f(start.x(), start.y(), start.z());
            glVertex3f(end.x(), end.y(), end.z());
        }
        glEnd();
        
        // Draw highlighted face if any
        if (m_highlightedVoxelFace >= 0 && m_highlightedVoxelFace < 6) {
            // Define the 4 corners of each face
            const int faces[6][4] = {
                {1, 2, 6, 5}, // +X face
                {0, 3, 7, 4}, // -X face
                {3, 2, 6, 7}, // +Y face
                {0, 1, 5, 4}, // -Y face
                {0, 1, 2, 3}, // +Z face
                {4, 5, 6, 7}  // -Z face
            };
            
            // Set face color (semi-transparent white)
            m_billboardProgram->setUniformValue("color", QVector4D(1.0f, 1.0f, 1.0f, 0.3f));
            
            // Draw highlighted face as a quad
            glBegin(GL_QUADS);
            for (int i = 0; i < 4; i++) {
                const QVector3D& vertex = corners[faces[m_highlightedVoxelFace][i]];
                glVertex3f(vertex.x(), vertex.y(), vertex.z());
            }
            glEnd();
        }
        
        // Unbind shader
        m_billboardProgram->release();
        
        // Restore OpenGL state
        if (depthWriteEnabled) {
            glDepthMask(GL_TRUE);
        }
        
        glBlendFunc(oldBlendSrc, oldBlendDst);
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in renderVoxelHighlight:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception in renderVoxelHighlight";
    }
}

// Raycast to find voxel under cursor - using Digital Differential Analyzer (DDA) algorithm
void GLArenaWidget::raycastVoxels(const QVector3D& origin, const QVector3D& direction) {
    // Safety check - return early if voxel system or world is not available
    if (!m_voxelSystem || !m_voxelSystem->getWorld()) {
        m_highlightedVoxelFace = -1; // Reset highlight
        return;
    }
    
    try {
        // Normalize direction vector and perform validation
        QVector3D dir = direction;
        if (dir.length() < 0.0001f) {
            // If direction vector is too small, reset highlight and return
            m_highlightedVoxelFace = -1;
            return;
        }
        dir.normalize();
        
        // Reset highlight state at the beginning to ensure clean state
        m_highlightedVoxelFace = -1;
        m_highlightedVoxelPos = QVector3D(0, 0, 0);
        
        // Input validation
        if (!qIsFinite(origin.x()) || !qIsFinite(origin.y()) || !qIsFinite(origin.z()) ||
            !qIsFinite(dir.x()) || !qIsFinite(dir.y()) || !qIsFinite(dir.z())) {
            return; // Invalid input
        }
        
        // Initialize voxel position at ray origin
        int currentX = floor(origin.x());
        int currentY = floor(origin.y());
        int currentZ = floor(origin.z());
        
        // Calculate step direction (-1, 0, or 1) for each axis
        int stepX = (dir.x() > 0) ? 1 : ((dir.x() < 0) ? -1 : 0);
        int stepY = (dir.y() > 0) ? 1 : ((dir.y() < 0) ? -1 : 0);
        int stepZ = (dir.z() > 0) ? 1 : ((dir.z() < 0) ? -1 : 0);
        
        // Prevent division by zero by using small epsilon values
        const float epsilon = 0.0001f;
        float dirX = (qAbs(dir.x()) < epsilon) ? (dir.x() >= 0 ? epsilon : -epsilon) : dir.x();
        float dirY = (qAbs(dir.y()) < epsilon) ? (dir.y() >= 0 ? epsilon : -epsilon) : dir.y();
        float dirZ = (qAbs(dir.z()) < epsilon) ? (dir.z() >= 0 ? epsilon : -epsilon) : dir.z();
        
        // Calculate initial tMax values - distance to first voxel boundary
        float tMaxX = (stepX == 0) ? std::numeric_limits<float>::max() : 
                      (stepX > 0) ? 
                      ((currentX + 1.0f - origin.x()) / dirX) : 
                      ((origin.x() - currentX) / -dirX);
        
        float tMaxY = (stepY == 0) ? std::numeric_limits<float>::max() : 
                      (stepY > 0) ? 
                      ((currentY + 1.0f - origin.y()) / dirY) : 
                      ((origin.y() - currentY) / -dirY);
        
        float tMaxZ = (stepZ == 0) ? std::numeric_limits<float>::max() : 
                      (stepZ > 0) ? 
                      ((currentZ + 1.0f - origin.z()) / dirZ) : 
                      ((origin.z() - currentZ) / -dirZ);
        
        // Calculate tDelta values - how far along ray to move for one voxel step
        float tDeltaX = (stepX == 0) ? std::numeric_limits<float>::max() : (1.0f / qAbs(dirX));
        float tDeltaY = (stepY == 0) ? std::numeric_limits<float>::max() : (1.0f / qAbs(dirY));
        float tDeltaZ = (stepZ == 0) ? std::numeric_limits<float>::max() : (1.0f / qAbs(dirZ));
        
        // Track which face of the voxel was entered
        int enteredFace = -1;
        
        // Maximum distance and iteration limit to prevent infinite loops
        float maxDistance = m_maxPlacementDistance;
        float totalDistance = 0.0f;
        int maxIterations = 100;
        int iterations = 0;
        
        // DDA algorithm main loop
        while (totalDistance < maxDistance && iterations < maxIterations) {
            iterations++;
            
            // Check current voxel for solid block
            VoxelPos voxelPos(currentX, currentY, currentZ);
            
            // Validate voxel position
            if (voxelPos.isValid()) {
                Voxel voxel = m_voxelSystem->getWorld()->getVoxel(voxelPos);
                
                // If we found a solid voxel, set highlight and exit loop
                if (voxel.type != VoxelType::Air) {
                    m_highlightedVoxelPos = QVector3D(currentX, currentY, currentZ);
                    m_highlightedVoxelFace = enteredFace;
                    
                    // Safety validation for face index
                    if (m_highlightedVoxelFace < 0 || m_highlightedVoxelFace > 5) {
                        m_highlightedVoxelFace = 0; // Default to safe value
                    }
                    
                    break;
                }
            }
            
            // Find axis with minimum tMax to determine next voxel boundary to cross
            if (tMaxX < tMaxY && tMaxX < tMaxZ) {
                // X-axis boundary is closest
                totalDistance = tMaxX;
                currentX += stepX;
                tMaxX += tDeltaX;
                // -X face when stepping positively, +X face when stepping negatively
                enteredFace = (stepX > 0) ? 1 : 0;
            } 
            else if (tMaxY < tMaxZ) {
                // Y-axis boundary is closest
                totalDistance = tMaxY;
                currentY += stepY;
                tMaxY += tDeltaY;
                // -Y face when stepping positively, +Y face when stepping negatively
                enteredFace = (stepY > 0) ? 3 : 2;
            } 
            else {
                // Z-axis boundary is closest
                totalDistance = tMaxZ;
                currentZ += stepZ;
                tMaxZ += tDeltaZ;
                // -Z face when stepping positively, +Z face when stepping negatively
                enteredFace = (stepZ > 0) ? 5 : 4;
            }
        }
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in raycastVoxels:" << e.what();
        m_highlightedVoxelFace = -1; // Reset highlight on error
    }
    catch (...) {
        qWarning() << "Unknown exception in raycastVoxels";
        m_highlightedVoxelFace = -1; // Reset highlight on error
    }
}

// Place a voxel at the highlighted face
void GLArenaWidget::placeVoxel() {
    // Enhanced safety checks
    if (!m_voxelSystem || !m_voxelSystem->getWorld() || 
        !m_inventoryUI || m_highlightedVoxelFace < 0 || m_highlightedVoxelFace >= 6) {
        return;
    }
    
    try {
        // Get the selected voxel type
        VoxelType voxelType = m_inventoryUI->getSelectedVoxelType();
        if (voxelType == VoxelType::Air) {
            return;
        }
        
        // Calculate position for new voxel based on highlighted face
        QVector3D newPos = m_highlightedVoxelPos;
        
        // Ensure position is valid
        if (!qIsFinite(newPos.x()) || !qIsFinite(newPos.y()) || !qIsFinite(newPos.z())) {
            qWarning() << "Invalid voxel position in placeVoxel";
            return;
        }
        
        // Adjust position based on face
        switch (m_highlightedVoxelFace) {
            case 0: newPos.setX(newPos.x() + 1); break; // +X face
            case 1: newPos.setX(newPos.x() - 1); break; // -X face
            case 2: newPos.setY(newPos.y() + 1); break; // +Y face
            case 3: newPos.setY(newPos.y() - 1); break; // -Y face
            case 4: newPos.setZ(newPos.z() + 1); break; // +Z face
            case 5: newPos.setZ(newPos.z() - 1); break; // -Z face
            default: 
                qWarning() << "Invalid face in placeVoxel:" << m_highlightedVoxelFace;
                return; // Invalid face
        }
        
        // Convert to voxel position
        VoxelPos voxelPos(newPos.x(), newPos.y(), newPos.z());
        
        // Ensure voxel position is valid
        if (!voxelPos.isValid()) {
            qWarning() << "Invalid voxel position after adjustment in placeVoxel";
            return;
        }
        
        // Check if position is occupied
        Voxel existingVoxel = m_voxelSystem->getWorld()->getVoxel(voxelPos);
        if (existingVoxel.type != VoxelType::Air) {
            return; // Position already occupied
        }
        
        // Get color for the voxel type (basic colors for now)
        QColor voxelColor;
        switch (voxelType) {
            case VoxelType::Dirt:
                voxelColor = QColor(139, 69, 19); // Brown
                break;
            case VoxelType::Grass:
                voxelColor = QColor(34, 139, 34); // Green
                break;
            case VoxelType::Cobblestone:
                voxelColor = QColor(128, 128, 128); // Gray
                break;
            default:
                voxelColor = QColor(255, 255, 255); // White
                break;
        }
        
        // Create voxel
        Voxel newVoxel(voxelType, voxelColor);
        
        // Set voxel in world
        m_voxelSystem->getWorld()->setVoxel(voxelPos, newVoxel);
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in placeVoxel:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception in placeVoxel";
    }
}

// Remove a voxel at the highlighted position
void GLArenaWidget::removeVoxel() {
    // Enhanced safety checks
    if (!m_voxelSystem || !m_voxelSystem->getWorld() || 
        !m_inventoryUI || m_highlightedVoxelFace < 0 || m_highlightedVoxelFace >= 6) {
        return;
    }
    
    try {
        // Ensure position is valid
        if (!qIsFinite(m_highlightedVoxelPos.x()) || 
            !qIsFinite(m_highlightedVoxelPos.y()) || 
            !qIsFinite(m_highlightedVoxelPos.z())) {
            qWarning() << "Invalid voxel position in removeVoxel";
            return;
        }
        
        // Convert to voxel position
        VoxelPos voxelPos(m_highlightedVoxelPos.x(), m_highlightedVoxelPos.y(), m_highlightedVoxelPos.z());
        
        // Ensure voxel position is valid
        if (!voxelPos.isValid()) {
            qWarning() << "Invalid voxel position in removeVoxel";
            return;
        }
        
        // Check if position contains a voxel
        Voxel existingVoxel = m_voxelSystem->getWorld()->getVoxel(voxelPos);
        if (existingVoxel.type == VoxelType::Air) {
            return; // Nothing to remove
        }
        
        // Create air voxel
        Voxel airVoxel(VoxelType::Air, QColor(0, 0, 0, 0));
        
        // Set voxel in world
        m_voxelSystem->getWorld()->setVoxel(voxelPos, airVoxel);
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in removeVoxel:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception in removeVoxel";
    }
}