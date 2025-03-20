// include/ui/inventory_ui.h
#ifndef INVENTORY_UI_H
#define INVENTORY_UI_H

#include <QObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>
#include <QMap>

#include "../game/inventory.h"

// Class to render inventory UI
class InventoryUI : public QObject, protected QOpenGLFunctions {
    Q_OBJECT
    
public:
    explicit InventoryUI(Inventory* inventory, QObject* parent = nullptr);
    ~InventoryUI();
    
    // Initialize OpenGL resources
    void initialize();
    
    // Render inventory UI
    void render(int screenWidth, int screenHeight);
    
    // Set inventory visibility
    void setVisible(bool visible);
    
    // Get inventory visibility
    bool isVisible() const;
    
    // Handle mouse events
    void handleMousePress(int x, int y, Qt::MouseButton button);
    void handleMouseMove(int x, int y);
    void handleMouseRelease(int x, int y, Qt::MouseButton button);
    
    // Check if mouse is over inventory UI
    bool isMouseOverUI(int x, int y) const;
    
    // Handle key events
    void handleKeyPress(int key);
    
    // Get the currently selected voxel type (for placing blocks)
    VoxelType getSelectedVoxelType() const;
    
    // Check if a voxel type is selected in the action bar
    bool hasVoxelTypeSelected() const;
    
    // Get the highlight box for the selected voxel face
    void getVoxelHighlight(QVector3D& position, QVector3D& size, int& face) const;
    
    // Set the currently highlighted voxel face
    void setHighlightedVoxelFace(const QVector3D& position, int face);
    
signals:
    void visibilityChanged(bool visible);
    void itemSelected(const InventoryItem& item);
    
private:
    Inventory* m_inventory;
    bool m_visible;
    
    // OpenGL resources
    QOpenGLShaderProgram* m_uiShader;
    QOpenGLBuffer m_quadVBO;
    QOpenGLVertexArrayObject m_quadVAO;
    
    // Textures for inventory items
    QMap<QString, QOpenGLTexture*> m_itemTextures;
    
    // Textures for UI elements
    QOpenGLTexture* m_inventoryBgTexture;
    QOpenGLTexture* m_actionBarBgTexture;
    QOpenGLTexture* m_slotTexture;
    QOpenGLTexture* m_selectedSlotTexture;
    
    // State tracking
    int m_draggedItemIndex;
    QVector2D m_dragStartPos;
    
    // Action bar state
    int m_actionBarSlots;
    
    // Voxel highlight
    QVector3D m_highlightPos;
    int m_highlightFace;
    
    // Helper methods
    void createShaders();
    void createQuadGeometry();
    void loadTextures();
    void renderInventory(int screenWidth, int screenHeight);
    void renderActionBar(int screenWidth, int screenHeight);
    void renderDraggedItem(int screenWidth, int screenHeight);
    void renderCurrentBlockPreview(int screenWidth, int screenHeight);
    
    // Get item at screen position
    int getItemIndexAtPosition(int x, int y, int screenWidth, int screenHeight) const;
    
    // Get action bar slot at screen position
    int getActionBarSlotAtPosition(int x, int y, int screenWidth, int screenHeight) const;
    
    // Draw a textured quad
    void drawTexturedQuad(float x, float y, float width, float height, QOpenGLTexture* texture,
                          const QVector4D& color = QVector4D(1, 1, 1, 1));
};

#endif // INVENTORY_UI_H