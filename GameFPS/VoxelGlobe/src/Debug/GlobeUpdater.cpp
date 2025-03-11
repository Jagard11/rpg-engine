// ./src/Debug/GlobeUpdater.cpp
#include "Debug/GlobeUpdater.hpp"
#include <GLFW/glfw3.h>
#include "Debug/Logger.hpp"

GlobeUpdater::GlobeUpdater(World& w, DebugWindow& dw)
    : world(w), 
      debugWindow(dw),
      lastUpdateTime(0.0)
{
    LOG_INFO(LogCategory::RENDERING, "Globe updater initialized");
}

void GlobeUpdater::update()
{
    // Get current time
    double currentTime = glfwGetTime();
    
    // Only update every second to avoid excessive updates
    if (currentTime - lastUpdateTime < 1.0) {
        return;
    }
    
    // Update the god view if active
    auto godViewTool = debugWindow.getGodViewTool();
    if (godViewTool && godViewTool->isActive()) {
        // Check if auto-rotate is enabled in the DebugWindow
        GodViewWindow* godViewWindow = debugWindow.getGodViewWindow();
        if (godViewWindow && godViewWindow->autoRotate) {
            // Apply rotation based on the speed
            godViewTool->rotateView(godViewTool->getCurrentRotation() + godViewWindow->rotationSpeed);
        }
    }
    
    lastUpdateTime = currentTime;
}