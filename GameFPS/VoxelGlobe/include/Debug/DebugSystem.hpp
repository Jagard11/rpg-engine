// ./include/Debug/DebugSystem.hpp
#ifndef DEBUG_SYSTEM_HPP
#define DEBUG_SYSTEM_HPP

#include "Debug/Logger.hpp"
#include "Debug/DebugManager.hpp"
#include <glm/glm.hpp>

/**
 * Enhanced debugging system that integrates DebugManager and Logger
 * Provides specialized debugging functionality for game-specific components
 */
class DebugSystem {
public:
    // Get singleton instance
    static DebugSystem& getInstance();
    
    // Delete copy and move constructors/assignment operators
    DebugSystem(const DebugSystem&) = delete;
    DebugSystem& operator=(const DebugSystem&) = delete;
    DebugSystem(DebugSystem&&) = delete;
    DebugSystem& operator=(DebugSystem&&) = delete;
    
    // Initialize system
    void initialize();
    
    // Set debug flags
    void setShowVoxelEdges(bool enabled);
    void setCullingEnabled(bool enabled);
    void setUseFaceColors(bool enabled);
    void setDebugVertexScaling(bool enabled);
    
    // Get debug flags
    bool showVoxelEdges() const;
    bool isCullingEnabled() const;
    bool useFaceColors() const;
    bool debugVertexScaling() const;
    
    // Debug visualization helpers
    void logCoordinateInfo(const std::string& prefix, const glm::vec3& position);
    void logBlockInfo(const std::string& prefix, int x, int y, int z, int blockType);
    void logCollisionCheck(const std::string& prefix, const glm::vec3& position, bool collided);
    
    // Performance monitoring
    void beginFrameTiming();
    void endFrameTiming();
    void reportPerformance();
    
    // Apply debug settings from DebugManager
    void syncWithDebugManager(const DebugManager& manager);
    
    // Save debug settings to file
    void saveSettings(const std::string& filename = "debug_settings.json") const;
    
    // Load debug settings from file
    bool loadSettings(const std::string& filename = "debug_settings.json");

private:
    // Private constructor for singleton
    DebugSystem();
    
    // Debug visualization flags
    bool showVoxelEdges_;
    bool enableCulling_;
    bool useFaceColors_;
    bool debugVertexScaling_;
    
    // Performance tracking
    double frameTimeSum;
    int frameCount;
    double lastReportTime;
    
    // Helper methods
    double getTimestamp() const;
};

#endif // DEBUG_SYSTEM_HPP