// src/arena/ui/gl_widgets/gl_arena_inventory.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include "../../include/ui/inventory_ui.h"
#include "../../include/game/inventory.h"
#include <QCursor>
#include <QDebug>

void GLArenaWidget::initializeInventory() {
    if (!m_initialized) {
        qWarning() << "Cannot initialize inventory: OpenGL not initialized";
        return;
    }
    
    try {
        // Create inventory system
        qDebug() << "Creating inventory object...";
        m_inventory = new Inventory(this);
        
        if (!m_inventory) {
            qCritical() << "Failed to create inventory object";
            return;
        }
        
        // Create inventory UI
        qDebug() << "Creating inventory UI...";
        m_inventoryUI = new InventoryUI(m_inventory, this);
        
        if (!m_inventoryUI) {
            qCritical() << "Failed to create inventory UI object";
            delete m_inventory;
            m_inventory = nullptr;
            return;
        }
        
        // Initialize inventory UI with OpenGL context - CRITICAL SECTION
        if (context() && context()->isValid()) {
            qDebug() << "Initializing inventory UI OpenGL resources...";
            
            // Lock current OpenGL context
            makeCurrent();
            
            try {
                // Check if we have valid shader program before initializing UI
                if (m_billboardProgram && m_billboardProgram->isLinked()) {
                    m_inventoryUI->initialize();
                } else {
                    qWarning() << "Skipping inventory UI initialization: shader program not ready";
                }
            } catch (const std::exception& e) {
                qCritical() << "Exception initializing inventory UI:" << e.what();
                // Don't throw, just handle the error
            } catch (...) {
                qCritical() << "Unknown exception initializing inventory UI";
                // Don't throw, just handle the error
            }
            
            doneCurrent();
        } else {
            qWarning() << "No valid OpenGL context for inventory UI initialization";
        }
        
        // Hide cursor by default (only show when inventory is open)
        setCursor(Qt::BlankCursor);
        
        // Connect inventory visibility signal with safety check
        if (m_inventoryUI) {
            connect(m_inventoryUI, &InventoryUI::visibilityChanged, 
                    this, &GLArenaWidget::onInventoryVisibilityChanged);
        }
        
        qDebug() << "Inventory system initialized";
    } catch (const std::exception& e) {
        qCritical() << "Failed to initialize inventory system:" << e.what();
        
        // Clean up on failure
        if (m_inventoryUI) {
            delete m_inventoryUI;
            m_inventoryUI = nullptr;
        }
        
        if (m_inventory) {
            delete m_inventory;
            m_inventory = nullptr;
        }
    } catch (...) {
        qCritical() << "Unknown exception initializing inventory system";
        
        // Clean up on failure
        if (m_inventoryUI) {
            delete m_inventoryUI;
            m_inventoryUI = nullptr;
        }
        
        if (m_inventory) {
            delete m_inventory;
            m_inventory = nullptr;
        }
    }
}

void GLArenaWidget::onInventoryVisibilityChanged(bool visible) {
    // Verify inventory UI exists
    if (!m_inventoryUI) {
        return;
    }
    
    // Show/hide cursor based on inventory visibility
    if (visible) {
        // Show cursor and use normal tracking when inventory is open
        setCursor(Qt::ArrowCursor);
        setMouseTracking(true);
    } else {
        // Hide cursor for game mode
        setCursor(Qt::BlankCursor);
        
        // Re-center the cursor to prevent camera jumping when closing inventory
        QPoint center(width()/2, height()/2);
        QCursor::setPos(mapToGlobal(center));
        
        // Grab keyboard focus
        setFocus();
    }
}

// Render inventory UI
void GLArenaWidget::renderInventory() {
    if (!m_inventoryUI) {
        return;
    }
    
    try {
        // Save current OpenGL state
        GLboolean depthTestEnabled;
        glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
        
        GLint blendSrc, blendDst;
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrc);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDst);
        
        // Setup for 2D UI rendering
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Verify width and height are valid
        int w = width();
        int h = height();
        if (w <= 0 || h <= 0) {
            qWarning() << "Invalid widget dimensions for inventory UI:" << w << "x" << h;
            return;
        }
        
        // Render inventory UI
        m_inventoryUI->render(w, h);
        
        // Restore OpenGL state
        if (depthTestEnabled) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        
        glBlendFunc(blendSrc, blendDst);
    } catch (const std::exception& e) {
        qCritical() << "Exception in renderInventory:" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception in renderInventory";
    }
}