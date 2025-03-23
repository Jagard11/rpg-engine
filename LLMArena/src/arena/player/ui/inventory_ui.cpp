// src/arena/player/ui/inventory_ui.cpp
#include "../../../../include/arena/player/inventory/inventory_ui.h"
#include <QDebug>
#include <QImage>
#include <QPainter>
#include <QDir>

InventoryUI::InventoryUI(Inventory* inventory, QObject* parent)
    : QObject(parent),
      m_inventory(inventory),
      m_visible(false),
      m_uiShader(nullptr),
      m_quadVBO(QOpenGLBuffer::VertexBuffer),
      m_inventoryBgTexture(nullptr),
      m_actionBarBgTexture(nullptr),
      m_slotTexture(nullptr),
      m_selectedSlotTexture(nullptr),
      m_draggedItemIndex(-1),
      m_actionBarSlots(12),
      m_highlightFace(-1)
{
    // Connect signals from inventory
    if (m_inventory) {
        connect(m_inventory, &Inventory::inventoryChanged, this, [this]() {
            loadTextures();
        });
        connect(m_inventory, &Inventory::actionBarChanged, this, [this](int) {
            // Update when action bar changes
        });
        connect(m_inventory, &Inventory::selectedActionBarSlotChanged, this, [this](int) {
            // Update when selected slot changes
        });
    }
}

InventoryUI::~InventoryUI() {
    // Clean up OpenGL resources
    if (m_quadVBO.isCreated()) {
        m_quadVBO.destroy();
    }
    
    if (m_quadVAO.isCreated()) {
        m_quadVAO.destroy();
    }
    
    delete m_uiShader;
    
    // Clean up textures
    delete m_inventoryBgTexture;
    delete m_actionBarBgTexture;
    delete m_slotTexture;
    delete m_selectedSlotTexture;
    
    // Clean up item textures
    for (auto it = m_itemTextures.begin(); it != m_itemTextures.end(); ++it) {
        delete it.value();
    }
    m_itemTextures.clear();
}

void InventoryUI::initialize() {
    // Initialize OpenGL functions - make sure context is current
    if (!QOpenGLContext::currentContext() || !QOpenGLContext::currentContext()->isValid()) {
        qWarning() << "No valid OpenGL context in InventoryUI::initialize";
        return;
    }
    
    try {
        initializeOpenGLFunctions();
        
        // Create shaders
        createShaders();
        
        // Create quad geometry
        createQuadGeometry();
        
        // Load textures
        loadTextures();
        
        qDebug() << "InventoryUI initialized successfully";
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in InventoryUI::initialize:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown exception in InventoryUI::initialize";
    }
}

void InventoryUI::render(int screenWidth, int screenHeight) {
    if (!m_uiShader || !m_uiShader->isLinked()) {
        // Silently return rather than warning - avoid spam
        return;
    }
    
    if (!m_quadVAO.isCreated()) {
        // Silently return rather than warning - avoid spam
        return;
    }
    
    // Validate screen dimensions
    if (screenWidth <= 0 || screenHeight <= 0) {
        return;
    }
    
    try {
        // Bind shader
        if (!m_uiShader->bind()) {
            return;
        }
        
        // Set up orthographic projection for 2D rendering
        QMatrix4x4 projection;
        projection.ortho(0, screenWidth, screenHeight, 0, -1, 1);
        m_uiShader->setUniformValue("projection", projection);
        
        // Set up model-view matrix (identity)
        QMatrix4x4 modelView;
        modelView.setToIdentity();
        m_uiShader->setUniformValue("modelView", modelView);
        
        // Save OpenGL state
        GLint oldDepthTest;
        glGetIntegerv(GL_DEPTH_TEST, &oldDepthTest);
        
        // Disable depth testing for UI
        glDisable(GL_DEPTH_TEST);
        
        // Enable blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Bind VAO
        m_quadVAO.bind();
        
        // Render action bar (always visible)
        renderActionBar(screenWidth, screenHeight);
        
        // Render full inventory if visible
        if (m_visible) {
            renderInventory(screenWidth, screenHeight);
        }
        
        // Render dragged item if any
        if (m_draggedItemIndex >= 0) {
            renderDraggedItem(screenWidth, screenHeight);
        }
        
        // Render current block preview if a voxel type is selected
        if (hasVoxelTypeSelected() && !m_visible) {
            renderCurrentBlockPreview(screenWidth, screenHeight);
        }
        
        // Unbind VAO
        m_quadVAO.release();
        
        // Restore OpenGL state
        if (oldDepthTest) {
            glEnable(GL_DEPTH_TEST);
        }
        
        // Unbind shader
        m_uiShader->release();
    }
    catch (const std::exception& e) {
        // Log but continue - don't crash on UI render issues
        qWarning() << "Exception in InventoryUI::render:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception in InventoryUI::render";
    }
}

