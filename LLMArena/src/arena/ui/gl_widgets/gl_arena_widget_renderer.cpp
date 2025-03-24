// src/arena/ui/gl_widgets/gl_arena_widget_renderer.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include <QDebug>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMatrix4x4>
#include <QVector3D>
#include <cmath>

// Rendering methods for GLArenaWidget

void GLArenaWidget::createFloor(double radius) {
    // Create floor VAO and buffer
    m_floorVAO.create();
    m_floorVAO.bind();
    
    // Create vertex buffer
    m_floorVBO.create();
    m_floorVBO.bind();
    
    // Simple floor: just a quad centered at the origin at y=0
    float vertices[] = {
        // Position (x,y,z), Normal (nx,ny,nz), Color (r,g,b), TexCoord (u,v)
        -radius, 0.0, -radius,  0.0, 1.0, 0.0,  0.5, 0.5, 0.5,  0.0, 0.0,
         radius, 0.0, -radius,  0.0, 1.0, 0.0,  0.5, 0.5, 0.5,  1.0, 0.0,
         radius, 0.0,  radius,  0.0, 1.0, 0.0,  0.5, 0.5, 0.5,  1.0, 1.0,
        -radius, 0.0,  radius,  0.0, 1.0, 0.0,  0.5, 0.5, 0.5,  0.0, 1.0
    };
    
    // Upload vertex data
    m_floorVBO.allocate(vertices, sizeof(vertices));
    
    // Create index buffer
    m_floorIBO.create();
    m_floorIBO.bind();
    
    // Simple quad indices
    GLuint indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    
    // Upload index data
    m_floorIBO.allocate(indices, sizeof(indices));
    m_floorIndexCount = 6;
    
    // Set up vertex attributes
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), nullptr);
    
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), 
                         reinterpret_cast<void*>(3 * sizeof(float)));
    
    // Color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), 
                         reinterpret_cast<void*>(6 * sizeof(float)));
    
    // Texture Coordinates
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), 
                         reinterpret_cast<void*>(9 * sizeof(float)));
    
    // Unbind buffers in reverse order
    m_floorIBO.release();
    m_floorVBO.release();
    m_floorVAO.release();
    
    // Log success
    qDebug() << "Floor geometry created successfully: radius =" << radius 
             << "VAO =" << m_floorVAO.isCreated() 
             << "VBO =" << m_floorVBO.isCreated() 
             << "IBO =" << m_floorIBO.isCreated()
             << "Indices =" << m_floorIndexCount;
}

void GLArenaWidget::createGrid(double size, int divisions) {
    // Create grid VAO and buffer
    m_gridVAO.create();
    m_gridVAO.bind();
    
    // Create vertex buffer
    m_gridVBO.create();
    m_gridVBO.bind();
    
    // Calculate number of lines in each direction
    int lineCount = (divisions + 1) * 2;
    
    // Calculate spacing between lines
    float spacing = size / divisions;
    
    // Create an array to hold all the grid line vertices
    // Each line has 2 vertices, each vertex has position (x,y,z) and color (r,g,b)
    std::vector<float> vertices;
    vertices.reserve(lineCount * 2 * 6); // 2 vertices per line, 6 floats per vertex
    
    // Create grid lines along X axis
    for (int i = 0; i <= divisions; ++i) {
        float pos = -size/2 + i * spacing;
        
        // First endpoint
        vertices.push_back(-size/2);  // x
        vertices.push_back(0.01f);    // y (slightly above ground to avoid z-fighting)
        vertices.push_back(pos);      // z
        vertices.push_back(0.3f);     // r
        vertices.push_back(0.3f);     // g
        vertices.push_back(0.3f);     // b
        
        // Second endpoint
        vertices.push_back(size/2);   // x
        vertices.push_back(0.01f);    // y
        vertices.push_back(pos);      // z
        vertices.push_back(0.3f);     // r
        vertices.push_back(0.3f);     // g
        vertices.push_back(0.3f);     // b
    }
    
    // Create grid lines along Z axis
    for (int i = 0; i <= divisions; ++i) {
        float pos = -size/2 + i * spacing;
        
        // First endpoint
        vertices.push_back(pos);      // x
        vertices.push_back(0.01f);    // y
        vertices.push_back(-size/2);  // z
        vertices.push_back(0.3f);     // r
        vertices.push_back(0.3f);     // g
        vertices.push_back(0.3f);     // b
        
        // Second endpoint
        vertices.push_back(pos);      // x
        vertices.push_back(0.01f);    // y
        vertices.push_back(size/2);   // z
        vertices.push_back(0.3f);     // r
        vertices.push_back(0.3f);     // g
        vertices.push_back(0.3f);     // b
    }
    
    // Upload vertex data
    m_gridVBO.allocate(vertices.data(), vertices.size() * sizeof(float));
    m_gridVertexCount = vertices.size() / 6; // 6 floats per vertex
    
    // Set up vertex attributes
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    
    // Color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 
                         reinterpret_cast<void*>(3 * sizeof(float)));
    
    // Unbind VBO and VAO
    m_gridVBO.release();
    m_gridVAO.release();
    
    // Log success
    qDebug() << "Grid created with" << m_gridVertexCount << "vertices"
             << "VAO =" << m_gridVAO.isCreated() 
             << "VBO =" << m_gridVBO.isCreated();
}

