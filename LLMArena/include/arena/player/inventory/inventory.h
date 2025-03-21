// include/arena/player/inventory/inventory.h
#ifndef INVENTORY_H
#define INVENTORY_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QMap>
#include "../voxels/types/voxel_types.h"

// Class to represent an inventory item
class InventoryItem {
public:
    QString id;
    QString name;
    QString description;
    QString iconPath;
    VoxelType voxelType;
    
    InventoryItem() : voxelType(VoxelType::Air) {}
    
    InventoryItem(const QString& _id, const QString& _name, const QString& _desc, 
                 const QString& _icon, VoxelType _type = VoxelType::Air)
        : id(_id), name(_name), description(_desc), iconPath(_icon), voxelType(_type) {}
    
    bool isVoxelItem() const { return voxelType != VoxelType::Air; }
};

// Class to represent a player's inventory
class Inventory : public QObject {
    Q_OBJECT
    
public:
    explicit Inventory(QObject* parent = nullptr);
    
    // Add item to inventory
    bool addItem(const InventoryItem& item);
    
    // Remove item from inventory
    bool removeItem(const QString& itemId);
    
    // Get item by ID
    InventoryItem getItem(const QString& itemId) const;
    
    // Get all items
    QVector<InventoryItem> getAllItems() const;
    
    // Get number of items
    int getItemCount() const;
    
    // Check if inventory contains an item
    bool hasItem(const QString& itemId) const;
    
    // Action bar methods
    void setActionBarItem(int slot, const QString& itemId);
    QString getActionBarItemId(int slot) const;
    InventoryItem getActionBarItem(int slot) const;
    void clearActionBarSlot(int slot);
    
    // Get current selected action bar slot
    int getSelectedActionBarSlot() const;
    
    // Set current selected action bar slot
    void setSelectedActionBarSlot(int slot);
    
signals:
    void inventoryChanged();
    void actionBarChanged(int slot);
    void selectedActionBarSlotChanged(int slot);
    
private:
    QVector<InventoryItem> items;
    QMap<int, QString> actionBar; // slot -> itemId
    int selectedActionBarSlot;
    
    static const int MAX_ACTION_BAR_SLOTS = 12;
    
    // Initialize with default items
    void initializeDefaultItems();
};

#endif // INVENTORY_H