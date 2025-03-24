// src/arena/ui/gl_widgets/gl_arena_widget_renderer.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include <QDebug>
#include <QtMath>

void GLArenaWidget::createFloor(double radius) {
    // First, clean up any existing resources
    if (m_floorVAO.isCreated()) {
        m_floorVAO.destroy();
    }
    
    if (m_floorVBO.isCreated()) {
        m_floorVBO.destroy();
    }
    
    if (m_floorIBO.isCreated()) {
        m_floorIBO.destroy();
    }

    // Create simple floor geometry
    struct Vertex {
        QVector3D position;
        QVector3D normal;
        QVector2D texCoord;
    };

    // Create a simple quad for the floor (with explicit float conversions)
    float fRadius = static_cast<float>(radius);  // Explicitly convert double to float
    Vertex vertices[] = {
        // Position                     Normal              TexCoord
        {{-fRadius, 0.0f, -fRadius}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ fRadius, 0.0f, -fRadius}, {0.0f, 1.0f, 0.0f}, {fRadius, 0.0f}},
        {{ fRadius, 0.0f,  fRadius}, {0.0f, 1.0f, 0.0f}, {fRadius, fRadius}},
        {{-fRadius, 0.0f,  fRadius}, {0.0f, 1.0f, 0.0f}, {0.0f, fRadius}}
    };

    // Define indices for the floor (two triangles forming a quad)
    GLuint indices[] = {
        0, 1, 2,  // First triangle
        0, 2, 3   // Second triangle
    };
    
    // Store the index count for rendering
    m_floorIndexCount = 6;

    // Create and bind VAO
    if (!m_floorVAO.create()) {
        qWarning() << "Failed to create floor VAO";
        return;
    }
    m_floorVAO.bind();

    // Create and bind vertex buffer
    if (!m_floorVBO.create()) {
        qWarning() << "Failed to create floor VBO";
        m_floorVAO.destroy();
        return;
    }
    m_floorVBO.bind();
    m_floorVBO.allocate(vertices, sizeof(vertices));

    // Create and bind index buffer
    if (!m_floorIBO.create()) {
        qWarning() << "Failed to create floor IBO";
        m_floorVBO.destroy();
        m_floorVAO.destroy();
        return;
    }
    m_floorIBO.bind();
    m_floorIBO.allocate(indices, sizeof(indices));

    // Set up vertex attributes
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                         reinterpret_cast<void*>(offsetof(Vertex, position)));
    
    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                         reinterpret_cast<void*>(offsetof(Vertex, normal)));
    
    // Texture coordinate attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                         reinterpret_cast<void*>(offsetof(Vertex, texCoord)));

    // Release resources - important to release in reverse order
    m_floorIBO.release();
    m_floorVBO.release();
    m_floorVAO.release();

    qDebug() << "Floor geometry created successfully: radius =" << radius
             << "VAO =" << m_floorVAO.isCreated()
             << "VBO =" << m_floorVBO.isCreated()
             << "IBO =" << m_floorIBO.isCreated()
             << "Indices =" << m_floorIndexCount;
}

void GLArenaWidget::renderFloor() {
    // Check if all resources are properly initialized before rendering
    if (!m_floorVAO.isCreated() || !m_floorVBO.isCreated() || !m_floorIBO.isCreated() || m_floorIndexCount <= 0) {
        qWarning() << "Floor not initialized properly, skipping renderFloor"
                   << "VAO =" << m_floorVAO.isCreated()
                   << "VBO =" << m_floorVBO.isCreated()
                   << "IBO =" << m_floorIBO.isCreated()
                   << "Indices =" << m_floorIndexCount;
        return;
    }

    // Use shader program for floor (should be bound elsewhere)
    if (!m_billboardProgram || !m_billboardProgram->isLinked()) {
        qWarning() << "No valid shader program for floor rendering";
        return;
    }
    
    m_billboardProgram->bind();

    // Set uniform variables for shader
    m_billboardProgram->setUniformValue("model", QMatrix4x4());  // Identity matrix for floor
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    
    // Set floor color or texture if needed
    m_billboardProgram->setUniformValue("color", QVector4D(0.3f, 0.3f, 0.3f, 1.0f));  // Gray floor

    // Bind the VAO
    m_floorVAO.bind();
    
    // Make sure IBO is bound
    m_floorIBO.bind();

    // Draw the floor
    glDrawElements(GL_TRIANGLES, m_floorIndexCount, GL_UNSIGNED_INT, nullptr);

    // Unbind
    m_floorIBO.release();
    m_floorVAO.release();
    m_billboardProgram->release();
}

