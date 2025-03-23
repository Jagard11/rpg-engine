// src/arena/ui/gl_widgets/gl_arena_highlighting.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include <QDebug>
#include <cmath>

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

// Raycast to find voxel under cursor
void GLArenaWidget::raycastVoxels(const QVector3D& origin, const QVector3D& direction) {
    // Enhanced safety checks
    if (!m_voxelSystem || !m_voxelSystem->getWorld()) {
        m_highlightedVoxelFace = -1; // Reset highlight
        return;
    }
    
    try {
        // Normalize direction
        QVector3D dir = direction.normalized();
        
        // Simple raycast implementation
        // Step along the ray checking for voxels at each step
        float step = 0.1f; // Small step size for accuracy
        float maxDistance = m_maxPlacementDistance;
        
        m_highlightedVoxelFace = -1; // Reset highlight
        
        for (float distance = 0.0f; distance < maxDistance; distance += step) {
            QVector3D pos = origin + dir * distance;
            
            // Make sure position is valid
            if (!pos.isNull() && qIsFinite(pos.x()) && qIsFinite(pos.y()) && qIsFinite(pos.z())) {
                // Convert to voxel coordinates
                VoxelPos voxelPos(floor(pos.x()), floor(pos.y()), floor(pos.z()));
                
                // Check if voxel position is valid
                if (voxelPos.isValid()) {
                    // Check if we hit a solid voxel
                    Voxel voxel = m_voxelSystem->getWorld()->getVoxel(voxelPos);
                    
                    if (voxel.type != VoxelType::Air) {
                        // We hit a voxel - determine which face
                        // Calculate exact hit point
                        QVector3D hitPoint = origin + dir * (distance - step/2);
                        
                        // Calculate offsets from voxel center
                        float xOffset = hitPoint.x() - (voxelPos.x + 0.5f);
                        float yOffset = hitPoint.y() - (voxelPos.y + 0.5f);
                        float zOffset = hitPoint.z() - (voxelPos.z + 0.5f);
                        
                        // Determine which face was hit based on largest offset
                        if (fabs(xOffset) > fabs(yOffset) && fabs(xOffset) > fabs(zOffset)) {
                            m_highlightedVoxelFace = (xOffset > 0) ? 0 : 1; // +X or -X face
                        } else if (fabs(yOffset) > fabs(xOffset) && fabs(yOffset) > fabs(zOffset)) {
                            m_highlightedVoxelFace = (yOffset > 0) ? 2 : 3; // +Y or -Y face
                        } else {
                            m_highlightedVoxelFace = (zOffset > 0) ? 4 : 5; // +Z or -Z face
                        }
                        
                        m_highlightedVoxelPos = QVector3D(voxelPos.x, voxelPos.y, voxelPos.z);
                        break;
                    }
                }
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
            default: return; // Invalid face
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