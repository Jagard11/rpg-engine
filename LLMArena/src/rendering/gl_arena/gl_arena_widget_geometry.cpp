// src/rendering/gl_arena/gl_arena_widget_geometry.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include <QDebug>
#include <cmath>

// Vertex struct for geometry creation
struct Vertex {
    QVector3D position;
    QVector3D normal;
    QVector2D texCoord;
};

void GLArenaWidget::createFloor(double radius)
{
    qDebug() << ">> createFloor: Starting with radius:" << radius;

    // Step 1: Clean up existing buffers if they exist
    if (m_floorVAO.isCreated()) {
        qDebug() << ">> createFloor: Destroying existing floor VAO";
        m_floorVAO.destroy();
    }
    if (m_floorVBO.isCreated()) {
        qDebug() << ">> createFloor: Destroying existing floor VBO";
        m_floorVBO.destroy();
    }
    if (m_floorIBO.isCreated()) {
        qDebug() << ">> createFloor: Destroying existing floor IBO";
        m_floorIBO.destroy();
    }

    // Step 2: Generate circular floor geometry
    const int segments = 32; // Number of segments for the circle
    const float angleStep = 2.0f * M_PI / segments;

    std::vector<float> vertices;         // Position (x, y, z) + Normal (nx, ny, nz)
    std::vector<unsigned int> indices;   // Indices for triangle fan

    // Center vertex
    qDebug() << ">> createFloor: Adding center vertex";
    vertices.push_back(0.0f);  // x
    vertices.push_back(0.0f);  // y
    vertices.push_back(0.0f);  // z
    vertices.push_back(0.0f);  // normal x
    vertices.push_back(1.0f);  // normal y (upward)
    vertices.push_back(0.0f);  // normal z

    // Outer rim vertices
    qDebug() << ">> createFloor: Adding outer rim vertices";
    for (int i = 0; i <= segments; ++i) {
        float angle = i * angleStep;
        float x = radius * cos(angle);
        float z = radius * sin(angle);
        vertices.push_back(x);      // x
        vertices.push_back(0.0f);   // y
        vertices.push_back(z);      // z
        vertices.push_back(0.0f);   // normal x
        vertices.push_back(1.0f);   // normal y
        vertices.push_back(0.0f);   // normal z
    }

    // Indices for triangle fan
    qDebug() << ">> createFloor: Creating indices for triangle fan";
    for (int i = 1; i < segments; ++i) {
        indices.push_back(0);       // Center vertex
        indices.push_back(i);       // Current vertex
        indices.push_back(i + 1);   // Next vertex
    }
    // Close the fan
    indices.push_back(0);
    indices.push_back(segments);
    indices.push_back(1);

    m_floorIndexCount = indices.size();
    qDebug() << ">> createFloor: Index count set to:" << m_floorIndexCount;

    // Step 3: Set up OpenGL buffers
    // Create and bind VAO
    qDebug() << ">> createFloor: Creating VAO";
    m_floorVAO.create();
    m_floorVAO.bind();

    // Create and populate VBO
    qDebug() << ">> createFloor: Creating VBO";
    m_floorVBO.create();
    m_floorVBO.bind();
    m_floorVBO.allocate(vertices.data(), vertices.size() * sizeof(float));

    // Configure vertex attributes
    qDebug() << ">> createFloor: Setting up vertex attributes";
    glEnableVertexAttribArray(0); // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    // Create and populate IBO
    qDebug() << ">> createFloor: Creating IBO";
    m_floorIBO.create();
    m_floorIBO.bind();
    m_floorIBO.allocate(indices.data(), indices.size() * sizeof(unsigned int));

    // Step 4: Release bindings
    qDebug() << ">> createFloor: Releasing bindings";
    m_floorVAO.release();
    m_floorVBO.release();
    m_floorIBO.release();

    // Step 5: Update game scene
    qDebug() << ">> createFloor: Updating game scene";
    
    // First, remove old floor entity if exists
    m_gameScene->removeEntity("arena_floor");
    
    // Create a proper GameEntity for the floor
    GameEntity floorEntity;
    floorEntity.id = "arena_floor";
    floorEntity.type = "arena_floor";
    floorEntity.position = QVector3D(0.0f, -0.05f, 0.0f);
    floorEntity.dimensions = QVector3D(radius * 2, 0.1, radius * 2);
    floorEntity.isStatic = true;
    
    // Add entity to game scene
    m_gameScene->addEntity(floorEntity);

    qDebug() << ">> createFloor: Floor creation completed";
}