void GLArenaWidget::renderGrid() {
    // Check if the grid is initialized
    if (!m_gridVAO.isCreated() || !m_gridVBO.isCreated() || m_gridVertexCount <= 0) {
        return; // Skip rendering if not initialized
    }

    // Use shader program for grid
    if (!m_billboardProgram || !m_billboardProgram->isLinked()) {
        return;
    }
    
    m_billboardProgram->bind();

    // Set uniform variables
    m_billboardProgram->setUniformValue("model", QMatrix4x4());  // Identity matrix
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    m_billboardProgram->setUniformValue("color", QVector4D(0.5f, 0.5f, 0.5f, 0.5f));  // Semi-transparent grid

    // Bind the VAO
    m_gridVAO.bind();

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw grid lines
    glDrawArrays(GL_LINES, 0, m_gridVertexCount);

    // Disable blending
    glDisable(GL_BLEND);

    // Unbind
    m_gridVAO.release();
    m_billboardProgram->release();
}

void GLArenaWidget::createGrid(double size, int divisions) {
    // Clean up existing resources
    if (m_gridVAO.isCreated()) {
        m_gridVAO.destroy();
    }

    if (m_gridVBO.isCreated()) {
        m_gridVBO.destroy();
    }

    // Calculate spacing between grid lines
    double spacing = size / divisions;
    
    // Calculate the total number of vertices (2 per line, 2 * divisions+1 lines per axis)
    int lineCount = (divisions + 1) * 2; // Lines along X and Z axes
    m_gridVertexCount = lineCount * 2; // 2 vertices per line
    
    // Create and allocate vertex buffer
    QVector<QVector3D> vertices;
    vertices.reserve(m_gridVertexCount);
    
    // Create grid lines parallel to X axis
    for (int i = 0; i <= divisions; ++i) {
        double z = -size / 2 + i * spacing;
        vertices.append(QVector3D(-size / 2, 0.01f, z)); // Slightly above ground
        vertices.append(QVector3D(size / 2, 0.01f, z));
    }
    
    // Create grid lines parallel to Z axis
    for (int i = 0; i <= divisions; ++i) {
        double x = -size / 2 + i * spacing;
        vertices.append(QVector3D(x, 0.01f, -size / 2));
        vertices.append(QVector3D(x, 0.01f, size / 2));
    }

    // Create and bind VAO
    if (!m_gridVAO.create()) {
        qWarning() << "Failed to create grid VAO";
        return;
    }
    m_gridVAO.bind();

    // Create and bind VBO
    if (!m_gridVBO.create()) {
        qWarning() << "Failed to create grid VBO";
        m_gridVAO.destroy();
        return;
    }
    m_gridVBO.bind();
    m_gridVBO.allocate(vertices.constData(), vertices.size() * sizeof(QVector3D));

    // Set up vertex attributes - just position for grid
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), nullptr);

    // Release
    m_gridVBO.release();
    m_gridVAO.release();

    qDebug() << "Grid created with" << m_gridVertexCount << "vertices"
             << "VAO =" << m_gridVAO.isCreated()
             << "VBO =" << m_gridVBO.isCreated();
}