void GLArenaWidget::renderFloor() {
    // Safety check
    if (!m_billboardProgram || !m_billboardProgram->isLinked() || !m_floorVAO.isCreated()) {
        qWarning() << "Cannot render floor: shader or VAO not initialized";
        return;
    }
    
    // Bind shader
    m_billboardProgram->bind();
    
    // Set up model view matrix (identity - floor is at origin)
    QMatrix4x4 modelView;
    modelView.setToIdentity();
    m_billboardProgram->setUniformValue("modelView", modelView);
    
    // Set up projection matrix
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    
    // Set lighting
    m_billboardProgram->setUniformValue("lightPos", QVector3D(0, 20, 0));
    m_billboardProgram->setUniformValue("useTexture", false);
    
    // Bind floor VAO
    m_floorVAO.bind();
    
    // Make sure the index buffer is bound
    m_floorIBO.bind();
    
    // Draw triangles with indices
    glDrawElements(GL_TRIANGLES, m_floorIndexCount, GL_UNSIGNED_INT, nullptr);
    
    // Unbind buffers
    m_floorIBO.release();
    m_floorVAO.release();
    
    // Unbind shader
    m_billboardProgram->release();
}

void GLArenaWidget::renderGrid() {
    // Safety check
    if (!m_billboardProgram || !m_billboardProgram->isLinked() || !m_gridVAO.isCreated()) {
        return;
    }
    
    // Bind shader
    m_billboardProgram->bind();
    
    // Set up model view matrix (identity - grid is at origin)
    QMatrix4x4 modelView;
    modelView.setToIdentity();
    m_billboardProgram->setUniformValue("modelView", modelView);
    
    // Set up projection matrix
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    
    // Turn off lighting for grid
    m_billboardProgram->setUniformValue("useTexture", false);
    m_billboardProgram->setUniformValue("useColor", true);
    
    // Bind grid VAO
    m_gridVAO.bind();
    
    // Draw lines
    glDrawArrays(GL_LINES, 0, m_gridVertexCount);
    
    // Unbind VAO
    m_gridVAO.release();
    
    // Unbind shader
    m_billboardProgram->release();
}

void GLArenaWidget::renderWalls() {
    // Safety check
    if (!m_billboardProgram || !m_billboardProgram->isLinked()) {
        return;
    }
    
    // Bind shader
    m_billboardProgram->bind();
    
    // Set lighting
    m_billboardProgram->setUniformValue("lightPos", QVector3D(0, 20, 0));
    m_billboardProgram->setUniformValue("useTexture", false);
    m_billboardProgram->setUniformValue("useColor", true);
    
    // Render each wall
    for (const auto& wall : m_walls) {
        // Safety check
        if (!wall.vao || !wall.vao->isCreated() || !wall.ibo || !wall.ibo->isCreated()) {
            continue;
        }
        
        // Set up model matrix (identity - walls are positioned in world space)
        QMatrix4x4 modelView;
        modelView.setToIdentity();
        m_billboardProgram->setUniformValue("modelView", modelView);
        
        // Set up projection matrix
        m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
        
        // Bind wall VAO
        wall.vao->bind();
        wall.ibo->bind();
        
        // Draw wall triangles
        glDrawElements(GL_TRIANGLES, wall.indexCount, GL_UNSIGNED_INT, nullptr);
        
        // Unbind buffers
        wall.ibo->release();
        wall.vao->release();
    }
    
    // Unbind shader
    m_billboardProgram->release();
}

