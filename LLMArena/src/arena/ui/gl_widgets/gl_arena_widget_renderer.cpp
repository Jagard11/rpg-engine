// src/arena/ui/gl_widgets/gl_arena_widget_renderer.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include <QDebug>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMatrix4x4>
#include <QVector3D>
#include <QPainter>
#include <cmath>

// Modified createFloor method to use vertex array only (no indices)
void GLArenaWidget::createFloor(double radius)
{
    qDebug() << "Creating floor with radius" << radius;
    
    // Clean up any existing resources
    if (m_floorVAO.isCreated()) {
        m_floorVAO.destroy();
    }
    
    if (m_floorVBO.isCreated()) {
        m_floorVBO.destroy();
    }
    
    // Clean up IBO but we won't use it anymore
    if (m_floorIBO.isCreated()) {
        m_floorIBO.destroy();
    }
    
    // Create a simple square floor with explicit triangles for glDrawArrays
    float halfSize = static_cast<float>(radius);
    
    // Create floor with explicit triangle vertices - 6 vertices total (2 triangles)
    // Format: Position(3) + Normal(3) + Color(3)
    float vertices[] = {
        // First triangle
        -halfSize, 0.0f, -halfSize,   0.0f, 1.0f, 0.0f,   0.5f, 0.5f, 0.5f,  // Bottom-left
         halfSize, 0.0f, -halfSize,   0.0f, 1.0f, 0.0f,   0.5f, 0.5f, 0.5f,  // Bottom-right
         halfSize, 0.0f,  halfSize,   0.0f, 1.0f, 0.0f,   0.5f, 0.5f, 0.5f,  // Top-right
        
        // Second triangle
        -halfSize, 0.0f, -halfSize,   0.0f, 1.0f, 0.0f,   0.5f, 0.5f, 0.5f,  // Bottom-left
         halfSize, 0.0f,  halfSize,   0.0f, 1.0f, 0.0f,   0.5f, 0.5f, 0.5f,  // Top-right
        -halfSize, 0.0f,  halfSize,   0.0f, 1.0f, 0.0f,   0.5f, 0.5f, 0.5f   // Top-left
    };
    
    // Store number of vertices
    m_floorVertexCount = 6;  // 6 vertices for 2 triangles
    
    // Create and bind VAO
    m_floorVAO.create();
    m_floorVAO.bind();
    
    // Create and bind VBO
    m_floorVBO.create();
    m_floorVBO.bind();
    m_floorVBO.allocate(vertices, sizeof(vertices));
    
    // Set up vertex attributes
    // Position (3 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), nullptr);
    
    // Normal (3 floats)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), 
                         reinterpret_cast<void*>(3 * sizeof(float)));
    
    // Color (3 floats)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), 
                         reinterpret_cast<void*>(6 * sizeof(float)));
    
    // Unbind buffers and VAO
    m_floorVBO.release();
    m_floorVAO.release();
    
    qDebug() << "Floor geometry created successfully: radius =" << radius 
            << "VAO =" << m_floorVAO.isCreated() 
            << "VBO =" << m_floorVBO.isCreated()
            << "Vertices =" << m_floorVertexCount;
}

// Modified renderFloor method to use glDrawArrays instead of glDrawElements
void GLArenaWidget::renderFloor()
{
    // Skip rendering if VAO is not created
    if (!m_floorVAO.isCreated()) {
        qWarning() << "Cannot render floor: VAO not created";
        return;
    }
    
    // Skip rendering if index count is invalid 
    if (m_floorIndexCount <= 0) {
        qWarning() << "Cannot render floor: Invalid index count";
        return;
    }
    
    // Bind shader program
    if (!m_billboardProgram->bind()) {
        qWarning() << "Failed to bind shader for floor rendering";
        return;
    }
    
    // Set up model matrix
    QMatrix4x4 modelMatrix;
    modelMatrix.setToIdentity();
    
    // Set uniforms
    m_billboardProgram->setUniformValue("model", modelMatrix);
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    
    // Bind VAO
    m_floorVAO.bind();
    
    // For debugging - just render the floor using arrays instead of elements
    // This will show the floor without using indices
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    // Release VAO and shader
    m_floorVAO.release();
    m_billboardProgram->release();
}

