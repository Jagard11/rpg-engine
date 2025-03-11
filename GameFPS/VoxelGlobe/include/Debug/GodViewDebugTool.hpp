// ./include/Debug/GodViewDebugTool.hpp
#ifndef GOD_VIEW_DEBUG_TOOL_HPP
#define GOD_VIEW_DEBUG_TOOL_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include "World/World.hpp"
#include "Graphics/GraphicsSettings.hpp"

/**
 * Debug tool that provides a god's eye view of the entire globe.
 * Useful for visualizing and debugging procedural terrain generation.
 */
class GodViewDebugTool {
public:
    GodViewDebugTool(const World& world);
    ~GodViewDebugTool();
    
    // Render the globe visualization
    void render(const GraphicsSettings& settings);
    
    // Set the position of the camera in the god view
    void setCameraPosition(const glm::vec3& position);
    
    // Set the target the camera is looking at
    void setCameraTarget(const glm::vec3& target);
    
    // Adjust the zoom level
    void setZoom(float zoom);
    
    // Rotate the view by specified degrees
    void rotateView(float degrees);
    
    // Toggle wireframe mode
    void setWireframeMode(bool enabled);
    
    // Set visualization type (height map, biomes, etc.)
    void setVisualizationType(int type);
    
    // Rotation-related methods
    float getCurrentRotation() const { return rotationAngle; }
    
    // Flag to indicate if the tool is active
    bool isActive() const { return active; }
    void setActive(bool enabled);

private:
    const World& world;
    bool active;
    bool wireframeMode;
    int visualizationType;
    bool shadersLoaded;
    
    // Camera settings
    glm::vec3 cameraPosition;
    glm::vec3 cameraTarget;
    float zoom;
    float rotationAngle;
    
    // OpenGL resources
    GLuint vao, vbo, ebo;
    GLuint shaderProgram;
    size_t indexCount;
    
    // Load shaders for rendering
    bool loadShaders();
    
    // Generate a sphere mesh for the globe
    bool generateGlobeMesh();
    
    // Generate height data based on position
    float generateHeight(const glm::vec3& pos);
    
    // Update the height data based on world state
    void updateHeightData();
};

#endif // GOD_VIEW_DEBUG_TOOL_HPP