void GLArenaWidget::createArena(double radius, double wallHeight) {
    // Store the arena parameters
    m_arenaRadius = radius;
    m_wallHeight = wallHeight;
    
    // Create floor
    createFloor(radius);
    
    // Create grid
    createGrid(radius * 2, 16);
    
    // Clear any existing walls
    m_walls.clear();
    
    // Create four walls for rectangular arena
    
    // North wall (positive Z)
    createWallGeometry(
        QVector3D(0.0f, wallHeight / 2.0f, radius), // Position at center of wall
        QVector3D(radius * 2.0f, wallHeight, 0.2f), // Dimensions (x, y, z)
        QVector3D(0.0f, 0.0f, 0.0f)                // No rotation
    );
    
    // South wall (negative Z)
    createWallGeometry(
        QVector3D(0.0f, wallHeight / 2.0f, -radius), // Position
        QVector3D(radius * 2.0f, wallHeight, 0.2f),  // Dimensions
        QVector3D(0.0f, 0.0f, 0.0f)                 // No rotation
    );
    
    // East wall (positive X)
    createWallGeometry(
        QVector3D(radius, wallHeight / 2.0f, 0.0f), // Position
        QVector3D(0.2f, wallHeight, radius * 2.0f), // Dimensions - note z is width now
        QVector3D(0.0f, 0.0f, 0.0f)                // No rotation
    );
    
    // West wall (negative X)
    createWallGeometry(
        QVector3D(-radius, wallHeight / 2.0f, 0.0f), // Position
        QVector3D(0.2f, wallHeight, radius * 2.0f),  // Dimensions
        QVector3D(0.0f, 0.0f, 0.0f)                 // No rotation
    );
    
    qDebug() << "Arena created with" << m_walls.size() << "walls";
}

