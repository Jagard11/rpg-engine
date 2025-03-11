// ./include/Debug/GlobeUpdater.hpp
#ifndef GLOBE_UPDATER_HPP
#define GLOBE_UPDATER_HPP

#include "World/World.hpp"
#include "Debug/DebugWindow.hpp"
#include <GLFW/glfw3.h>
#include <mutex>
#include <atomic>
#include <future>
#include <vector>
#include <unordered_set>

// Forward declarations
class Player;

/**
 * Enhanced helper class for managing globe visualization updates
 * Acts as a bridge between world state and debug visualization,
 * providing efficient globe updates and tracking modified regions
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
          lastFullUpdateTime(0.0),
          isInitialized(false),
          updateInProgress(false),
          autoFocusEnabled(false)
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
        
        // Regular updates - every second
        if (currentTime - lastUpdateTime >= 1.0) {
            performRegularUpdate();
            lastUpdateTime = currentTime;
        }
        
        // Full updates - every 15 seconds
        if (currentTime - lastFullUpdateTime >= 15.0) {
            performFullUpdate();
            lastFullUpdateTime = currentTime;
        }
    }
    
    /**
     * Track a block modification for visualization updates
     * @param worldPos The position where a block was modified
     */
    void trackModification(const glm::vec3& worldPos) {
        std::lock_guard<std::mutex> lock(modificationMutex);
        recentModifications.push_back(worldPos);
        
        // Limit the size of the list to prevent memory issues
        const size_t MAX_MODIFICATIONS = 1000;
        if (recentModifications.size() > MAX_MODIFICATIONS) {
            recentModifications.erase(recentModifications.begin());
        }
        
        // Record the chunk coordinates of the modification
        int chunkX = static_cast<int>(floor(worldPos.x / static_cast<float>(Chunk::SIZE)));
        int chunkY = static_cast<int>(floor(worldPos.y / static_cast<float>(Chunk::SIZE)));
        int chunkZ = static_cast<int>(floor(worldPos.z / static_cast<float>(Chunk::SIZE)));
        
        std::string chunkKey = std::to_string(chunkX) + "," + 
                              std::to_string(chunkY) + "," + 
                              std::to_string(chunkZ);
        
        modifiedChunks.insert(chunkKey);
        
        // If auto-focus is enabled, focus the god view on the latest modification
        if (autoFocusEnabled && debugWindow.getGodViewWindow() && 
            debugWindow.getGodViewWindow()->visible) {
            debugWindow.getGodViewWindow()->focusOnLocation(worldPos);
        }
    }
    
    /**
     * Toggle auto-focus feature which focuses the god view on recent modifications
     * @param enabled Whether auto-focus should be enabled
     */
    void setAutoFocus(bool enabled) {
        autoFocusEnabled = enabled;
    }
    
    /**
     * Get list of recent modifications for debugging/visualization
     * @return Vector of world positions where blocks were modified
     */
    std::vector<glm::vec3> getRecentModifications() const {
        std::lock_guard<std::mutex> lock(modificationMutex);
        return recentModifications;
    }
    
    /**
     * Clear the list of tracked modifications
     */
    void clearModifications() {
        std::lock_guard<std::mutex> lock(modificationMutex);
        recentModifications.clear();
        modifiedChunks.clear();
    }
    
    /**
     * Focus the god view on a specific world position
     * @param worldPos The position to focus on
     */
    void focusOnLocation(const glm::vec3& worldPos) {
        if (debugWindow.getGodViewWindow() && debugWindow.getGodViewWindow()->visible) {
            debugWindow.getGodViewWindow()->focusOnLocation(worldPos);
        }
    }
    
    /**
     * Focus the god view on the player's current position
     * @param player Reference to the player
     */
    void focusOnPlayer(const Player& player);

private:
    World& world;
    DebugWindow& debugWindow;
    
    // Update timers
    double lastUpdateTime;
    double lastFullUpdateTime;
    
    // Initialization state
    bool isInitialized;
    
    // Async update management
    std::atomic<bool> updateInProgress;
    std::future<void> updateFuture;
    
    // Auto-focus feature
    bool autoFocusEnabled;
    
    // Track block modifications for visualization updates
    mutable std::mutex modificationMutex;
    std::vector<glm::vec3> recentModifications;
    std::unordered_set<std::string> modifiedChunks;
    
    // Regular updates - called approximately once per second
    void performRegularUpdate() {
        // Skip if an update is already in progress
        if (updateInProgress.exchange(true)) {
            return;
        }
        
        // Update the god view if active and exists
        auto godViewTool = debugWindow.getGodViewTool();
        auto godViewWindow = debugWindow.getGodViewWindow();
        
        if (godViewTool && godViewWindow && godViewTool->isActive() && godViewWindow->autoRotate) {
            // Apply rotation based on the speed
            godViewTool->rotateView(godViewTool->getCurrentRotation() + godViewWindow->rotationSpeed);
        }
        
        // Mark update as complete
        updateInProgress.store(false);
    }
    
    // Full updates - called approximately every 15 seconds
    void performFullUpdate() {
        // Skip if an update is already in progress
        if (updateInProgress.exchange(true)) {
            return;
        }
        
        try {
            // If the god view tool exists, clear its height cache to force a refresh
            auto godViewTool = debugWindow.getGodViewTool();
            if (godViewTool) {
                godViewTool->clearHeightCache();
            }
            
            // Log the update
            LOG_INFO(LogCategory::RENDERING, "Performed full God View update");
            
            // Get information about modified chunks
            std::lock_guard<std::mutex> lock(modificationMutex);
            if (!modifiedChunks.empty()) {
                LOG_INFO(LogCategory::RENDERING, "Found " + std::to_string(modifiedChunks.size()) + 
                         " modified chunks for God View update");
            }
            
        } catch (const std::exception& e) {
            LOG_ERROR(LogCategory::RENDERING, "Exception in God View update: " + std::string(e.what()));
        } catch (...) {
            LOG_ERROR(LogCategory::RENDERING, "Unknown exception in God View update");
        }
        
        // Mark update as complete
        updateInProgress.store(false);
    }
};

#endif // GLOBE_UPDATER_HPP