// src/rendering/gl_arena/gl_arena_widget_geometry.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include <QDebug>
#include <cmath>

// Vertex struct for passing to OpenGL
struct Vertex {
    QVector3D position;
    QVector3D normal;
    QVector2D texCoord;
};

void GLArenaWidget::createArena(double radius, double wallHeight)
{
    // Store arena parameters
    m_arenaRadius = radius;
    m_wallHeight = wallHeight;
    
    // Create octagonal arena walls
    const int numWalls = 8;
    m_walls.clear();
    m_walls.resize(numWalls);
    
    for (int i = 0; i < numWalls; i++) {
        const double angle1 = 2.0 * M_PI * i / numWalls;
        const double angle2 = 2.0 * M_PI * (i + 1) / numWalls;
        
        const double x1 = radius * cos(angle1);
        const double z1 = radius * sin(angle1);
        const double x2 = radius * cos(angle2);
        const double z2 = radius * sin(angle2);
        
        // Compute wall normal (pointing inward)
        QVector3D wallDir(x2 - x1, 0.0, z2 - z1);
        QVector3D normal = QVector3D::crossProduct(wallDir, QVector3D(0.0, 1.0, 0.0)).normalized();
        normal = -normal; // Invert to point inward
        
        // Calculate wall length
        float wallLength = QVector2D(x2 - x1, z2 - z1).length();
        
        // Create vertices for wall
        Vertex vertices[4];
        // Bottom left
        vertices[0].position = QVector3D(x1, 0.0, z1);
        vertices[0].normal = normal;
        vertices[0].texCoord = QVector2D(0.0, 1.0);
        
        // Bottom right
        vertices[1].position = QVector3D(x2, 0.0, z2);
        vertices[1].normal = normal;
        vertices[1].texCoord = QVector2D(1.0, 1.0);
        
        // Top right
        vertices[2].position = QVector3D(x2, wallHeight, z2);
        vertices[2].normal = normal;
        vertices[2].texCoord = QVector2D(1.0, 0.0);
        
        // Top left
        vertices[3].position = QVector3D(x1, wallHeight, z1);
        vertices[3].normal = normal;
        vertices[3].texCoord = QVector2D(0.0, 0.0);
        
        // Indices for two triangles
        GLuint indices[6] = { 0, 1, 2, 2, 3, 0 };
        
        // Create OpenGL buffers - using the unique_ptr in WallGeometry
        WallGeometry& wall = m_walls[i];
        
        wall.vao->create();
        wall.vao->bind();
        
        wall.vbo->create();
        wall.vbo->bind();
        wall.vbo->allocate(vertices, 4 * sizeof(Vertex));
        
        // Enable vertex attributes
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        
        wall.ibo->create();
        wall.ibo->bind();
        wall.ibo->allocate(indices, 6 * sizeof(GLuint));
        
        wall.indexCount = 6;
        
        wall.vao->release();
        
        // Create wall entity in game scene
        GameEntity wallEntity;
        wallEntity.id = QString("arena_wall_%1").arg(i);
        wallEntity.type = "arena_wall";
        wallEntity.position = QVector3D((x1 + x2) / 2, wallHeight / 2, (z1 + z2) / 2);
        wallEntity.dimensions = QVector3D(wallLength, wallHeight, 0.2);
        wallEntity.isStatic = true;
        
        m_gameScene->addEntity(wallEntity);
    }
}

void GLArenaWidget::createFloor(double radius)
{
    // Create a circular floor with appropriate number of segments
    const int segments = 32;
    QVector<Vertex> vertices;
    QVector<GLuint> indices;
    
    // Center vertex
    Vertex center;
    center.position = QVector3D(0.0f, -0.05f, 0.0f); // Slightly below 0 to avoid z-fighting
    center.normal = QVector3D(0.0f, 1.0f, 0.0f);
    center.texCoord = QVector2D(0.5f, 0.5f);
    vertices.append(center);
    
    // Outer rim vertices
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = radius * cos(angle);
        float z = radius * sin(angle);
        
        Vertex v;
        v.position = QVector3D(x, -0.05f, z);
        v.normal = QVector3D(0.0f, 1.0f, 0.0f);
        
        // Texture coordinates - map [0, 1] x [0, 1] to floor
        v.texCoord = QVector2D(0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle));
        
        vertices.append(v);
    }
    
    // Create indices for triangle fan
    for (int i = 1; i <= segments; i++) {
        indices.append(0);  // Center
        indices.append(i);
        indices.append(i + 1 > segments ? 1 : i + 1);
    }
    
    // Create OpenGL buffers
    m_floorVAO.create();
    m_floorVAO.bind();
    
    m_floorVBO.create();
    m_floorVBO.bind();
    m_floorVBO.allocate(vertices.constData(), vertices.size() * sizeof(Vertex));
    
    // Enable vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    
    m_floorIBO.create();
    m_floorIBO.bind();
    m_floorIBO.allocate(indices.constData(), indices.size() * sizeof(GLuint));
    
    m_floorIndexCount = indices.size();
    
    m_floorVAO.release();
    
    // Create floor entity in game scene
    GameEntity floorEntity;
    floorEntity.id = "arena_floor";
    floorEntity.type = "arena_floor";
    floorEntity.position = QVector3D(0.0f, -0.05f, 0.0f);
    floorEntity.dimensions = QVector3D(radius * 2, 0.1f, radius * 2);
    floorEntity.isStatic = true;
    
    m_gameScene->addEntity(floorEntity);
}

void GLArenaWidget::createGrid(double size, int divisions)
{
    // Create a grid of lines on the floor for better orientation
    QVector<QVector3D> lineVertices;
    
    float step = size / divisions;
    float halfSize = size / 2.0f;
    
    // Create grid lines along x-axis
    for (int i = 0; i <= divisions; i++) {
        float x = -halfSize + i * step;
        lineVertices.append(QVector3D(x, -0.04f, -halfSize)); // Start point
        lineVertices.append(QVector3D(x, -0.04f, halfSize));  // End point
    }
    
    // Create grid lines along z-axis
    for (int i = 0; i <= divisions; i++) {
        float z = -halfSize + i * step;
        lineVertices.append(QVector3D(-halfSize, -0.04f, z)); // Start point
        lineVertices.append(QVector3D(halfSize, -0.04f, z));  // End point
    }
    
    // Create OpenGL buffers
    m_gridVAO.create();
    m_gridVAO.bind();
    
    m_gridVBO.create();
    m_gridVBO.bind();
    m_gridVBO.allocate(lineVertices.constData(), lineVertices.size() * sizeof(QVector3D));
    
    // Enable vertex attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), nullptr);
    
    m_gridVertexCount = lineVertices.size();
    
    m_gridVAO.release();
}