// src/arena/ui/gl_widgets/gl_arena_widget_hooks.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include <QDebug>

// This file contains hook implementations for the GLArenaWidget
// These hooks connect the widget with various subsystems

// Initialize GL function
void GLArenaWidget::initializeGL() {
    // Initialize OpenGL functions
    initializeOpenGLFunctions();
    
    try {
        // Print OpenGL version info
        qDebug() << "OpenGL Initialization:";
        qDebug() << "  Vendor:" << reinterpret_cast<const char*>(glGetString(GL_VENDOR));
        qDebug() << "  Renderer:" << reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        qDebug() << "  Version:" << reinterpret_cast<const char*>(glGetString(GL_VERSION));
        qDebug() << "  GLSL Version:" << reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
        
        // Set up OpenGL state
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        
        // Initialize shaders
        if (!initShaders()) {
            qWarning() << "Failed to initialize shaders";
            return;
        }
        
        // Create basic geometries
        createFloor(10.0);
        createGrid(20.0, 20);
        
        // Initialize voxel system
        m_voxelSystem = new VoxelSystemIntegration(m_gameScene, this);
        m_voxelSystem->initialize();
        m_voxelSystem->createDefaultWorld();
        
        // Initialize inventory system
        initializeInventory();
        
        // Initialize debug system - THIS IS THE NEW DEBUG HOOK
        initializeDebugSystem();
        
        // Set initialized flag
        m_initialized = true;
        
        // Emit signal that rendering is initialized
        emit renderingInitialized();
        
        qDebug() << "OpenGL initialization complete";
    } catch (const std::exception& e) {
        qCritical() << "Exception during OpenGL initialization:" << e.what();
        m_initialized = false;
    } catch (...) {
        qCritical() << "Unknown exception during OpenGL initialization";
        m_initialized = false;
    }
    
    // Set up update timer to redraw
    QTimer::singleShot(100, this, [this]() {
        this->update();
    });
}

// Paint GL function
void GLArenaWidget::paintGL() {
    if (!m_initialized) {
        // If not initialized, just clear the screen and return
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return;
    }
    
    try {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Set up view and projection matrices
        QMatrix4x4 viewMatrix = m_viewMatrix;
        QMatrix4x4 projectionMatrix = m_projectionMatrix;
        
        // Render voxel world
        if (m_voxelSystem) {
            m_voxelSystem->render(viewMatrix, projectionMatrix);
        }
        
        // Render characters
        renderCharacters();
        
        // Render voxel highlight
        renderVoxelHighlight();
        
        // Render inventory UI
        renderInventory();
        
        // Render debug overlays - THIS IS THE NEW DEBUG HOOK
        renderDebugOverlays();
    } catch (const std::exception& e) {
        qWarning() << "Exception during rendering:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception during rendering";
    }
    
    // Schedule next frame
    update();
}

// Key press event handler
void GLArenaWidget::keyPressEvent(QKeyEvent* event) {
    if (!event) return;
    
    // Check if debug system handles this key press - THIS IS THE NEW DEBUG HOOK
    if (handleDebugKeyPress(event)) {
        return; // Debug system handled the key, stop processing
    }
    
    // If inventory is open, only handle inventory-related keys
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleKeyPress(event->key());
        return;
    }
    
    // Handle inventory toggle key
    if (event->key() == Qt::Key_I) {
        if (m_inventoryUI) {
            m_inventoryUI->setVisible(!m_inventoryUI->isVisible());
        }
        return;
    }
    
    // Handle voxel placement keys
    if (event->key() == Qt::Key_F) {
        placeVoxel();
        return;
    }
    if (event->key() == Qt::Key_G) {
        removeVoxel();
        return;
    }
    
    // Debug visualizer toggle (moved from old V key to Z key to avoid conflict)
    if (event->key() == Qt::Key_Z && event->modifiers() & Qt::ControlModifier) {
        toggleDebugVisualizer(0); // Toggle frustum visualizer
        return;
    }
    
    // Pass to player controller if available
    if (m_playerController) {
        m_playerController->handleKeyPress(event);
        updateMouseTrackingState();
    }
}

// Key release event handler
void GLArenaWidget::keyReleaseEvent(QKeyEvent* event) {
    if (!event) return;
    
    // If inventory is open, don't pass keys to player controller
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        return;
    }
    
    // Pass to player controller if available
    if (m_playerController) {
        m_playerController->handleKeyRelease(event);
    }
}

// Mouse move event handler
void GLArenaWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!event) return;
    
    // If inventory is open or debug console is visible, handle UI interaction
    if ((m_inventoryUI && m_inventoryUI->isVisible()) || 
        (m_debugSystem && m_debugSystem->getConsole()->isVisible())) {
        if (m_inventoryUI && m_inventoryUI->isVisible()) {
            m_inventoryUI->handleMouseMove(event->x(), event->y());
        }
        return;
    }
    
    // Cast ray for voxel highlighting
    if (m_voxelSystem && m_playerController) {
        QVector3D camPos = m_playerController->getPosition();
        QVector3D camDir = QVector3D(
            cos(m_playerController->getRotation()) * cos(m_playerController->getPitch()),
            sin(m_playerController->getPitch()),
            sin(m_playerController->getRotation()) * cos(m_playerController->getPitch())
        );
        raycastVoxels(camPos, camDir);
    }
    
    // Pass to player controller if available
    if (m_playerController) {
        m_playerController->handleMouseMove(event);
    }
    
    // Recenter mouse cursor if mouse look is enabled
    if (hasFocus() && 
        (!m_inventoryUI || !m_inventoryUI->isVisible()) && 
        (!m_debugSystem || !m_debugSystem->getConsole()->isVisible())) {
        QCursor::setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));
    }
}

// Mouse press event handler
void GLArenaWidget::mousePressEvent(QMouseEvent* event) {
    if (!event) return;
    
    // If inventory is open or debug console is visible, handle UI interaction
    if ((m_inventoryUI && m_inventoryUI->isVisible()) || 
        (m_debugSystem && m_debugSystem->getConsole()->isVisible())) {
        if (m_inventoryUI && m_inventoryUI->isVisible()) {
            m_inventoryUI->handleMousePress(event->x(), event->y(), event->button());
        }
        return;
    }
    
    // Handle mouse button press for game interactions
    if (event->button() == Qt::LeftButton) {
        placeVoxel();
    } else if (event->button() == Qt::RightButton) {
        removeVoxel();
    }
}

// Mouse release event handler
void GLArenaWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (!event) return;
    
    // If inventory is open, handle UI interaction
    if (m_inventoryUI && m_inventoryUI->isVisible()) {
        m_inventoryUI->handleMouseRelease(event->x(), event->y(), event->button());
        return;
    }
}

// Helper method to handle mouse cursor visibility based on UI state
void GLArenaWidget::updateMouseTrackingState() {
    bool showCursor = false;
    
    // Show cursor when inventory is open or debug console is visible
    if ((m_inventoryUI && m_inventoryUI->isVisible()) || 
        (m_debugSystem && m_debugSystem->getConsole()->isVisible())) {
        showCursor = true;
    }
    
    // Update cursor visibility
    if (showCursor) {
        setCursor(Qt::ArrowCursor);
    } else {
        setCursor(Qt::BlankCursor);
    }
}