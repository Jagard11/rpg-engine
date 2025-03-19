// src/rendering/gl_arena/gl_arena_widget_geometry.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include <QDebug>
#include <QOpenGLContext>
#include <stdexcept>

void GLArenaWidget::createFloor(double radius) {
    try {
        qDebug() << "Creating floor geometry...";
        
        // Clean up existing buffers if they exist
        if (m_floorVAO.isCreated()) {
            m_floorVAO.destroy();
        }
        
        if (m_floorVBO.isCreated()) {
            m_floorVBO.destroy();
        }
        
        if (m_floorIBO.isCreated()) {
            m_floorIBO.destroy();
        }
        
        // Verify OpenGL context before proceeding
        if (!QOpenGLContext::currentContext() || !QOpenGLContext::currentContext()->isValid()) {
            qCritical() << "No valid OpenGL context during floor creation";
            return;
        }
        
        // Create new buffers
        if (!m_floorVAO.create()) {
            qCritical() << "Failed to create floor VAO";
            return;
        }
        
        if (!m_floorVBO.create()) {
            qCritical() << "Failed to create floor VBO";
            m_floorVAO.destroy();
            return;
        }
        
        if (!m_floorIBO.create()) {
            qCritical() << "Failed to create floor IBO";
            m_floorVBO.destroy();
            m_floorVAO.destroy();
            return;
        }
        
        // Generate floor vertices (simple, flat square)
        QVector<float> vertices;
        QVector<unsigned int> indices;
        
        // Floor vertices (x, y, z, nx, ny, nz, s, t)
        // Bottom (flat at y=0)
        float halfSize = static_cast<float>(radius);
        
        vertices << -halfSize << 0.0f << -halfSize << 0.0f << 1.0f << 0.0f << 0.0f << 0.0f;
        vertices << halfSize << 0.0f << -halfSize << 0.0f << 1.0f << 0.0f << 1.0f << 0.0f;
        vertices << halfSize << 0.0f << halfSize << 0.0f << 1.0f << 0.0f << 1.0f << 1.0f;
        vertices << -halfSize << 0.0f << halfSize << 0.0f << 1.0f << 0.0f << 0.0f << 1.0f;
        
        // Indices (two triangles for the floor)
        indices << 0 << 1 << 2;  // Triangle 1
        indices << 0 << 2 << 3;  // Triangle 2
        
        // Store index count for rendering
        m_floorIndexCount = indices.size();
        
        if (m_floorIndexCount == 0) {
            qCritical() << "Floor creation failed: no indices generated";
            return;
        }
        
        // Bind VAO and set up buffers
        m_floorVAO.bind();
        
        // Load vertex buffer
        m_floorVBO.bind();
        m_floorVBO.allocate(vertices.constData(), vertices.size() * sizeof(float));
        
        // Set up vertex attributes
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);  // Position
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 
                            reinterpret_cast<void*>(3 * sizeof(float)));  // Normal
        glEnableVertexAttribArray(1);
        
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 
                            reinterpret_cast<void*>(6 * sizeof(float)));  // TexCoord
        glEnableVertexAttribArray(2);
        
        // Load index buffer
        m_floorIBO.bind();
        m_floorIBO.allocate(indices.constData(), indices.size() * sizeof(unsigned int));
        
        // Unbind VAO to prevent accidental modification
        m_floorVAO.release();
        m_floorVBO.release();
        m_floorIBO.release();
        
        qDebug() << "Floor geometry created successfully with" << m_floorIndexCount << "indices";
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in createFloor:" << e.what();
        
        // Clean up on error
        if (m_floorVAO.isCreated()) m_floorVAO.destroy();
        if (m_floorVBO.isCreated()) m_floorVBO.destroy();
        if (m_floorIBO.isCreated()) m_floorIBO.destroy();
        m_floorIndexCount = 0;
    }
    catch (...) {
        qCritical() << "Unknown exception in createFloor";
        
        // Clean up on error
        if (m_floorVAO.isCreated()) m_floorVAO.destroy();
        if (m_floorVBO.isCreated()) m_floorVBO.destroy();
        if (m_floorIBO.isCreated()) m_floorIBO.destroy();
        m_floorIndexCount = 0;
    }
}