void InventoryUI::setVisible(bool visible) {
    if (m_visible != visible) {
        m_visible = visible;
        emit visibilityChanged(m_visible);
    }
}

bool InventoryUI::isVisible() const {
    return m_visible;
}

void InventoryUI::handleMousePress(int x, int y, Qt::MouseButton button) {
    if (!m_inventory) {
        return;
    }
    
    if (m_visible) {
        // Handle inventory interaction
        if (button == Qt::LeftButton) {
            // Check if clicked on an item
            int itemIndex = getItemIndexAtPosition(x, y, 800, 600); // TODO: Get actual screen size
            if (itemIndex >= 0) {
                m_draggedItemIndex = itemIndex;
                m_dragStartPos = QVector2D(x, y);
            } else {
                // Check if clicked on action bar slot
                int slotIndex = getActionBarSlotAtPosition(x, y, 800, 600);
                if (slotIndex >= 0) {
                    // Handle action bar slot click
                    m_inventory->setSelectedActionBarSlot(slotIndex);
                }
            }
        }
    } else {
        // Handle action bar interaction
        if (button == Qt::LeftButton) {
            int slotIndex = getActionBarSlotAtPosition(x, y, 800, 600);
            if (slotIndex >= 0) {
                m_inventory->setSelectedActionBarSlot(slotIndex);
            }
        }
    }
}

void InventoryUI::handleMouseMove(int x, int y) {
    if (m_draggedItemIndex >= 0) {
        // Update dragged item position
        m_dragStartPos = QVector2D(x, y);
    }
}

void InventoryUI::handleMouseRelease(int x, int y, Qt::MouseButton button) {
    if (!m_inventory) {
        return;
    }
    
    if (m_visible && m_draggedItemIndex >= 0 && button == Qt::LeftButton) {
        // Check if released on an action bar slot
        int slotIndex = getActionBarSlotAtPosition(x, y, 800, 600);
        if (slotIndex >= 0) {
            // Move item to action bar
            QVector<InventoryItem> items = m_inventory->getAllItems();
            if (m_draggedItemIndex < items.size()) {
                m_inventory->setActionBarItem(slotIndex, items[m_draggedItemIndex].id);
            }
        }
        
        m_draggedItemIndex = -1;
    }
}

bool InventoryUI::isMouseOverUI(int x, int y) const {
    if (m_visible) {
        // When inventory is open, all mouse events are over UI
        return true;
    } else {
        // Check if over action bar
        int slotIndex = getActionBarSlotAtPosition(x, y, 800, 600); // TODO: Get actual screen size
        return slotIndex >= 0;
    }
}

void InventoryUI::handleKeyPress(int key) {
    // Handle action bar selection (keys 1-=)
    if (key >= Qt::Key_1 && key <= Qt::Key_Equal) {
        int slot = key - Qt::Key_1;
        if (slot >= 0 && slot < m_actionBarSlots) {
            m_inventory->setSelectedActionBarSlot(slot);
        }
    }
    
    // Toggle inventory with 'I'
    if (key == Qt::Key_I) {
        setVisible(!m_visible);
    }
}

VoxelType InventoryUI::getSelectedVoxelType() const {
    if (!m_inventory) {
        return VoxelType::Air;
    }
    
    int slot = m_inventory->getSelectedActionBarSlot();
    InventoryItem item = m_inventory->getActionBarItem(slot);
    
    return item.isVoxelItem() ? item.voxelType : VoxelType::Air;
}

