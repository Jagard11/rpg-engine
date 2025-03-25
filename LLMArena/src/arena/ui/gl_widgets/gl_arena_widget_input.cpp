// src/arena/ui/gl_widgets/gl_arena_widget_input.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include <QDebug>
#include <QCursor>

/*
// This implementation conflicts with gl_arena_widget_key_events.cpp
// Comment out to avoid conflicts
void GLArenaWidget::keyPressEvent(QKeyEvent* event) {
    // First, try debug key handling
    if (processDebugKeyEvent(event)) {
        return; // Event was handled by debug system
    }
    
    // Check for Escape key to toggle escape menu
    if (event->key() == Qt::Key_Escape) {
        // If escape menu exists, toggle it
        if (m_escapeMenu) {
            toggleEscapeMenu();
            return;
        }
        
        // Fallback behavior: toggle cursor capture
        setCursor(cursor().shape() == Qt::BlankCursor ? Qt::ArrowCursor : Qt::BlankCursor);
        return;
    }
    
    // Toggle inventory with I key
    if (event->key() == Qt::Key_I && m_inventoryUI) {
        m_inventoryUI->setVisible(!m_inventoryUI->isVisible());
        updateMouseTrackingState();
        return;
    }
    
    // Toggle debug console with ~ key
    if (event->key() == Qt::Key_AsciiTilde) {
        toggleDebugConsole();
        return;
    }
    
    // Voxel placement controls
    if (event->key() == Qt::Key_E && m_voxelSystem) {
        placeVoxel();
        return;
    }
    
    if (event->key() == Qt::Key_Q && m_voxelSystem) {
        removeVoxel();
        return;
    }
    
    // If inventory is visible, let it handle the key
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleKeyPress(event->key());
        return;
    }
    
    // Pass along to player controller
    m_playerController->handleKeyPress(event);
}

void GLArenaWidget::keyReleaseEvent(QKeyEvent* event) {
    // Skip if inventory is visible
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        return;
    }
    
    // Pass along to player controller
    m_playerController->handleKeyRelease(event);
}
*/

void GLArenaWidget::mouseMoveEvent(QMouseEvent* event) {
    // Handle mouse movement for inventory if it's visible
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMouseMove(event->x(), event->y());
        return;
    }
    
    // Check if cursor is captured
    if (cursor().shape() == Qt::BlankCursor) {
        // Pass mouse movement to player controller
        m_playerController->handleMouseMove(event);
        
        // Reset cursor to center of widget to prevent hitting screen edges
        QCursor::setPos(mapToGlobal(QPoint(width()/2, height()/2)));
    }
    
    // Raycast for voxel highlighting
    if (m_voxelSystem) {
        QVector3D origin = m_playerController->getPosition();
        
        // Calculate forward vector from player rotation
        float yaw = m_playerController->getRotation();
        float pitch = m_playerController->getPitch();
        
        QVector3D direction(
            cos(yaw) * cos(pitch),
            sin(pitch),
            sin(yaw) * cos(pitch)
        );
        
        raycastVoxels(origin, direction);
    }
}

void GLArenaWidget::mousePressEvent(QMouseEvent* event) {
    // Handle inventory first if visible
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMousePress(event->x(), event->y(), event->button());
        return;
    }
    
    // Handle voxel placement/removal
    if (m_voxelSystem) {
        if (event->button() == Qt::LeftButton) {
            placeVoxel();
        } else if (event->button() == Qt::RightButton) {
            removeVoxel();
        }
    }
}

void GLArenaWidget::mouseReleaseEvent(QMouseEvent* event) {
    // Handle inventory first if visible
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMouseRelease(event->x(), event->y(), event->button());
        return;
    }
}

void GLArenaWidget::onPlayerPositionChanged(const QVector3D& position) {
    // Stream voxel chunks around the player
    if (m_voxelSystem) {
        m_voxelSystem->streamChunksAroundPlayer(position);
    }
    
    // Update rendering
    update();
    
    // Emit signal for external components
    emit playerPositionUpdated(position.x(), position.y(), position.z());
}

void GLArenaWidget::onPlayerRotationChanged(float rotation) {
    // Update rendering
    update();
}