void GLArenaWidget::createWallGeometry(const QVector3D& position, const QVector3D& dimensions, const QVector3D& rotation) {
    // Create a new wall geometry
    WallGeometry wall;
    
    // Create VAO
    wall.vao = std::make_unique<QOpenGLVertexArrayObject>();
    wall.vao->create();
    wall.vao->bind();
    
    // Create VBO
    wall.vbo = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    wall.vbo->create();
    wall.vbo->bind();
    
    // Half dimensions for vertex positions
    float halfWidth = dimensions.x() / 2.0f;
    float halfHeight = dimensions.y() / 2.0f;
    float halfDepth = dimensions.z() / 2.0f;
    
    // Wall color (grey)
    float r = 0.7f;
    float g = 0.7f;
    float b = 0.7f;
    
    // Create vertices for the wall
    // Format: position (x,y,z), normal (nx,ny,nz), color (r,g,b), texcoord (u,v)
    const float vertices[] = {
        // Front face
        -halfWidth, -halfHeight,  halfDepth,  0.0f, 0.0f,  1.0f,  r, g, b,  0.0f, 0.0f,
         halfWidth, -halfHeight,  halfDepth,  0.0f, 0.0f,  1.0f,  r, g, b,  1.0f, 0.0f,
         halfWidth,  halfHeight,  halfDepth,  0.0f, 0.0f,  1.0f,  r, g, b,  1.0f, 1.0f,
        -halfWidth,  halfHeight,  halfDepth,  0.0f, 0.0f,  1.0f,  r, g, b,  0.0f, 1.0f,
        
        // Back face
        -halfWidth, -halfHeight, -halfDepth,  0.0f, 0.0f, -1.0f,  r, g, b,  1.0f, 0.0f,
        -halfWidth,  halfHeight, -halfDepth,  0.0f, 0.0f, -1.0f,  r, g, b,  1.0f, 1.0f,
         halfWidth,  halfHeight, -halfDepth,  0.0f, 0.0f, -1.0f,  r, g, b,  0.0f, 1.0f,
         halfWidth, -halfHeight, -halfDepth,  0.0f, 0.0f, -1.0f,  r, g, b,  0.0f, 0.0f,
        
        // Left face
        -halfWidth,  halfHeight,  halfDepth, -1.0f, 0.0f,  0.0f,  r, g, b,  1.0f, 1.0f,
        -halfWidth,  halfHeight, -halfDepth, -1.0f, 0.0f,  0.0f,  r, g, b,  0.0f, 1.0f,
        -halfWidth, -halfHeight, -halfDepth, -1.0f, 0.0f,  0.0f,  r, g, b,  0.0f, 0.0f,
        -halfWidth, -halfHeight,  halfDepth, -1.0f, 0.0f,  0.0f,  r, g, b,  1.0f, 0.0f,
        
        // Right face
         halfWidth,  halfHeight,  halfDepth,  1.0f, 0.0f,  0.0f,  r, g, b,  0.0f, 1.0f,
         halfWidth, -halfHeight,  halfDepth,  1.0f, 0.0f,  0.0f,  r, g, b,  0.0f, 0.0f,
         halfWidth, -halfHeight, -halfDepth,  1.0f, 0.0f,  0.0f,  r, g, b,  1.0f, 0.0f,
         halfWidth,  halfHeight, -halfDepth,  1.0f, 0.0f,  0.0f,  r, g, b,  1.0f, 1.0f,
        
        // Bottom face
        -halfWidth, -halfHeight, -halfDepth,  0.0f, -1.0f, 0.0f,  r, g, b,  0.0f, 0.0f,
         halfWidth, -halfHeight, -halfDepth,  0.0f, -1.0f, 0.0f,  r, g, b,  1.0f, 0.0f,
         halfWidth, -halfHeight,  halfDepth,  0.0f, -1.0f, 0.0f,  r, g, b,  1.0f, 1.0f,
        -halfWidth, -halfHeight,  halfDepth,  0.0f, -1.0f, 0.0f,  r, g, b,  0.0f, 1.0f,
        
        // Top face
        -halfWidth,  halfHeight, -halfDepth,  0.0f,  1.0f, 0.0f,  r, g, b,  0.0f, 0.0f,
        -halfWidth,  halfHeight,  halfDepth,  0.0f,  1.0f, 0.0f,  r, g, b,  0.0f, 1.0f,
         halfWidth,  halfHeight,  halfDepth,  0.0f,  1.0f, 0.0f,  r, g, b,  1.0f, 1.0f,
         halfWidth,  halfHeight, -halfDepth,  0.0f,  1.0f, 0.0f,  r, g, b,  1.0f, 0.0f
    };
    
    // Upload vertex data
    wall.vbo->allocate(vertices, sizeof(vertices));
    
    // Create IBO
    wall.ibo = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer);
    wall.ibo->create();
    wall.ibo->bind();
    
    // Create indices for the wall (6 faces, 2 triangles each, 3 vertices per triangle)
    const GLuint indices[] = {
        // Front face
        0, 1, 2, 2, 3, 0,
        // Back face
        4, 5, 6, 6, 7, 4,
        // Left face
        8, 9, 10, 10, 11, 8,
        // Right face
        12, 13, 14, 14, 15, 12,
        // Bottom face
        16, 17, 18, 18, 19, 16,
        // Top face
        20, 21, 22, 22, 23, 20
    };
    
    // Upload index data
    wall.ibo->allocate(indices, sizeof(indices));
    wall.indexCount = sizeof(indices) / sizeof(GLuint);
    
    // Set up vertex attributes
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), nullptr);
    
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), 
                        reinterpret_cast<void*>(3 * sizeof(float)));
    
    // Color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), 
                        reinterpret_cast<void*>(6 * sizeof(float)));
    
    // Texture coordinates
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), 
                        reinterpret_cast<void*>(9 * sizeof(float)));
    
    // Release buffers
    wall.ibo->release();
    wall.vbo->release();
    wall.vao->release();
    
    // Add the wall to the walls vector
    m_walls.push_back(std::move(wall));
}