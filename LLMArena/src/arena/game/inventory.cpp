// src/arena/game/inventory.cpp
#include "../../include/game/inventory.h"
#include <QDir>
#include <QDebug>
#include <QPainter>

Inventory::Inventory(QObject* parent) : QObject(parent), selectedActionBarSlot(0) {
    initializeDefaultItems();
}

bool Inventory::addItem(const InventoryItem& item) {
    // Check if item already exists
    for (int i = 0; i < items.size(); ++i) {
        if (items[i].id == item.id) {
            // Item already exists
            return false;
        }
    }
    
    // Add new item
    items.append(item);
    emit inventoryChanged();
    return true;
}

bool Inventory::removeItem(const QString& itemId) {
    for (int i = 0; i < items.size(); ++i) {
        if (items[i].id == itemId) {
            items.remove(i);
            
            // Remove item from action bar if it's there
            for (auto it = actionBar.begin(); it != actionBar.end(); ++it) {
                if (it.value() == itemId) {
                    actionBar.remove(it.key());
                    emit actionBarChanged(it.key());
                }
            }
            
            emit inventoryChanged();
            return true;
        }
    }
    
    return false;
}

InventoryItem Inventory::getItem(const QString& itemId) const {
    for (const InventoryItem& item : items) {
        if (item.id == itemId) {
            return item;
        }
    }
    
    return InventoryItem(); // Return empty item if not found
}

QVector<InventoryItem> Inventory::getAllItems() const {
    return items;
}

int Inventory::getItemCount() const {
    return items.size();
}

bool Inventory::hasItem(const QString& itemId) const {
    for (const InventoryItem& item : items) {
        if (item.id == itemId) {
            return true;
        }
    }
    
    return false;
}

void Inventory::setActionBarItem(int slot, const QString& itemId) {
    if (slot < 0 || slot >= MAX_ACTION_BAR_SLOTS) {
        return;
    }
    
    // Check if the item exists in inventory
    if (!itemId.isEmpty() && !hasItem(itemId)) {
        return;
    }
    
    actionBar[slot] = itemId;
    emit actionBarChanged(slot);
}

QString Inventory::getActionBarItemId(int slot) const {
    if (slot < 0 || slot >= MAX_ACTION_BAR_SLOTS) {
        return QString();
    }
    
    return actionBar.value(slot, QString());
}

InventoryItem Inventory::getActionBarItem(int slot) const {
    QString itemId = getActionBarItemId(slot);
    if (itemId.isEmpty()) {
        return InventoryItem();
    }
    
    return getItem(itemId);
}

void Inventory::clearActionBarSlot(int slot) {
    if (slot < 0 || slot >= MAX_ACTION_BAR_SLOTS) {
        return;
    }
    
    if (actionBar.contains(slot)) {
        actionBar.remove(slot);
        emit actionBarChanged(slot);
    }
}

int Inventory::getSelectedActionBarSlot() const {
    return selectedActionBarSlot;
}

void Inventory::setSelectedActionBarSlot(int slot) {
    if (slot < 0 || slot >= MAX_ACTION_BAR_SLOTS) {
        return;
    }
    
    if (selectedActionBarSlot != slot) {
        selectedActionBarSlot = slot;
        emit selectedActionBarSlotChanged(slot);
    }
}

void Inventory::initializeDefaultItems() {
    // Create paths for voxel textures
    QString resourcePath = QDir::currentPath() + "/resources/";
    qDebug() << "Initializing inventory with resource path:" << resourcePath;
    
    // Ensure resources directory exists
    QDir resourceDir(resourcePath);
    if (!resourceDir.exists()) {
        qDebug() << "Creating resources directory";
        QDir().mkpath(resourcePath);
    }
    
    // Helper function to create default textures if they don't exist
    auto ensureTextureExists = [&resourcePath](const QString& filename, const QColor& color) {
        QString filePath = resourcePath + filename;
        if (!QFile::exists(filePath)) {
            qDebug() << "Creating default texture:" << filePath;
            
            // Create a basic texture
            QImage img(32, 32, QImage::Format_RGBA8888);
            img.fill(color);
            
            // Add some texture
            QPainter painter(&img);
            painter.setPen(color.darker());
            for (int y = 0; y < 32; y += 4) {
                for (int x = 0; x < 32; x += 4) {
                    if ((x + y) % 8 == 0) {
                        painter.drawPoint(x, y);
                    }
                }
            }
            painter.end();
            
            // Save the texture
            if (!img.save(filePath)) {
                qWarning() << "Failed to create texture file:" << filePath;
            } else {
                qDebug() << "Successfully created texture:" << filePath;
            }
        }
        return filePath;
    };
    
    // Ensure textures exist or create them
    QString dirtTexture = ensureTextureExists("dirt.png", QColor(139, 69, 19)); // Brown
    QString grassTexture = ensureTextureExists("grass.png", QColor(34, 139, 34)); // Green
    QString cobblestoneTexture = ensureTextureExists("cobblestone.png", QColor(128, 128, 128)); // Gray
    
    // Add dirt item
    InventoryItem dirtItem("item_dirt", "Dirt Block", "A block of dirt.", 
                          dirtTexture, VoxelType::Dirt);
    addItem(dirtItem);
    
    // Add grass item
    InventoryItem grassItem("item_grass", "Grass Block", "A block of grass.", 
                           grassTexture, VoxelType::Grass);
    addItem(grassItem);
    
    // Add cobblestone item
    InventoryItem cobblestoneItem("item_cobblestone", "Cobblestone Block", "A block of cobblestone.", 
                                 cobblestoneTexture, VoxelType::Cobblestone);
    addItem(cobblestoneItem);
    
    // Set default action bar
    setActionBarItem(0, "item_dirt");
    setActionBarItem(1, "item_grass");
    setActionBarItem(2, "item_cobblestone");
    
    qDebug() << "Inventory initialization complete with" << items.size() << "items";
}