void GLArenaWidget::createGrid(double size, int divisions) {
    // Cleanup previous geometry if it exists
    if (m_gridVAO.isCreated()) {
        m_gridVAO.destroy();
    }
    
    if (m_gridVBO.isCreated()) {
        m_gridVBO.destroy();
    }
    
    // Create VAO
    if (!m_gridVAO.create()) {
        qWarning() << "Failed to create grid VAO";
        return;
    }
    
    m_gridVAO.bind();
    
    // Create VBO
    if (!m_gridVBO.create()) {
        qWarning() << "Failed to create grid VBO";
        m_gridVAO.release();
        return;
    }
    
    m_gridVBO.bind();
    
    // Calculate the number of vertices needed
    int numLines = divisions + 1;
    int numVertices = numLines * 4;  // 2 vertices per line, 2 lines per grid line (X and Z)
    
    // Create vertex data for grid lines
    QVector<float> vertices;
    vertices.reserve(numVertices * 3);  // 3 floats (x, y, z) per vertex
    
    float step = size / divisions;
    float halfSize = size / 2.0f;
    float y = 0.01f;  // Slightly above floor to avoid z-fighting
    
    // Add lines along X-axis
    for (int i = 0; i <= divisions; i++) {
        float z = -halfSize + i * step;
        
        // Line from -X to +X at current Z
        vertices.append(-halfSize);
        vertices.append(y);
        vertices.append(z);
        
        vertices.append(halfSize);
        vertices.append(y);
        vertices.append(z);
    }
    
    // Add lines along Z-axis
    for (int i = 0; i <= divisions; i++) {
        float x = -halfSize + i * step;
        
        // Line from -Z to +Z at current X
        vertices.append(x);
        vertices.append(y);
        vertices.append(-halfSize);
        
        vertices.append(x);
        vertices.append(y);
        vertices.append(halfSize);
    }
    
    m_gridVBO.allocate(vertices.data(), vertices.size() * sizeof(float));
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0);  // Position only
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    
    // Store the number of vertices
    m_gridVertexCount = vertices.size() / 3;
    
    // Release bindings
    m_gridVBO.release();
    m_gridVAO.release();
    
    qDebug() << "Grid created with" << m_gridVertexCount << "vertices"
            << "VAO =" << m_gridVAO.isCreated()
            << "VBO =" << m_gridVBO.isCreated();
}

void GLArenaWidget::renderGrid() {
    // Skip if shader is not available
    if (!m_billboardProgram || !m_billboardProgram->isLinked()) {
        return;
    }
    
    // Skip if grid geometry is not created
    if (!m_gridVAO.isCreated() || !m_gridVBO.isCreated() || m_gridVertexCount == 0) {
        return;
    }
    
    // Bind shader
    m_billboardProgram->bind();
    
    // Set uniforms for grid rendering
    QMatrix4x4 modelMatrix;
    modelMatrix.setToIdentity();
    
    m_billboardProgram->setUniformValue("model", modelMatrix);
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    
    // Set grid color (dark gray)
    QVector4D gridColor(0.3f, 0.3f, 0.3f, 0.7f);
    m_billboardProgram->setUniformValue("color", gridColor);
    m_billboardProgram->setUniformValue("useTexture", false);  // No texture for grid
    
    // Bind VAO
    m_gridVAO.bind();
    
    // Enable blending for semi-transparent grid
    GLboolean blendEnabled;
    glGetBooleanv(GL_BLEND, &blendEnabled);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw grid lines
    glDrawArrays(GL_LINES, 0, m_gridVertexCount);
    
    // Restore previous blend state
    if (!blendEnabled) {
        glDisable(GL_BLEND);
    }
    
    // Release bindings
    m_gridVAO.release();
    m_billboardProgram->release();
}

