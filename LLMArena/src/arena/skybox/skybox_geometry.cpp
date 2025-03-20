// src/arena/skybox/skybox_geometry.cpp
#include "../../include/voxel/sky_system.h"
#include <QDebug>

void SkySystem::createSkyboxGeometry() {
    // Use a much simpler approach - just create a simple quad for sky background
    // This will reduce potential problems with complex geometry
    
    float vertices[] = {
        // Position (screenspace quad)
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f
    };
    
    // Create and bind VAO
    m_skyboxVAO.create();
    m_skyboxVAO.bind();
    
    // Create and bind VBO
    m_skyboxVBO.create();
    m_skyboxVBO.bind();
    m_skyboxVBO.allocate(vertices, sizeof(vertices));
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    
    // Unbind
    m_skyboxVAO.release();
    m_skyboxVBO.release();
}

void SkySystem::createCelestialGeometry() {
    // Create a quad for billboard rendering of sun and moon
    float celestialVertices[] = {
        // Positions          // Texture Coords
        -1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,   1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f,   0.0f, 1.0f
    };
    
    // Create and bind VAO
    m_celestialVAO.create();
    m_celestialVAO.bind();
    
    // Create and bind VBO
    m_celestialVBO.create();
    m_celestialVBO.bind();
    m_celestialVBO.allocate(celestialVertices, sizeof(celestialVertices));
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 
                         reinterpret_cast<void*>(3 * sizeof(float)));
    
    // Unbind
    m_celestialVAO.release();
    m_celestialVBO.release();
}