bool InventoryUI::hasVoxelTypeSelected() const {
    return getSelectedVoxelType() != VoxelType::Air;
}

void InventoryUI::getVoxelHighlight(QVector3D& position, QVector3D& size, int& face) const {
    position = m_highlightPos;
    size = QVector3D(1.02f, 1.02f, 1.02f); // Slightly larger than voxel
    face = m_highlightFace;
}

void InventoryUI::setHighlightedVoxelFace(const QVector3D& position, int face) {
    m_highlightPos = position;
    m_highlightFace = face;
}

void InventoryUI::createShaders() {
    // Clean up previous shader if it exists
    if (m_uiShader) {
        delete m_uiShader;
        m_uiShader = nullptr;
    }
    
    // Create shader program for inventory UI
    m_uiShader = new QOpenGLShaderProgram();
    if (!m_uiShader) {
        qCritical() << "Failed to allocate shader program for inventory UI";
        return;
    }
    
    // Simple vertex shader
    const char* vertexShaderSource = R"(
        #version 120
        attribute vec2 position;
        attribute vec2 texCoord;
        
        uniform mat4 modelView;
        uniform mat4 projection;
        
        varying vec2 fragTexCoord;
        
        void main() {
            gl_Position = projection * modelView * vec4(position, 0.0, 1.0);
            fragTexCoord = texCoord;
        }
    )";
    
    // Simple fragment shader
    const char* fragmentShaderSource = R"(
        #version 120
        varying vec2 fragTexCoord;
        
        uniform sampler2D textureSampler;
        uniform vec4 color;
        
        void main() {
            vec4 texColor = texture2D(textureSampler, fragTexCoord);
            gl_FragColor = texColor * color;
        }
    )";
    
    // Compile and link shaders with error checking
    bool success = true;
    
    if (!m_uiShader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qCritical() << "Failed to compile UI vertex shader:" << m_uiShader->log();
        success = false;
    }
    
    if (success && !m_uiShader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qCritical() << "Failed to compile UI fragment shader:" << m_uiShader->log();
        success = false;
    }
    
    // Set attribute locations before linking
    if (success) {
        m_uiShader->bindAttributeLocation("position", 0);
        m_uiShader->bindAttributeLocation("texCoord", 1);
    }
    
    if (success && !m_uiShader->link()) {
        qCritical() << "Failed to link UI shader program:" << m_uiShader->log();
        success = false;
    }
    
    if (!success) {
        delete m_uiShader;
        m_uiShader = nullptr;
    }
}

void InventoryUI::createQuadGeometry() {
    // Cleanup previous objects if they exist
    if (m_quadVAO.isCreated()) {
        m_quadVAO.destroy();
    }
    
    if (m_quadVBO.isCreated()) {
        m_quadVBO.destroy();
    }
    
    // Create VAO with error checking
    if (!m_quadVAO.create()) {
        qCritical() << "Failed to create VAO for inventory UI";
        return;
    }
    m_quadVAO.bind();
    
    // Create VBO with error checking
    if (!m_quadVBO.create()) {
        qCritical() << "Failed to create VBO for inventory UI";
        m_quadVAO.release();
        m_quadVAO.destroy();
        return;
    }
    m_quadVBO.bind();
    
    // Quad vertex data: position(2) + texCoord(2)
    float quadVertices[] = {
        // Position    // TexCoord
        0.0f, 0.0f,    0.0f, 0.0f,
        1.0f, 0.0f,    1.0f, 0.0f,
        1.0f, 1.0f,    1.0f, 1.0f,
        0.0f, 1.0f,    0.0f, 1.0f
    };
    
    // Allocate vertex data
    m_quadVBO.allocate(quadVertices, sizeof(quadVertices));
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    
    glEnableVertexAttribArray(1); // texCoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    
    // Release bindings
    m_quadVBO.release();
    m_quadVAO.release();
}

