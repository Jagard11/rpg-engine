// ./GameFPS/VoxelGlobe/src/Player/Inventory.cpp
#include "Player/Inventory.hpp"
#include <iostream>
#include "Core/Debug.hpp"

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
    if (g_showDebug) {
        std::cout << "Selected Slot: " << selectedSlot << " (" << static_cast<int>(slots[selectedSlot]) << ")" << std::endl;
    }
}