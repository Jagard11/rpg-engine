// ./include/UI/Inventory/Inventory.hpp
#ifndef UI_INVENTORY_HPP
#define UI_INVENTORY_HPP

#include "Core/Types.hpp"

class Inventory {
public:
    Inventory();
    void scroll(float delta);
    int selectedSlot;
    BlockType slots[10];
};

#endif