void InventoryUI::loadTextures() {
    if (!m_inventory) {
        qWarning() << "No inventory available in loadTextures";
        return;
    }
    
    // Ensure we have a valid OpenGL context
    if (!QOpenGLContext::currentContext() || !QOpenGLContext::currentContext()->isValid()) {
        qWarning() << "No valid OpenGL context in loadTextures";
        return;
    }
    
    QString resourcePath = QDir::currentPath() + "/resources/";
    qDebug() << "Loading inventory textures from:" << resourcePath;
    
    // Ensure resource directory exists
    QDir resourceDir(resourcePath);
    if (!resourceDir.exists()) {
        qDebug() << "Creating resources directory";
        if (!QDir().mkpath(resourcePath)) {
            qWarning() << "Failed to create resources directory";
        }
    }
    
    // Create a helper function to safely create texture from image
    auto createSafeTexture = [](const QImage& image) -> QOpenGLTexture* {
        if (image.isNull()) {
            qWarning() << "Null image provided to createSafeTexture";
            return nullptr;
        }
        
        QOpenGLTexture* texture = nullptr;
        try {
            texture = new QOpenGLTexture(image);
            if (!texture) {
                qWarning() << "Failed to allocate texture";
                return nullptr;
            }
            
            if (!texture->isCreated()) {
                qWarning() << "Texture creation failed";
                delete texture;
                return nullptr;
            }
            
            texture->setMinificationFilter(QOpenGLTexture::Linear);
            texture->setMagnificationFilter(QOpenGLTexture::Linear);
            return texture;
        }
        catch (const std::exception& e) {
            qWarning() << "Exception creating texture:" << e.what();
            delete texture;
            return nullptr;
        }
        catch (...) {
            qWarning() << "Unknown exception creating texture";
            delete texture;
            return nullptr;
        }
    };
    
    // Clean up existing item textures
    for (auto it = m_itemTextures.begin(); it != m_itemTextures.end(); ++it) {
        delete it.value();
    }
    m_itemTextures.clear();
    
    // Create UI textures if they don't exist
    try {
        // Safely delete old texture if it exists
        if (m_inventoryBgTexture) {
            delete m_inventoryBgTexture;
            m_inventoryBgTexture = nullptr;
        }
        
        // Create inventory background texture
        QImage bgImage(512, 384, QImage::Format_RGBA8888);
        bgImage.fill(QColor(64, 64, 64, 200));
        
        QPainter painter(&bgImage);
        painter.setPen(QColor(200, 200, 200));
        painter.drawRect(0, 0, bgImage.width() - 1, bgImage.height() - 1);
        painter.end();
        
        m_inventoryBgTexture = createSafeTexture(bgImage);
        
        // Safely delete old texture if it exists
        if (m_actionBarBgTexture) {
            delete m_actionBarBgTexture;
            m_actionBarBgTexture = nullptr;
        }
        
        // Create action bar background texture
        QImage barImage(480, 48, QImage::Format_RGBA8888);
        barImage.fill(QColor(64, 64, 64, 180));
        
        painter.begin(&barImage);
        painter.setPen(QColor(180, 180, 180));
        painter.drawRect(0, 0, barImage.width() - 1, barImage.height() - 1);
        painter.end();
        
        m_actionBarBgTexture = createSafeTexture(barImage);
        
        // Safely delete old texture if it exists
        if (m_slotTexture) {
            delete m_slotTexture;
            m_slotTexture = nullptr;
        }
        
        // Create slot texture
        QImage slotImage(40, 40, QImage::Format_RGBA8888);
        slotImage.fill(QColor(48, 48, 48, 220));
        
        painter.begin(&slotImage);
        painter.setPen(QColor(150, 150, 150));
        painter.drawRect(0, 0, slotImage.width() - 1, slotImage.height() - 1);
        painter.end();
        
        m_slotTexture = createSafeTexture(slotImage);
        
        // Safely delete old texture if it exists
        if (m_selectedSlotTexture) {
            delete m_selectedSlotTexture;
            m_selectedSlotTexture = nullptr;
        }
        
        // Create selected slot texture
        QImage selectedSlotImage(40, 40, QImage::Format_RGBA8888);
        selectedSlotImage.fill(QColor(80, 80, 128, 220));
        
        painter.begin(&selectedSlotImage);
        painter.setPen(QColor(200, 200, 255));
        painter.drawRect(0, 0, selectedSlotImage.width() - 1, selectedSlotImage.height() - 1);
        painter.end();
        
        m_selectedSlotTexture = createSafeTexture(selectedSlotImage);
    }
    catch (const std::exception& e) {
        qWarning() << "Exception creating UI textures:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception creating UI textures";
    }
    
    // Load textures for all inventory items
    QVector<InventoryItem> items = m_inventory->getAllItems();
    qDebug() << "Loading textures for" << items.size() << "inventory items";
    
    for (const InventoryItem& item : items) {
        if (m_itemTextures.contains(item.id)) {
            continue; // Skip if already loaded
        }
        
        try {
            QImage itemImage;
            
            // Check if the icon file exists
            bool imageLoaded = false;
            if (!item.iconPath.isEmpty()) {
                QFileInfo fileInfo(item.iconPath);
                if (fileInfo.exists() && fileInfo.isFile()) {
                    if (itemImage.load(item.iconPath)) {
                        imageLoaded = true;
                        qDebug() << "Loaded texture from:" << item.iconPath;
                    } else {
                        qWarning() << "Failed to load image from:" << item.iconPath;
                    }
                } else {
                    qWarning() << "Texture file does not exist:" << item.iconPath;
                }
            }
            
            // Create default texture if icon not found or failed to load
            if (!imageLoaded) {
                qDebug() << "Creating default texture for item:" << item.id;
                
                // Create a colored image based on voxel type
                QColor color;
                switch (item.voxelType) {
                    case VoxelType::Dirt:
                        color = QColor(139, 69, 19); // Brown
                        break;
                    case VoxelType::Grass:
                        color = QColor(34, 139, 34); // Green
                        break;
                    case VoxelType::Cobblestone:
                        color = QColor(128, 128, 128); // Gray
                        break;
                    default:
                        color = QColor(255, 0, 255); // Pink for default/unknown
                        break;
                }
                
                itemImage = QImage(32, 32, QImage::Format_RGBA8888);
                itemImage.fill(color);
                
                // Add some texture pattern
                QPainter painter(&itemImage);
                painter.setPen(color.darker(150));
                
                for (int y = 0; y < 32; y += 4) {
                    for (int x = 0; x < 32; x += 4) {
                        if ((x + y) % 8 == 0) {
                            painter.drawPoint(x, y);
                        }
                    }
                }
                
                // Add item name text
                painter.setPen(Qt::white);
                painter.setFont(QFont("Arial", 7));
                
                QString displayText = item.id;
                if (displayText.startsWith("item_")) {
                    displayText = displayText.mid(5); // Remove "item_" prefix
                }
                
                if (displayText.length() > 6) {
                    displayText = displayText.left(6); // Truncate to fit
                }
                
                painter.drawText(QRect(0, 12, 32, 10), Qt::AlignCenter, displayText);
                painter.end();
            }
            
            // Create the OpenGL texture
            QOpenGLTexture* texture = createSafeTexture(itemImage);
            if (texture) {
                m_itemTextures[item.id] = texture;
                qDebug() << "Created texture for item:" << item.id;
            }
        }
        catch (const std::exception& e) {
            qWarning() << "Exception creating texture for item" << item.id << ":" << e.what();
        }
        catch (...) {
            qWarning() << "Unknown exception creating texture for item" << item.id;
        }
    }
}