void GLArenaWidget::createArena(double radius, double wallHeight)
{
    // Store parameters
    m_arenaRadius = radius;
    m_wallHeight = wallHeight;
    
    // Clear old walls
    m_walls.clear();
    
    try {
        qDebug() << ">> createArena: Starting with radius:" << radius << "height:" << wallHeight;
        
        // Create octagonal arena walls
        const int numWalls = 8;
        m_walls.resize(numWalls);
        
        for (int i = 0; i < numWalls; i++) {
            double angle1 = 2.0 * M_PI * i / numWalls;
            double angle2 = 2.0 * M_PI * (i + 1) / numWalls;
            
            double x1 = radius * cos(angle1);
            double z1 = radius * sin(angle1);
            double x2 = radius * cos(angle2);
            double z2 = radius * sin(angle2);
            
            // Compute wall normal (pointing inward)
            QVector3D wallDir(x2 - x1, 0.0, z2 - z1);
            QVector3D normal = QVector3D::crossProduct(wallDir, QVector3D(0.0, 1.0, 0.0)).normalized();
            normal = -normal; // Point inward
            
            // Calculate wall length
            float wallLength = QVector2D(x2 - x1, z2 - z1).length();
            
            qDebug() << ">> createArena: Creating wall" << i << "from" << x1 << z1 << "to" << x2 << z2;
            
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
            
            qDebug() << ">> createArena: Setting up wall" << i << "GL objects";
            
            // Create new WallGeometry
            WallGeometry& wall = m_walls[i];
            
            // Initialize OpenGL objects if they don't exist
            if (!wall.vao) wall.vao.reset(new QOpenGLVertexArrayObject);
            if (!wall.vbo) wall.vbo.reset(new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer));
            if (!wall.ibo) wall.ibo.reset(new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer));
            
            // Create and bind VAO first
            if (!wall.vao->create()) {
                qWarning() << ">> createArena: Failed to create VAO for wall" << i;
                continue;
            }
            wall.vao->bind();
            
            // Create and bind VBO
            if (!wall.vbo->create()) {
                qWarning() << ">> createArena: Failed to create VBO for wall" << i;
                wall.vao->release();
                continue;
            }
            wall.vbo->bind();
            wall.vbo->allocate(vertices, 4 * sizeof(Vertex));
            
            // Enable vertex attributes
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                                reinterpret_cast<void*>(offsetof(Vertex, position)));
            
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                                reinterpret_cast<void*>(offsetof(Vertex, normal)));
            
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                                reinterpret_cast<void*>(offsetof(Vertex, texCoord)));
            
            // Create and bind IBO
            if (!wall.ibo->create()) {
                qWarning() << ">> createArena: Failed to create IBO for wall" << i;
                wall.vbo->release();
                wall.vao->release();
                continue;
            }
            wall.ibo->bind();
            wall.ibo->allocate(indices, 6 * sizeof(GLuint));
            
            wall.indexCount = 6;
            
            // Release bindings
            wall.ibo->release();
            wall.vbo->release();
            wall.vao->release();
            
            qDebug() << ">> createArena: Adding wall" << i << "entity to game scene";
            
            // Create wall entity in game scene
            GameEntity wallEntity;
            wallEntity.id = QString("arena_wall_%1").arg(i);
            wallEntity.type = "arena_wall";
            wallEntity.position = QVector3D((x1 + x2) / 2, wallHeight / 2, (z1 + z2) / 2);
            wallEntity.dimensions = QVector3D(wallLength, wallHeight, 0.2);
            wallEntity.isStatic = true;
            
            m_gameScene->addEntity(wallEntity);
        }
        
        qDebug() << ">> createArena: Creating floor";
        createFloor(radius);
        
        qDebug() << ">> createArena: Creating grid";
        createGrid(radius * 2, 20);
        
        qDebug() << ">> createArena: Arena creation completed successfully";
    } catch (const std::exception& e) {
        qCritical() << ">> createArena: Exception in createArena:" << e.what();
    }
}

void GLArenaWidget::createGrid(double size, int divisions)
{
    try {
        qDebug() << ">> createGrid: Starting with size:" << size << "divisions:" << divisions;
        
        // Clean up existing grid if needed
        if (m_gridVAO.isCreated()) {
            qDebug() << ">> createGrid: Destroying existing grid VAO";
            m_gridVAO.destroy();
        }
        if (m_gridVBO.isCreated()) {
            qDebug() << ">> createGrid: Destroying existing grid VBO";
            m_gridVBO.destroy();
        }
        
        // Create a grid of lines for orientation
        QVector<QVector3D> lineVertices;
        
        float step = size / divisions;
        float halfSize = size / 2.0f;
        
        qDebug() << ">> createGrid: Creating X-axis lines";
        // Create grid lines along x-axis
        for (int i = 0; i <= divisions; i++) {
            float x = -halfSize + i * step;
            lineVertices.append(QVector3D(x, -0.04f, -halfSize)); // Start point
            lineVertices.append(QVector3D(x, -0.04f, halfSize));  // End point
        }
        
        qDebug() << ">> createGrid: Creating Z-axis lines";
        // Create grid lines along z-axis
        for (int i = 0; i <= divisions; i++) {
            float z = -halfSize + i * step;
            lineVertices.append(QVector3D(-halfSize, -0.04f, z)); // Start point
            lineVertices.append(QVector3D(halfSize, -0.04f, z));  // End point
        }
        
        qDebug() << ">> createGrid: Total line vertices:" << lineVertices.size();
        
        qDebug() << ">> createGrid: Creating VAO";
        // Create VAO
        if (!m_gridVAO.create()) {
            qWarning() << ">> createGrid: Failed to create grid VAO";
            return;
        }
        m_gridVAO.bind();
        
        qDebug() << ">> createGrid: Creating VBO";
        // Create VBO
        if (!m_gridVBO.create()) {
            qWarning() << ">> createGrid: Failed to create grid VBO";
            m_gridVAO.release();
            return;
        }
        m_gridVBO.bind();
        m_gridVBO.allocate(lineVertices.constData(), lineVertices.size() * sizeof(QVector3D));
        
        qDebug() << ">> createGrid: Setting up vertex attributes";
        // Set vertex attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), nullptr);
        
        m_gridVertexCount = lineVertices.size();
        
        qDebug() << ">> createGrid: Releasing bindings";
        // Release bindings
        m_gridVBO.release();
        m_gridVAO.release();
        
        qDebug() << ">> createGrid: Grid creation completed successfully";
    } catch (const std::exception& e) {
        qCritical() << ">> createGrid: Exception in createGrid:" << e.what();
    }
}