void GLArenaWidget::renderWalls() {
    // Check if walls are initialized
    if (m_walls.empty()) {
        return;
    }

    // Use shader program for walls
    if (!m_billboardProgram || !m_billboardProgram->isLinked()) {
        return;
    }
    
    m_billboardProgram->bind();

    // Set view and projection matrices
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);

    // Set wall color
    m_billboardProgram->setUniformValue("color", QVector4D(0.7f, 0.7f, 0.8f, 1.0f));

    // Draw each wall
    for (const auto& wall : m_walls) {
        if (!wall.vao->isCreated() || !wall.vbo->isCreated() || !wall.ibo->isCreated()) {
            continue;
        }

        // Bind wall VAO
        wall.vao->bind();
        wall.ibo->bind();

        // Set model matrix (identity for now)
        m_billboardProgram->setUniformValue("model", QMatrix4x4());

        // Draw wall
        glDrawElements(GL_TRIANGLES, wall.indexCount, GL_UNSIGNED_INT, nullptr);

        // Unbind
        wall.ibo->release();
        wall.vao->release();
    }

    // Release shader
    m_billboardProgram->release();
}

void GLArenaWidget::createArena(double radius, double wallHeight) {
    // Clean up existing walls
    m_walls.clear();

    // Define the four walls of a rectangular arena
    struct WallDefinition {
        QVector3D position;
        QVector3D dimensions;
        QVector3D rotation;
    };

    // Four walls of a rectangular arena
    WallDefinition wallDefs[] = {
        // North wall (+Z)
        { QVector3D(0, wallHeight / 2, radius), QVector3D(radius * 2, wallHeight, 0.2f), QVector3D(0, 0, 0) },
        // South wall (-Z)
        { QVector3D(0, wallHeight / 2, -radius), QVector3D(radius * 2, wallHeight, 0.2f), QVector3D(0, 0, 0) },
        // East wall (+X)
        { QVector3D(radius, wallHeight / 2, 0), QVector3D(0.2f, wallHeight, radius * 2), QVector3D(0, 0, 0) },
        // West wall (-X)
        { QVector3D(-radius, wallHeight / 2, 0), QVector3D(0.2f, wallHeight, radius * 2), QVector3D(0, 0, 0) }
    };

    // For each wall definition, create geometry
    for (const auto& def : wallDefs) {
        createWallGeometry(def.position, def.dimensions, def.rotation);
    }

    qDebug() << "Arena created with" << m_walls.size() << "walls";
}

