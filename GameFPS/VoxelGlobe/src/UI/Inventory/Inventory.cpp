// ./src/UI/Inventory/Inventory.cpp
#include "UI/Inventory/Inventory.hpp"
#include <iostream>
#include "Debug/DebugManager.hpp"

Inventory::Inventory() : selectedSlot(0) {
    slots[0] = BlockType::GRASS;
    slots[1] = BlockType::DIRT;
    for (int i = 2; i < 10; i++) slots[i] = BlockType::AIR;
}

void Inventory::scroll(float delta) {
    if (delta > 0) {
        selectedSlot = (selectedSlot + 1) % 10;
    } else if (delta < 0) {
        selectedSlot = (selectedSlot - 1 + 10) % 10;
    }
    if (DebugManager::getInstance().logInventory()) { // Updated to new toggle
        std::cout << "Selected Slot: " << selectedSlot << " (" << static_cast<int>(slots[selectedSlot]) << ")" << std::endl;
    }
}