void GLArenaWidget::createArena(double radius, double wallHeight) {
    qDebug() << "Creating arena with radius" << radius << "and wall height" << wallHeight;
    
    // Store parameters
    m_arenaRadius = radius;
    m_wallHeight = wallHeight;
    
    // Create floor
    createFloor(radius);
    
    // Create grid
    createGrid(radius * 2, 10);
    
    // Clear any existing walls
    m_walls.clear();
    
    // Create 4 walls for a rectangular arena
    // North wall (positive Z)
    QVector3D northWallPos(0, wallHeight / 2, radius);
    QVector3D northWallDim(radius * 2, wallHeight, 0.2);
    QVector3D northWallRot(0, 0, 0);
    createWallGeometry(northWallPos, northWallDim, northWallRot);
    
    // South wall (negative Z)
    QVector3D southWallPos(0, wallHeight / 2, -radius);
    QVector3D southWallDim(radius * 2, wallHeight, 0.2);
    QVector3D southWallRot(0, 0, 0);
    createWallGeometry(southWallPos, southWallDim, southWallRot);
    
    // East wall (positive X)
    QVector3D eastWallPos(radius, wallHeight / 2, 0);
    QVector3D eastWallDim(0.2, wallHeight, radius * 2);
    QVector3D eastWallRot(0, 0, 0);
    createWallGeometry(eastWallPos, eastWallDim, eastWallRot);
    
    // West wall (negative X)
    QVector3D westWallPos(-radius, wallHeight / 2, 0);
    QVector3D westWallDim(0.2, wallHeight, radius * 2);
    QVector3D westWallRot(0, 0, 0);
    createWallGeometry(westWallPos, westWallDim, westWallRot);
    
    qDebug() << "Arena created with 4 walls";
}

void GLArenaWidget::createWallGeometry(const QVector3D& position, const QVector3D& dimensions, const QVector3D& rotation) {
    // Position, dimensions, and rotation are used to create the wall geometry
    // Let's create a model matrix that we'll apply to the basic box vertices
    QMatrix4x4 modelMatrix;
    modelMatrix.setToIdentity();
    modelMatrix.translate(position);
    
    // Apply rotation if any
    if (rotation.x() != 0.0f) modelMatrix.rotate(rotation.x(), 1.0f, 0.0f, 0.0f);
    if (rotation.y() != 0.0f) modelMatrix.rotate(rotation.y(), 0.0f, 1.0f, 0.0f);
    if (rotation.z() != 0.0f) modelMatrix.rotate(rotation.z(), 0.0f, 0.0f, 1.0f);
    
    // Create a new wall geometry entry
    WallGeometry wall;
    wall.vao = std::make_unique<QOpenGLVertexArrayObject>();
    wall.vbo = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    wall.ibo = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer);
    
    // Create VAO
    if (!wall.vao->create()) {
        qWarning() << "Failed to create wall VAO";
        return;
    }
    
    wall.vao->bind();
    
    // Create VBO
    if (!wall.vbo->create()) {
        qWarning() << "Failed to create wall VBO";
        wall.vao->release();
        return;
    }
    
    wall.vbo->bind();
    
    // Calculate half dimensions for vertex positions
    float halfWidth = dimensions.x() / 2.0f;
    float halfHeight = dimensions.y() / 2.0f;
    float halfDepth = dimensions.z() / 2.0f;
    
    // Create box vertices with position, normal, and texture coordinates
    float vertices[] = {
        // Front face
        -halfWidth, -halfHeight,  halfDepth,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         halfWidth, -halfHeight,  halfDepth,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         halfWidth,  halfHeight,  halfDepth,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -halfWidth,  halfHeight,  halfDepth,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        
        // Back face
        -halfWidth, -halfHeight, -halfDepth,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -halfWidth,  halfHeight, -halfDepth,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
         halfWidth,  halfHeight, -halfDepth,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         halfWidth, -halfHeight, -halfDepth,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
        
        // Top face
        -halfWidth,  halfHeight, -halfDepth,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -halfWidth,  halfHeight,  halfDepth,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         halfWidth,  halfHeight,  halfDepth,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         halfWidth,  halfHeight, -halfDepth,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        
        // Bottom face
        -halfWidth, -halfHeight, -halfDepth,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         halfWidth, -halfHeight, -halfDepth,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         halfWidth, -halfHeight,  halfDepth,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
        -halfWidth, -halfHeight,  halfDepth,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
        
        // Right face
         halfWidth, -halfHeight, -halfDepth,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         halfWidth,  halfHeight, -halfDepth,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         halfWidth,  halfHeight,  halfDepth,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         halfWidth, -halfHeight,  halfDepth,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        
        // Left face
        -halfWidth, -halfHeight, -halfDepth, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -halfWidth, -halfHeight,  halfDepth, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -halfWidth,  halfHeight,  halfDepth, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -halfWidth,  halfHeight, -halfDepth, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f
    };
    
    wall.vbo->allocate(vertices, sizeof(vertices));
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0);  // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    
    glEnableVertexAttribArray(1);  // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 
                       reinterpret_cast<void*>(3 * sizeof(float)));
    
    glEnableVertexAttribArray(2);  // Texture coordinates
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 
                       reinterpret_cast<void*>(6 * sizeof(float)));
    
    // Create IBO
    if (!wall.ibo->create()) {
        qWarning() << "Failed to create wall IBO";
        wall.vbo->release();
        wall.vao->release();
        return;
    }
    
    wall.ibo->bind();
    
    // Indices for 12 triangles (6 faces)
    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,       // Front face
        4, 5, 6, 6, 7, 4,       // Back face
        8, 9, 10, 10, 11, 8,    // Top face
        12, 13, 14, 14, 15, 12, // Bottom face
        16, 17, 18, 18, 19, 16, // Right face
        20, 21, 22, 22, 23, 20  // Left face
    };
    
    wall.ibo->allocate(indices, sizeof(indices));
    wall.indexCount = 36;  // 36 indices total (6 faces, 2 triangles per face, 3 vertices per triangle)
    
    // Release bindings
    wall.ibo->release();
    wall.vbo->release();
    wall.vao->release();
    
        // Add to walls vector with move semantics
    m_walls.push_back(std::move(wall));
}

