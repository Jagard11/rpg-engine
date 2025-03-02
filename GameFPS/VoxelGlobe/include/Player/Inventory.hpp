// ./include/Player/Inventory.hpp
#ifndef INVENTORY_HPP
#define INVENTORY_HPP

#include "Core/Types.hpp"

class Inventory {
public:
    Inventory();
    void scroll(float delta);
    int selectedSlot;
    BlockType slots[10];
};

#endif