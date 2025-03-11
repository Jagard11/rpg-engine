// ./include/Debug/GlobeUpdater.hpp
#ifndef GLOBE_UPDATER_HPP
#define GLOBE_UPDATER_HPP

#include "World/World.hpp"
#include "Debug/DebugWindow.hpp"
#include <GLFW/glfw3.h>

/**
 * Helper class for managing globe visualization updates
 * Acts as a bridge between world state and debug visualization
 */
class GlobeUpdater {
public:
    /**
     * Constructor
     * @param w Reference to the world
     * @param dw Reference to the debug window containing the god view
     */
    GlobeUpdater(World& w, DebugWindow& dw) 
        : world(w), 
          debugWindow(dw),
          lastUpdateTime(0.0),
          isInitialized(false)
    {
        // Defer initialization to avoid startup crashes
    }
    
    /**
     * Safely initialize the Globe Updater after all systems are ready
     */
    void safeInit() {
        if (!isInitialized) {
            isInitialized = true;
            // Make sure the god view is disabled initially
            if (debugWindow.getGodViewTool()) {
                debugWindow.getGodViewTool()->setActive(false);
            }
            if (debugWindow.getGodViewWindow()) {
                debugWindow.getGodViewWindow()->visible = false;
            }
        }
    }
    
    /**
     * Update globe visualization based on current world state
     * Should be called each frame
     */
    void update() {
        // Skip if not initialized yet
        if (!isInitialized) {
            safeInit();
            return;
        }
        
        // Get current time
        double currentTime = glfwGetTime();
        
        // Only update every second to avoid excessive updates
        if (currentTime - lastUpdateTime < 1.0) {
            return;
        }
        
        // Update the god view if active and exists
        auto godViewTool = debugWindow.getGodViewTool();
        auto godViewWindow = debugWindow.getGodViewWindow();
        
        if (godViewTool && godViewWindow && godViewTool->isActive() && godViewWindow->autoRotate) {
            // Apply rotation based on the speed
            godViewTool->rotateView(godViewTool->getCurrentRotation() + godViewWindow->rotationSpeed);
        }
        
        lastUpdateTime = currentTime;
    }

private:
    World& world;
    DebugWindow& debugWindow;
    
    // Track last update time for throttling
    double lastUpdateTime;
    
    // Track initialization state
    bool isInitialized;
};

#endif // GLOBE_UPDATER_HPP