void GLArenaWidget::renderWalls() {
    // Skip if shader is not available
    if (!m_billboardProgram || !m_billboardProgram->isLinked()) {
        return;
    }
    
    // Bind shader
    m_billboardProgram->bind();
    
    // Set up common parameters
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    m_billboardProgram->setUniformValue("useTexture", false);  // No texture for walls
    
    // Wall color (light blue-gray, semi-transparent)
    QVector4D wallColor(0.7f, 0.7f, 0.8f, 0.8f);
    
    // Enable blending for semi-transparent walls
    GLboolean blendEnabled;
    glGetBooleanv(GL_BLEND, &blendEnabled);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw each wall
    for (const auto& wall : m_walls) {
        // Skip walls with invalid geometry
        if (!wall.vao || !wall.vao->isCreated() || 
            !wall.vbo || !wall.vbo->isCreated() || 
            !wall.ibo || !wall.ibo->isCreated()) {
            continue;
        }
        
        // For each wall, just use an identity model matrix
        // The wall vertices already contain the transformed positions
        QMatrix4x4 modelMatrix;
        modelMatrix.setToIdentity();
        
        m_billboardProgram->setUniformValue("model", modelMatrix);
        m_billboardProgram->setUniformValue("color", wallColor);
        
        // Bind VAO and IBO
        wall.vao->bind();
        wall.ibo->bind();
        
        // Draw wall
        glDrawElements(GL_TRIANGLES, wall.indexCount, GL_UNSIGNED_INT, nullptr);
        
        // Release bindings
        wall.ibo->release();
        wall.vao->release();
    }
    
    // Restore previous blend state
    if (!blendEnabled) {
        glDisable(GL_BLEND);
    }
    
    // Release shader
    m_billboardProgram->release();
}