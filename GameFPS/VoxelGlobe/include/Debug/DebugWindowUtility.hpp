// ./include/Debug/DebugWindowUtility.hpp
#ifndef DEBUG_WINDOW_UTILITY_HPP
#define DEBUG_WINDOW_UTILITY_HPP

#include "Debug/GodViewDebugTool.hpp"
#include "Debug/Logger.hpp"
#include <string>
#include <sstream>

namespace DebugWindowUtility {

/**
 * Print detailed diagnostic information about the god view tool's state
 */
inline void dumpGodViewState(const GodViewDebugTool* godViewTool) {
    if (!godViewTool) {
        LOG_ERROR(LogCategory::UI, "GodViewDebugTool is null, cannot dump state");
        return;
    }
    
    std::stringstream ss;
    ss << "God View Tool State: "
       << "\n- Is Active: " << (godViewTool->isActive() ? "YES" : "NO")
       << "\n- Current Rotation: " << godViewTool->getCurrentRotation();
    
    LOG_INFO(LogCategory::UI, ss.str());
}

/**
 * Force activation of the god view with default settings
 */
inline void forceActivateGodView(GodViewDebugTool* godViewTool) {
    if (!godViewTool) {
        LOG_ERROR(LogCategory::UI, "GodViewDebugTool is null, cannot activate");
        return;
    }
    
    // Set a good default view
    godViewTool->setCameraPosition(glm::vec3(0.0f, 0.0f, -15000.0f));
    godViewTool->setCameraTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    godViewTool->setZoom(1.0f);
    godViewTool->setWireframeMode(true); // Wireframe mode for better visibility
    godViewTool->setVisualizationType(0); // Height map
    
    // Activate
    godViewTool->setActive(true);
    
    LOG_INFO(LogCategory::UI, "God View forcibly activated with default settings");
}

}  // namespace DebugWindowUtility

#endif // DEBUG_WINDOW_UTILITY_HPP