void GLArenaWidget::createArena(double radius, double wallHeight) {
    try {
        qDebug() << "Creating arena geometry...";
        m_arenaRadius = radius;
        m_wallHeight = wallHeight;
        
        // Create floor
        createFloor(radius);
        
        // Create grid
        createGrid(radius * 2, 20);
        
        // Create walls - with safety check for OpenGL context
        if (!QOpenGLContext::currentContext() || !QOpenGLContext::currentContext()->isValid()) {
            qCritical() << "No valid OpenGL context during arena creation";
            return;
        }
        
        // Clear existing walls
        m_walls.clear();
        
        // Create rectangular walls
        const float halfWidth = static_cast<float>(radius);
        const float height = static_cast<float>(wallHeight);
        
        // Temporary vectors to hold wall positions
        QVector<QVector3D> wallPositions;
        QVector<QVector3D> wallSizes;
        
        // North wall (positive Z)
        wallPositions.append(QVector3D(0, height/2, halfWidth));
        wallSizes.append(QVector3D(2*halfWidth, height, 0.2f)); // thin wall
        
        // South wall (negative Z)
        wallPositions.append(QVector3D(0, height/2, -halfWidth));
        wallSizes.append(QVector3D(2*halfWidth, height, 0.2f));
        
        // East wall (positive X)
        wallPositions.append(QVector3D(halfWidth, height/2, 0));
        wallSizes.append(QVector3D(0.2f, height, 2*halfWidth));
        
        // West wall (negative X)
        wallPositions.append(QVector3D(-halfWidth, height/2, 0));
        wallSizes.append(QVector3D(0.2f, height, 2*halfWidth));
        
        // Create the wall geometries - inline implementation instead of separate function
        for (int i = 0; i < wallPositions.size(); i++) {
            const QVector3D& position = wallPositions[i];
            const QVector3D& size = wallSizes[i];
            
            try {
                // Verify OpenGL context first
                if (!QOpenGLContext::currentContext() || !QOpenGLContext::currentContext()->isValid()) {
                    qCritical() << "No valid OpenGL context during wall creation";
                    continue;
                }
                
                // Create a new wall geometry
                WallGeometry wall;
                
                // Create and check buffers
                wall.vao = std::make_unique<QOpenGLVertexArrayObject>();
                if (!wall.vao->create()) {
                    qCritical() << "Failed to create wall VAO";
                    continue;
                }
                
                wall.vbo = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
                if (!wall.vbo->create()) {
                    qCritical() << "Failed to create wall VBO";
                    wall.vao->destroy();
                    continue;
                }
                
                wall.ibo = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer);
                if (!wall.ibo->create()) {
                    qCritical() << "Failed to create wall IBO";
                    wall.vbo->destroy();
                    wall.vao->destroy();
                    continue;
                }
                
                // Generate wall vertices and indices
                QVector<float> vertices;
                QVector<unsigned int> indices;
                
                // Wall dimensions
                float width = size.x();
                float height = size.y();
                float depth = size.z();
                
                // Wall half-dimensions
                float halfWidth = width / 2.0f;
                float halfHeight = height / 2.0f;
                float halfDepth = depth / 2.0f;
                
                // Vertex format: x, y, z, nx, ny, nz, s, t
                
                // For very thin walls (like those along X or Z axis),
                // we'll just create a single quad instead of a full box
                
                // Check if the wall is very thin along X axis
                if (width < 0.3f) {
                    // It's a vertical wall along X axis (East-West)
                    // Create a flat quad oriented along Z axis
                    
                    // Front face
                    vertices << 0.0f << -halfHeight << -halfDepth << 1.0f << 0.0f << 0.0f << 0.0f << 0.0f;
                    vertices << 0.0f << halfHeight << -halfDepth << 1.0f << 0.0f << 0.0f << 1.0f << 0.0f;
                    vertices << 0.0f << halfHeight << halfDepth << 1.0f << 0.0f << 0.0f << 1.0f << 1.0f;
                    vertices << 0.0f << -halfHeight << halfDepth << 1.0f << 0.0f << 0.0f << 0.0f << 1.0f;
                    
                    // Indices for the quad
                    indices << 0 << 1 << 2;
                    indices << 0 << 2 << 3;
                }
                // Check if the wall is very thin along Z axis
                else if (depth < 0.3f) {
                    // It's a vertical wall along Z axis (North-South)
                    // Create a flat quad oriented along X axis
                    
                    // Front face
                    vertices << -halfWidth << -halfHeight << 0.0f << 0.0f << 0.0f << 1.0f << 0.0f << 0.0f;
                    vertices << halfWidth << -halfHeight << 0.0f << 0.0f << 0.0f << 1.0f << 1.0f << 0.0f;
                    vertices << halfWidth << halfHeight << 0.0f << 0.0f << 0.0f << 1.0f << 1.0f << 1.0f;
                    vertices << -halfWidth << halfHeight << 0.0f << 0.0f << 0.0f << 1.0f << 0.0f << 1.0f;
                    
                    // Indices for the quad
                    indices << 0 << 1 << 2;
                    indices << 0 << 2 << 3;
                }
                else {
                    // It's a full wall box, create all six faces
                    // (Front, back, left, right, top, bottom)
                    
                    // Front face
                    vertices << -halfWidth << -halfHeight << halfDepth << 0.0f << 0.0f << 1.0f << 0.0f << 0.0f;
                    vertices << halfWidth << -halfHeight << halfDepth << 0.0f << 0.0f << 1.0f << 1.0f << 0.0f;
                    vertices << halfWidth << halfHeight << halfDepth << 0.0f << 0.0f << 1.0f << 1.0f << 1.0f;
                    vertices << -halfWidth << halfHeight << halfDepth << 0.0f << 0.0f << 1.0f << 0.0f << 1.0f;
                    
                    // Back face
                    vertices << halfWidth << -halfHeight << -halfDepth << 0.0f << 0.0f << -1.0f << 0.0f << 0.0f;
                    vertices << -halfWidth << -halfHeight << -halfDepth << 0.0f << 0.0f << -1.0f << 1.0f << 0.0f;
                    vertices << -halfWidth << halfHeight << -halfDepth << 0.0f << 0.0f << -1.0f << 1.0f << 1.0f;
                    vertices << halfWidth << halfHeight << -halfDepth << 0.0f << 0.0f << -1.0f << 0.0f << 1.0f;
                    
                    // Left face
                    vertices << -halfWidth << -halfHeight << -halfDepth << -1.0f << 0.0f << 0.0f << 0.0f << 0.0f;
                    vertices << -halfWidth << -halfHeight << halfDepth << -1.0f << 0.0f << 0.0f << 1.0f << 0.0f;
                    vertices << -halfWidth << halfHeight << halfDepth << -1.0f << 0.0f << 0.0f << 1.0f << 1.0f;
                    vertices << -halfWidth << halfHeight << -halfDepth << -1.0f << 0.0f << 0.0f << 0.0f << 1.0f;
                    
                    // Right face
                    vertices << halfWidth << -halfHeight << halfDepth << 1.0f << 0.0f << 0.0f << 0.0f << 0.0f;
                    vertices << halfWidth << -halfHeight << -halfDepth << 1.0f << 0.0f << 0.0f << 1.0f << 0.0f;
                    vertices << halfWidth << halfHeight << -halfDepth << 1.0f << 0.0f << 0.0f << 1.0f << 1.0f;
                    vertices << halfWidth << halfHeight << halfDepth << 1.0f << 0.0f << 0.0f << 0.0f << 1.0f;
                    
                    // Top face
                    vertices << -halfWidth << halfHeight << halfDepth << 0.0f << 1.0f << 0.0f << 0.0f << 0.0f;
                    vertices << halfWidth << halfHeight << halfDepth << 0.0f << 1.0f << 0.0f << 1.0f << 0.0f;
                    vertices << halfWidth << halfHeight << -halfDepth << 0.0f << 1.0f << 0.0f << 1.0f << 1.0f;
                    vertices << -halfWidth << halfHeight << -halfDepth << 0.0f << 1.0f << 0.0f << 0.0f << 1.0f;
                    
                    // Bottom face
                    vertices << -halfWidth << -halfHeight << -halfDepth << 0.0f << -1.0f << 0.0f << 0.0f << 0.0f;
                    vertices << halfWidth << -halfHeight << -halfDepth << 0.0f << -1.0f << 0.0f << 1.0f << 0.0f;
                    vertices << halfWidth << -halfHeight << halfDepth << 0.0f << -1.0f << 0.0f << 1.0f << 1.0f;
                    vertices << -halfWidth << -halfHeight << halfDepth << 0.0f << -1.0f << 0.0f << 0.0f << 1.0f;
                    
                    // Indices for all six faces
                    for (int face = 0; face < 6; face++) {
                        int baseVertex = face * 4;
                        indices << baseVertex << baseVertex + 1 << baseVertex + 2;
                        indices << baseVertex << baseVertex + 2 << baseVertex + 3;
                    }
                }
                
                // Set the index count
                wall.indexCount = indices.size();
                
                if (wall.indexCount == 0) {
                    qCritical() << "Wall creation failed: no indices generated";
                    continue;
                }
                
                // Now bind and initialize buffers
                wall.vao->bind();
                
                // Load vertex data
                wall.vbo->bind();
                wall.vbo->allocate(vertices.constData(), vertices.size() * sizeof(float));
                
                // Set up vertex attributes (position, normal, texcoord)
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);  // Position
                glEnableVertexAttribArray(0);
                
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 
                                    reinterpret_cast<void*>(3 * sizeof(float)));  // Normal
                glEnableVertexAttribArray(1);
                
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 
                                    reinterpret_cast<void*>(6 * sizeof(float)));  // TexCoord
                glEnableVertexAttribArray(2);
                
                // Load index data
                wall.ibo->bind();
                wall.ibo->allocate(indices.constData(), indices.size() * sizeof(unsigned int));
                
                // Unbind to prevent accidental modification
                wall.ibo->release();
                wall.vbo->release();
                wall.vao->release();
                
                qDebug() << "Wall geometry created with" << wall.indexCount << "indices";
                
                // Add to walls list
                m_walls.push_back(std::move(wall));
            }
            catch (const std::exception& e) {
                qCritical() << "Exception in wall creation:" << e.what();
            }
            catch (...) {
                qCritical() << "Unknown exception in wall creation";
            }
        }
        
        // Update the game scene with wall entities
        if (m_gameScene) {
            // First remove any existing walls
            QVector<GameEntity> entities = m_gameScene->getAllEntities();
            for (const GameEntity& entity : entities) {
                if (entity.type == "wall") {
                    m_gameScene->removeEntity(entity.id);
                }
            }
            
            // Add new wall entities for collision
            for (int i = 0; i < wallPositions.size(); i++) {
                GameEntity entity;
                entity.id = QString("wall_%1").arg(i);
                entity.type = "wall";
                entity.position = wallPositions[i];
                entity.dimensions = wallSizes[i];
                entity.isStatic = true;
                
                m_gameScene->addEntity(entity);
            }
        }
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in createArena:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown exception in createArena";
    }
}