void InventoryUI::renderInventory(int screenWidth, int screenHeight) {
    if (!m_inventory || !m_inventoryBgTexture || !m_inventoryBgTexture->isCreated()) {
        return;
    }
    
    // Calculate inventory position (centered)
    float invWidth = 512;
    float invHeight = 384;
    float invX = (screenWidth - invWidth) / 2;
    float invY = (screenHeight - invHeight) / 2;
    
    // Draw inventory background
    drawTexturedQuad(invX, invY, invWidth, invHeight, m_inventoryBgTexture);
    
    // Draw inventory slots
    const int SLOTS_PER_ROW = 10;
    const int SLOT_SIZE = 40;
    const int SLOT_SPACING = 4;
    
    float startX = invX + 16;
    float startY = invY + 60;
    
    QVector<InventoryItem> items = m_inventory->getAllItems();
    for (int i = 0; i < items.size(); ++i) {
        int row = i / SLOTS_PER_ROW;
        int col = i % SLOTS_PER_ROW;
        
        float x = startX + col * (SLOT_SIZE + SLOT_SPACING);
        float y = startY + row * (SLOT_SIZE + SLOT_SPACING);
        
        // Draw slot background
        if (m_slotTexture && m_slotTexture->isCreated()) {
            drawTexturedQuad(x, y, SLOT_SIZE, SLOT_SIZE, m_slotTexture);
        }
        
        // Draw item icon
        const InventoryItem& item = items[i];
        if (m_itemTextures.contains(item.id) && m_itemTextures[item.id]->isCreated() && i != m_draggedItemIndex) {
            drawTexturedQuad(x + 4, y + 4, SLOT_SIZE - 8, SLOT_SIZE - 8, m_itemTextures[item.id]);
        }
    }
}