void GLArenaWidget::createWallGeometry(const QVector3D& position, const QVector3D& dimensions, const QVector3D& rotation) {
    // Create a new wall geometry
    WallGeometry wall;
    wall.vao = std::make_unique<QOpenGLVertexArrayObject>();
    wall.vbo = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    wall.ibo = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer);

    // Create and bind VAO
    if (!wall.vao->create()) {
        qWarning() << "Failed to create wall VAO";
        return;
    }
    wall.vao->bind();

    // Create vertex and index data for a box
    struct Vertex {
        QVector3D position;
        QVector3D normal;
        QVector2D texCoord;
    };

    // Half dimensions for easier calculations
    float halfWidth = dimensions.x() / 2.0f;
    float halfHeight = dimensions.y() / 2.0f;
    float halfDepth = dimensions.z() / 2.0f;

    // 8 vertices for a box (cube)
    Vertex vertices[] = {
        // Front face (+Z)
        {{-halfWidth, -halfHeight, halfDepth}, {0, 0, 1}, {0, 0}},  // Bottom-left
        {{halfWidth, -halfHeight, halfDepth}, {0, 0, 1}, {1, 0}},   // Bottom-right
        {{halfWidth, halfHeight, halfDepth}, {0, 0, 1}, {1, 1}},    // Top-right
        {{-halfWidth, halfHeight, halfDepth}, {0, 0, 1}, {0, 1}},   // Top-left

        // Back face (-Z)
        {{halfWidth, -halfHeight, -halfDepth}, {0, 0, -1}, {0, 0}}, // Bottom-left
        {{-halfWidth, -halfHeight, -halfDepth}, {0, 0, -1}, {1, 0}},// Bottom-right
        {{-halfWidth, halfHeight, -halfDepth}, {0, 0, -1}, {1, 1}}, // Top-right
        {{halfWidth, halfHeight, -halfDepth}, {0, 0, -1}, {0, 1}},  // Top-left

        // Top face (+Y)
        {{-halfWidth, halfHeight, -halfDepth}, {0, 1, 0}, {0, 0}},  // Bottom-left
        {{halfWidth, halfHeight, -halfDepth}, {0, 1, 0}, {1, 0}},   // Bottom-right
        {{halfWidth, halfHeight, halfDepth}, {0, 1, 0}, {1, 1}},    // Top-right
        {{-halfWidth, halfHeight, halfDepth}, {0, 1, 0}, {0, 1}},   // Top-left

        // Bottom face (-Y)
        {{-halfWidth, -halfHeight, halfDepth}, {0, -1, 0}, {0, 0}}, // Bottom-left
        {{halfWidth, -halfHeight, halfDepth}, {0, -1, 0}, {1, 0}},  // Bottom-right
        {{halfWidth, -halfHeight, -halfDepth}, {0, -1, 0}, {1, 1}}, // Top-right
        {{-halfWidth, -halfHeight, -halfDepth}, {0, -1, 0}, {0, 1}},// Top-left

        // Right face (+X)
        {{halfWidth, -halfHeight, halfDepth}, {1, 0, 0}, {0, 0}},   // Bottom-left
        {{halfWidth, -halfHeight, -halfDepth}, {1, 0, 0}, {1, 0}},  // Bottom-right
        {{halfWidth, halfHeight, -halfDepth}, {1, 0, 0}, {1, 1}},   // Top-right
        {{halfWidth, halfHeight, halfDepth}, {1, 0, 0}, {0, 1}},    // Top-left

        // Left face (-X)
        {{-halfWidth, -halfHeight, -halfDepth}, {-1, 0, 0}, {0, 0}},// Bottom-left
        {{-halfWidth, -halfHeight, halfDepth}, {-1, 0, 0}, {1, 0}}, // Bottom-right
        {{-halfWidth, halfHeight, halfDepth}, {-1, 0, 0}, {1, 1}},  // Top-right
        {{-halfWidth, halfHeight, -halfDepth}, {-1, 0, 0}, {0, 1}}  // Top-left
    };

    // Apply position offset to all vertices
    for (auto& vertex : vertices) {
        // Apply rotation (simplified - just supporting Y-axis rotation for now)
        float angle = rotation.y() * M_PI / 180.0f;
        float cosA = cos(angle);
        float sinA = sin(angle);
        
        float x = vertex.position.x();
        float z = vertex.position.z();
        
        vertex.position.setX(x * cosA - z * sinA);
        vertex.position.setZ(x * sinA + z * cosA);
        
        // Apply position offset
        vertex.position += position;
    }

    // 36 indices for a box (6 faces * 2 triangles * 3 vertices)
    GLuint indices[] = {
        // Front face
        0, 1, 2, 0, 2, 3,
        // Back face
        4, 5, 6, 4, 6, 7,
        // Top face
        8, 9, 10, 8, 10, 11,
        // Bottom face
        12, 13, 14, 12, 14, 15,
        // Right face
        16, 17, 18, 16, 18, 19,
        // Left face
        20, 21, 22, 20, 22, 23
    };

    // Store index count
    wall.indexCount = sizeof(indices) / sizeof(indices[0]);

    // Create and bind VBO
    if (!wall.vbo->create()) {
        qWarning() << "Failed to create wall VBO";
        wall.vao->release();
        wall.vao->destroy();
        return;
    }
    wall.vbo->bind();
    wall.vbo->allocate(vertices, sizeof(vertices));

    // Create and bind IBO
    if (!wall.ibo->create()) {
        qWarning() << "Failed to create wall IBO";
        wall.vbo->release();
        wall.vbo->destroy();
        wall.vao->release();
        wall.vao->destroy();
        return;
    }
    wall.ibo->bind();
    wall.ibo->allocate(indices, sizeof(indices));

    // Set vertex attributes
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                         reinterpret_cast<void*>(offsetof(Vertex, position)));
    
    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                         reinterpret_cast<void*>(offsetof(Vertex, normal)));
    
    // Texture coordinate attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                         reinterpret_cast<void*>(offsetof(Vertex, texCoord)));

    // Release resources
    wall.ibo->release();
    wall.vbo->release();
    wall.vao->release();

    // Add wall to the list
    m_walls.push_back(std::move(wall));
}