// ./src/UI/Inventory/InventoryUI.cpp
#include "UI/Inventory/InventoryUI.hpp"
#include <string> // Added for std::to_string

void InventoryUI::render(Inventory& inventory, const ImGuiIO& io) {
    ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y - 40));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, 40));
    ImGui::Begin("Inventory", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
    for (int i = 0; i < 10; i++) {
        ImGui::PushID(i);
        if (i == inventory.selectedSlot) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
        if (ImGui::Button(std::to_string(i).c_str(), ImVec2(40, 40))) {
            inventory.selectedSlot = i;
        }
        if (i == inventory.selectedSlot) ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PopID();
    }
    ImGui::End();
}