void InventoryUI::renderActionBar(int screenWidth, int screenHeight) {
    if (!m_inventory || !m_actionBarBgTexture || !m_actionBarBgTexture->isCreated() || 
        !m_slotTexture || !m_slotTexture->isCreated() || 
        !m_selectedSlotTexture || !m_selectedSlotTexture->isCreated()) {
        return;
    }
    
    // Calculate action bar position (centered at bottom)
    float barWidth = 480;
    float barHeight = 48;
    float barX = (screenWidth - barWidth) / 2;
    float barY = screenHeight - barHeight - 8;
    
    // Draw action bar background
    drawTexturedQuad(barX, barY, barWidth, barHeight, m_actionBarBgTexture);
    
    // Draw action bar slots
    const int SLOT_SIZE = 40;
    const int SLOT_SPACING = 4;
    float startX = barX + (barWidth - (m_actionBarSlots * (SLOT_SIZE + SLOT_SPACING) - SLOT_SPACING)) / 2;
    float startY = barY + (barHeight - SLOT_SIZE) / 2;
    
    for (int i = 0; i < m_actionBarSlots; ++i) {
        float x = startX + i * (SLOT_SIZE + SLOT_SPACING);
        
        // Draw slot background (selected or normal)
        QOpenGLTexture* slotTexture = (i == m_inventory->getSelectedActionBarSlot()) 
                                    ? m_selectedSlotTexture : m_slotTexture;
        if (slotTexture && slotTexture->isCreated()) {
            drawTexturedQuad(x, startY, SLOT_SIZE, SLOT_SIZE, slotTexture);
        }
        
        // Draw item icon if slot has an item
        QString itemId = m_inventory->getActionBarItemId(i);
        if (!itemId.isEmpty() && m_itemTextures.contains(itemId) && 
            m_itemTextures[itemId] && m_itemTextures[itemId]->isCreated()) {
            drawTexturedQuad(x + 4, startY + 4, SLOT_SIZE - 8, SLOT_SIZE - 8, m_itemTextures[itemId]);
        }
    }
}

void InventoryUI::renderDraggedItem(int screenWidth, int screenHeight) {
    if (!m_inventory || m_draggedItemIndex < 0) {
        return;
    }
    
    QVector<InventoryItem> items = m_inventory->getAllItems();
    if (m_draggedItemIndex >= items.size()) {
        return;
    }
    
    const InventoryItem& item = items[m_draggedItemIndex];
    if (!m_itemTextures.contains(item.id) || !m_itemTextures[item.id] || !m_itemTextures[item.id]->isCreated()) {
        return;
    }
    
    // Draw dragged item at mouse position
    const float ITEM_SIZE = 32;
    float x = m_dragStartPos.x() - ITEM_SIZE / 2;
    float y = m_dragStartPos.y() - ITEM_SIZE / 2;
    
    drawTexturedQuad(x, y, ITEM_SIZE, ITEM_SIZE, m_itemTextures[item.id], QVector4D(1, 1, 1, 0.8f));
}