void GLArenaWidget::createGrid(double size, int divisions) {
    try {
        qDebug() << "Creating grid geometry...";
        
        // Clean up existing grid buffers if they exist
        if (m_gridVAO.isCreated()) {
            m_gridVAO.destroy();
        }
        
        if (m_gridVBO.isCreated()) {
            m_gridVBO.destroy();
        }
        
        // Verify OpenGL context before proceeding
        if (!QOpenGLContext::currentContext() || !QOpenGLContext::currentContext()->isValid()) {
            qCritical() << "No valid OpenGL context during grid creation";
            return;
        }
        
        // Create new buffers
        if (!m_gridVAO.create()) {
            qCritical() << "Failed to create grid VAO";
            return;
        }
        
        if (!m_gridVBO.create()) {
            qCritical() << "Failed to create grid VBO";
            m_gridVAO.destroy();
            return;
        }
        
        // Generate grid lines
        QVector<float> vertices;
        
        // Grid parameters
        float halfSize = static_cast<float>(size) / 2.0f;
        float step = static_cast<float>(size) / static_cast<float>(divisions);
        
        // Create a single line at y=0
        // X-axis grid lines (vary along Z)
        for (int i = 0; i <= divisions; i++) {
            float z = -halfSize + i * step;
            
            vertices << -halfSize << 0.01f << z;  // Slightly above floor
            vertices << halfSize << 0.01f << z;
        }
        
        // Z-axis grid lines (vary along X)
        for (int i = 0; i <= divisions; i++) {
            float x = -halfSize + i * step;
            
            vertices << x << 0.01f << -halfSize;  // Slightly above floor
            vertices << x << 0.01f << halfSize;
        }
        
        // Store vertex count for rendering
        m_gridVertexCount = vertices.size() / 3;  // 3 components per vertex
        
        if (m_gridVertexCount == 0) {
            qCritical() << "Grid creation failed: no vertices generated";
            return;
        }
        
        // Bind VAO and set up buffer
        m_gridVAO.bind();
        
        // Load vertex buffer
        m_gridVBO.bind();
        m_gridVBO.allocate(vertices.constData(), vertices.size() * sizeof(float));
        
        // Set up vertex attributes
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);  // Position
        glEnableVertexAttribArray(0);
        
        // Unbind VAO to prevent accidental modification
        m_gridVAO.release();
        m_gridVBO.release();
        
        qDebug() << "Grid geometry created successfully with" << m_gridVertexCount << "vertices";
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in createGrid:" << e.what();
        
        // Clean up on error
        if (m_gridVAO.isCreated()) m_gridVAO.destroy();
        if (m_gridVBO.isCreated()) m_gridVBO.destroy();
        m_gridVertexCount = 0;
    }
    catch (...) {
        qCritical() << "Unknown exception in createGrid";
        
        // Clean up on error
        if (m_gridVAO.isCreated()) m_gridVAO.destroy();
        if (m_gridVBO.isCreated()) m_gridVBO.destroy();
        m_gridVertexCount = 0;
    }
}