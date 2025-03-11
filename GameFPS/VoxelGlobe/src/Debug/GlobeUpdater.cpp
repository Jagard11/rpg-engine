// ./src/Debug/GlobeUpdater.cpp
#include "Debug/GlobeUpdater.hpp"
#include <GLFW/glfw3.h>
#include "Debug/Logger.hpp"
#include "Player/Player.hpp"
#include "Debug/DebugManager.hpp"

// Implementation of GlobeUpdater methods not defined inline in the header

// Focus on player implementation
void GlobeUpdater::focusOnPlayer(const Player& player) {
    if (!debugWindow.getGodViewWindow() || !debugWindow.getGodViewWindow()->visible) {
        return;
    }
    
    // Focus the god view on the player's current position
    glm::vec3 playerPos = player.position;
    
    // Add a small offset to look at the player from slightly above
    glm::vec3 dirFromCenter = glm::normalize(playerPos);
    
    // Set up a view that looks at the player from slightly behind and above
    float cameraDistance = 2000.0f; // 2km away from player
    
    // Create a rotation matrix around up vector for "behind" position
    glm::vec3 right = glm::normalize(glm::cross(dirFromCenter, glm::vec3(0, 1, 0)));
    if (glm::length(right) < 0.01f) {
        right = glm::vec3(1, 0, 0); // Fallback if player is at exact pole
    }
    
    glm::vec3 forward = glm::normalize(glm::cross(right, dirFromCenter));
    
    // Position camera behind and above player
    glm::vec3 cameraOffset = -forward * 0.8f * cameraDistance + dirFromCenter * 0.6f * cameraDistance;
    glm::vec3 cameraPosition = playerPos + cameraOffset;
    
    // Focus the view
    debugWindow.getGodViewWindow()->cameraPosition = cameraPosition;
    debugWindow.getGodViewWindow()->cameraTarget = playerPos;
    
    // Calculate rotation based on direction from center to player
    float azimuth = std::atan2(playerPos.z, playerPos.x);
    debugWindow.getGodViewWindow()->manualRotation = glm::degrees(azimuth);
    
    // Set zoom to emphasize the player
    debugWindow.getGodViewWindow()->zoom = 1.0f;
    
    // Apply the changes to the god view tool
    debugWindow.getGodViewTool()->setCameraPosition(cameraPosition);
    debugWindow.getGodViewTool()->setCameraTarget(playerPos);
    debugWindow.getGodViewTool()->rotateView(glm::degrees(azimuth));
    debugWindow.getGodViewTool()->setZoom(1.0f);
    
    LOG_INFO(LogCategory::UI, "God View focused on player at: " + 
             std::to_string(playerPos.x) + ", " + 
             std::to_string(playerPos.y) + ", " + 
             std::to_string(playerPos.z));
}