void InventoryUI::renderCurrentBlockPreview(int screenWidth, int screenHeight) {
    if (!m_inventory || !hasVoxelTypeSelected()) {
        return;
    }
    
    int slot = m_inventory->getSelectedActionBarSlot();
    InventoryItem item = m_inventory->getActionBarItem(slot);
    
    if (!item.isVoxelItem() || !m_itemTextures.contains(item.id) || 
        !m_itemTextures[item.id] || !m_itemTextures[item.id]->isCreated()) {
        return;
    }
    
    // Draw block preview in bottom right
    const float PREVIEW_SIZE = 64;
    float x = screenWidth - PREVIEW_SIZE - 16;
    float y = screenHeight - PREVIEW_SIZE - 16;
    
    drawTexturedQuad(x, y, PREVIEW_SIZE, PREVIEW_SIZE, m_itemTextures[item.id], QVector4D(1, 1, 1, 0.8f));
}

int InventoryUI::getItemIndexAtPosition(int x, int y, int screenWidth, int screenHeight) const {
    if (!m_inventory || !m_visible) {
        return -1;
    }
    
    // Calculate inventory position
    float invWidth = 512;
    float invHeight = 384;
    float invX = (screenWidth - invWidth) / 2;
    float invY = (screenHeight - invHeight) / 2;
    
    // Calculate slots area
    const int SLOTS_PER_ROW = 10;
    const int SLOT_SIZE = 40;
    const int SLOT_SPACING = 4;
    float startX = invX + 16;
    float startY = invY + 60;
    
    // Check if mouse is in slots area
    if (x < startX || y < startY) {
        return -1;
    }
    
    // Calculate slot index
    int col = (x - startX) / (SLOT_SIZE + SLOT_SPACING);
    int row = (y - startY) / (SLOT_SIZE + SLOT_SPACING);
    
    // Check if within valid range
    if (col < 0 || col >= SLOTS_PER_ROW || row < 0) {
        return -1;
    }
    
    int index = row * SLOTS_PER_ROW + col;
    if (index >= m_inventory->getItemCount()) {
        return -1;
    }
    
    // Check if within slot bounds
    float slotX = startX + col * (SLOT_SIZE + SLOT_SPACING);
    float slotY = startY + row * (SLOT_SIZE + SLOT_SPACING);
    if (x < slotX || x >= slotX + SLOT_SIZE || y < slotY || y >= slotY + SLOT_SIZE) {
        return -1;
    }
    
    return index;
}

int InventoryUI::getActionBarSlotAtPosition(int x, int y, int screenWidth, int screenHeight) const {
    if (!m_inventory) {
        return -1;
    }
    
    // Calculate action bar position
    float barWidth = 480;
    float barHeight = 48;
    float barX = (screenWidth - barWidth) / 2;
    float barY = screenHeight - barHeight - 8;
    
    // Check if mouse is in action bar area
    if (x < barX || x >= barX + barWidth || y < barY || y >= barY + barHeight) {
        return -1;
    }
    
    // Calculate slot positions
    const int SLOT_SIZE = 40;
    const int SLOT_SPACING = 4;
    float startX = barX + (barWidth - (m_actionBarSlots * (SLOT_SIZE + SLOT_SPACING) - SLOT_SPACING)) / 2;
    float startY = barY + (barHeight - SLOT_SIZE) / 2;
    
    // Calculate slot index
    for (int i = 0; i < m_actionBarSlots; ++i) {
        float slotX = startX + i * (SLOT_SIZE + SLOT_SPACING);
        if (x >= slotX && x < slotX + SLOT_SIZE && y >= startY && y < startY + SLOT_SIZE) {
            return i;
        }
    }
    
    return -1;
}

void InventoryUI::drawTexturedQuad(float x, float y, float width, float height, QOpenGLTexture* texture,
                                const QVector4D& color) {
    if (!m_uiShader || !texture || !texture->isCreated()) {
        return;
    }
    
    // Set shader uniforms
    QMatrix4x4 modelView;
    modelView.setToIdentity();
    modelView.translate(x, y, 0);
    modelView.scale(width, height, 1);
    
    m_uiShader->setUniformValue("modelView", modelView);
    m_uiShader->setUniformValue("color", color);
    m_uiShader->setUniformValue("textureSampler", 0);
    
    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    texture->bind();
    
    // Draw quad
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    // Unbind texture
    texture->release();
}