void GLArenaWidget::onPlayerPitchChanged(float pitch) {
    // Update rendering
    update();
}

void GLArenaWidget::raycastVoxels(const QVector3D& origin, const QVector3D& direction) {
    if (!m_voxelSystem) {
        return;
    }
    
    // Perform raycast to find voxel under cursor
    QVector3D hitPos, hitNormal;
    Voxel hitVoxel;
    if (m_voxelSystem->raycast(origin, direction, m_maxPlacementDistance, hitPos, hitNormal, hitVoxel)) {
        // Determine which face was hit
        int face = -1;
        
        // Calculate which face was hit based on normal
        if (hitNormal.x() > 0.5f) face = 0;      // +X
        else if (hitNormal.x() < -0.5f) face = 1; // -X
        else if (hitNormal.y() > 0.5f) face = 2;  // +Y
        else if (hitNormal.y() < -0.5f) face = 3; // -Y
        else if (hitNormal.z() > 0.5f) face = 4;  // +Z
        else if (hitNormal.z() < -0.5f) face = 5; // -Z
        
        // Set highlighted voxel
        m_highlightedVoxelPos = hitPos;
        m_highlightedVoxelFace = face;
        
        // Set highlight in voxel system
        m_voxelSystem->setVoxelHighlight(VoxelPos::fromVector3D(hitPos), face);
    } else {
        // No hit, clear highlight
        m_highlightedVoxelFace = -1;
        m_voxelSystem->setVoxelHighlight(VoxelPos(), -1);
    }
}

void GLArenaWidget::placeVoxel() {
    if (!m_voxelSystem || !m_inventory || !m_inventoryUI || m_highlightedVoxelFace < 0) {
        return;
    }
    
    // Get selected voxel type from inventory
    VoxelType voxelType = m_inventoryUI->getSelectedVoxelType();
    if (voxelType == VoxelType::Air) {
        return; // Cannot place air
    }
    
    // Create voxel
    Voxel voxel(voxelType, QColor(255, 255, 255));
    
    // Get normal direction based on face
    QVector3D normal;
    switch (m_highlightedVoxelFace) {
        case 0: normal = QVector3D(1, 0, 0); break;  // +X
        case 1: normal = QVector3D(-1, 0, 0); break; // -X
        case 2: normal = QVector3D(0, 1, 0); break;  // +Y
        case 3: normal = QVector3D(0, -1, 0); break; // -Y
        case 4: normal = QVector3D(0, 0, 1); break;  // +Z
        case 5: normal = QVector3D(0, 0, -1); break; // -Z
        default: return;
    }
    
    // Place voxel
    if (m_voxelSystem->placeVoxel(m_highlightedVoxelPos, normal, voxel)) {
        // Trigger redraw
        update();
    }
}

void GLArenaWidget::removeVoxel() {
    if (!m_voxelSystem || m_highlightedVoxelFace < 0) {
        return;
    }
    
    // Remove voxel
    if (m_voxelSystem->removeVoxel(m_highlightedVoxelPos)) {
        // Trigger redraw
        update();
    }
}

void GLArenaWidget::initializeInventory() {
    // Create inventory if not exists
    if (!m_inventory) {
        m_inventory = new Inventory(this);
    }
    
    // Create inventory UI if not exists
    if (!m_inventoryUI) {
        m_inventoryUI = new InventoryUI(m_inventory, this);
        
        // Initialize OpenGL resources for inventory UI
        m_inventoryUI->initialize();
        
        // Connect signals
        connect(m_inventoryUI, &InventoryUI::visibilityChanged,
                this, &GLArenaWidget::onInventoryVisibilityChanged);
    }
    
    // Set initial visibility
    m_inventoryUI->setVisible(false);
    updateMouseTrackingState();
}

void GLArenaWidget::renderInventory() {
    if (m_inventoryUI) {
        m_inventoryUI->render(width(), height());
    }
}

void GLArenaWidget::onInventoryVisibilityChanged(bool visible) {
    // Update mouse tracking state
    updateMouseTrackingState();
    
    // Update display
    update();
}