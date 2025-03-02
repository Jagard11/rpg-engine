// ./include/UI/Inventory/InventoryUI.hpp
#ifndef UI_INVENTORY_UI_HPP
#define UI_INVENTORY_UI_HPP

#include "UI/Inventory/Inventory.hpp"
#include "imgui.h"

class InventoryUI {
public:
    void render(Inventory& inventory, const ImGuiIO& io);
};

#endif