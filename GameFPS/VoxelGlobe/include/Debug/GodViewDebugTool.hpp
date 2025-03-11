// ./include/Debug/GodViewDebugTool.hpp
#ifndef GOD_VIEW_DEBUG_TOOL_HPP
#define GOD_VIEW_DEBUG_TOOL_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <mutex>
#include <future>
#include <atomic>
#include "World/World.hpp"
#include "Graphics/GraphicsSettings.hpp"

// Custom hash function for glm::ivec2
struct IVec2Hash {
    std::size_t operator()(const glm::ivec2& v) const {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
    }
};

/**
 * Debug tool that provides a god's eye view of the entire globe.
 * Useful for visualizing and debugging procedural terrain generation.
 */
class GodViewDebugTool {
public:
    // Visualization modes
    enum class VisualizationMode {
        PROCEDURAL,   // Show only procedural terrain
        ACTUAL,       // Show only actual voxel data
        HYBRID        // Show both procedural and actual data
    };

    // Height sample cache
    struct HeightSample {
        double height;      // Height value
        bool isActual;      // True if from actual chunk, false if procedural
        double timestamp;   // When this sample was taken
    };

    GodViewDebugTool(const World& world);
    ~GodViewDebugTool();
    
    // Render the globe visualization
    void render(const GraphicsSettings& settings);
    
    // Camera and view controls
    void setCameraPosition(const glm::vec3& position);
    void setCameraTarget(const glm::vec3& target);
    void setZoom(float zoom);
    void rotateView(float degrees);
    
    // Appearance controls
    void setWireframeMode(bool enabled);
    void setVisualizationMode(VisualizationMode mode);
    void setAdaptiveResolution(bool enabled);
    void setAdaptiveDetailFactor(float factor);
    void clearHeightCache();
    void setVisualizationType(int type);
    
    // Data updates
    void updateHeightData();
    bool generateGlobeMesh();
    
    // Get current rotation angle
    float getCurrentRotation() const;
    
    // Flag to indicate if the tool is active
    bool isActive() const { return active; }
    void setActive(bool enabled);

private:
    const World& world;
    bool active;
    bool wireframeMode;
    int visualizationType;
    bool shadersLoaded;
    VisualizationMode visualizationMode;
    bool useAdaptiveResolution;
    float adaptiveDetailFactor;
    bool meshDirty;
    
    // Camera settings
    glm::vec3 cameraPosition;
    glm::vec3 cameraTarget;
    float zoom;
    float rotationAngle;
    
    // OpenGL resources
    GLuint vao, vbo, ebo;
    GLuint shaderProgram;
    size_t indexCount;
    
    // Height sampling
    std::unordered_map<glm::ivec2, HeightSample, IVec2Hash> heightSampleCache;
    std::mutex cacheMutex;
    std::atomic<bool> updateInProgress;
    std::future<void> updateFuture;
    
    // Mesh generation
    bool loadShaders();
    void generateAdaptiveMesh();
    void createFallbackSphere();
    
    // Height sampling
    double sampleHeight(const glm::dvec3& direction);
    double sampleFromChunks(const glm::dvec3& direction);
    double sampleProcedural(const glm::dvec3& direction);
    float generateHeight(const glm::vec3& pos);
    void updateHeightDataAsync();
    
    // Resource management
    void releaseResources();
    
    // Helper functions
    bool needsMeshUpdate() const;
};

#endif // GOD_VIEW_DEBUG_TOOL_HPP