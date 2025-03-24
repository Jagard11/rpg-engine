// src/arena/ui/gl_widgets/gl_arena_widget_renderer.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include <QDebug>
#include <cmath>

// This file contains rendering methods for GLArenaWidget
// These are separated for better modularity and code organization

void GLArenaWidget::renderFloor() {
    // Skip if not initialized
    if (!m_initialized || !m_floorVAO.isCreated() || !m_floorVBO.isCreated() || !m_floorIBO.isCreated()) {
        qWarning() << "Floor not initialized properly, skipping renderFloor";
        return;
    }

    // Set the shader program
    if (!m_billboardProgram || !m_billboardProgram->isLinked()) {
        qWarning() << "Shader program not initialized for floor rendering";
        return;
    }

    // Bind the shader program
    if (!m_billboardProgram->bind()) {
        qWarning() << "Failed to bind shader program for floor rendering";
        return;
    }

    // Set up matrices
    QMatrix4x4 modelMatrix;
    modelMatrix.setToIdentity();
    
    // Set shader uniforms
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    m_billboardProgram->setUniformValue("model", modelMatrix);
    m_billboardProgram->setUniformValue("color", QVector4D(0.3f, 0.3f, 0.3f, 1.0f));
    
    // Bind the VAO that contains both VBO and IBO bindings
    m_floorVAO.bind();
    
    // Draw the floor
    glDrawElements(GL_TRIANGLES, m_floorIndexCount, GL_UNSIGNED_INT, nullptr);
    
    // Release bindings
    m_floorVAO.release();
    m_billboardProgram->release();
}

void GLArenaWidget::renderGrid() {
    // Skip if not initialized
    if (!m_initialized || !m_gridVAO.isCreated() || !m_gridVBO.isCreated()) {
        return;
    }

    // Set the shader program
    if (!m_billboardProgram || !m_billboardProgram->isLinked()) {
        return;
    }

    if (!m_billboardProgram->bind()) {
        return;
    }

    // Set up matrices
    QMatrix4x4 modelMatrix;
    modelMatrix.setToIdentity();
    
    // Set shader uniforms
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    m_billboardProgram->setUniformValue("model", modelMatrix);
    m_billboardProgram->setUniformValue("color", QVector4D(0.5f, 0.5f, 0.5f, 0.5f));
    
    // Bind VAO
    m_gridVAO.bind();
    
    // Draw grid lines
    glDrawArrays(GL_LINES, 0, m_gridVertexCount);
    
    // Release bindings
    m_gridVAO.release();
    m_billboardProgram->release();
}

void GLArenaWidget::renderWalls() {
    // Skip if not initialized
    if (!m_initialized || m_walls.empty()) {
        return;
    }

    // Set the shader program
    if (!m_billboardProgram || !m_billboardProgram->isLinked()) {
        return;
    }

    if (!m_billboardProgram->bind()) {
        return;
    }

    // Set up matrices
    QMatrix4x4 modelMatrix;
    modelMatrix.setToIdentity();
    
    // Set shader uniforms
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    m_billboardProgram->setUniformValue("model", modelMatrix);
    m_billboardProgram->setUniformValue("color", QVector4D(0.7f, 0.7f, 0.7f, 1.0f));
    
    // Draw each wall
    for (const auto& wall : m_walls) {
        if (wall.vao && wall.vao->isCreated() && wall.ibo && wall.ibo->isCreated()) {
            wall.vao->bind();
            glDrawElements(GL_TRIANGLES, wall.indexCount, GL_UNSIGNED_INT, nullptr);
            wall.vao->release();
        }
    }
    
    // Release shader
    m_billboardProgram->release();
}

void GLArenaWidget::renderVoxelHighlight() {
    // Skip if not initialized or no voxel highlighted
    if (!m_initialized || m_highlightedVoxelFace < 0) {
        return;
    }

    // FUTURE IMPLEMENTATION: Render a highlight around the voxel that's being pointed at
}

void GLArenaWidget::drawCharacterQuad(QOpenGLTexture* texture, float x, float y, float z, float width, float height) {
    if (!texture || !m_billboardProgram || !m_billboardProgram->isLinked()) {
        return;
    }

    // Set up model matrix
    QMatrix4x4 modelMatrix;
    modelMatrix.setToIdentity();
    modelMatrix.translate(x, y, z);
    
    // Calculate billboard orientation based on camera
    QVector3D right = QVector3D::crossProduct(QVector3D(0, 1, 0), 
                                             QVector3D(m_viewMatrix(0, 2), m_viewMatrix(1, 2), m_viewMatrix(2, 2)));
    QVector3D up(0, 1, 0);
    
    // Set shader uniforms
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    m_billboardProgram->setUniformValue("cameraRight", right);
    m_billboardProgram->setUniformValue("cameraUp", up);
    m_billboardProgram->setUniformValue("billboardPos", QVector3D(x, y, z));
    m_billboardProgram->setUniformValue("billboardSize", QVector2D(width, height));
    
    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    texture->bind();
    m_billboardProgram->setUniformValue("textureSampler", 0);
    
    // Draw character quad
    // Note: This is a fallback method for direct rendering without VAOs
    // We create a temporary direct rendering of the billboard
    
    // Define quad vertices
    const float quadVertices[] = {
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
         0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 0.0f, 1.0f
    };
    
    // Create temporary VBO
    QOpenGLBuffer tempVBO(QOpenGLBuffer::VertexBuffer);
    tempVBO.create();
    tempVBO.bind();
    tempVBO.allocate(quadVertices, sizeof(quadVertices));
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    
    glEnableVertexAttribArray(1); // texCoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    
    // Draw the quad
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    // Clean up
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    tempVBO.release();
    tempVBO.destroy();
    
    // Unbind texture
    texture->release();
}

void GLArenaWidget::placeVoxel() {
    if (m_highlightedVoxelFace < 0 || !m_voxelSystem || !m_inventory) {
        return;
    }
    
    // Get the selected voxel type from inventory
    if (m_inventoryUI && m_inventoryUI->hasVoxelTypeSelected()) {
        VoxelType type = m_inventoryUI->getSelectedVoxelType();
        
        // Create voxel based on the selected type
        Voxel voxel(type, QColor(255, 255, 255)); // Default to white, texture will be applied
        
        // Calculate position offset based on the face normal
        QVector3D normal;
        switch (m_highlightedVoxelFace) {
            case 0: normal = QVector3D(1, 0, 0); break;  // +X
            case 1: normal = QVector3D(-1, 0, 0); break; // -X
            case 2: normal = QVector3D(0, 1, 0); break;  // +Y
            case 3: normal = QVector3D(0, -1, 0); break; // -Y
            case 4: normal = QVector3D(0, 0, 1); break;  // +Z
            case 5: normal = QVector3D(0, 0, -1); break; // -Z
            default: return; // Invalid face
        }
        
        // Place the voxel using the voxel system
        m_voxelSystem->placeVoxel(m_highlightedVoxelPos, normal, voxel);
        
        // Reset highlight
        m_highlightedVoxelFace = -1;
    }
}

void GLArenaWidget::removeVoxel() {
    if (m_highlightedVoxelFace < 0 || !m_voxelSystem) {
        return;
    }
    
    // Remove the voxel using the voxel system
    m_voxelSystem->removeVoxel(m_highlightedVoxelPos);
    
    // Reset highlight
    m_highlightedVoxelFace = -1;
}

QVector3D GLArenaWidget::worldToNDC(const QVector3D& worldPos) {
    // Convert world coordinates to normalized device coordinates
    // This is useful for UI positioning based on world positions
    
    // Create combined view-projection matrix
    QMatrix4x4 viewProj = m_projectionMatrix * m_viewMatrix;
    
    // Transform world position to clip space
    QVector4D clipPos = viewProj * QVector4D(worldPos, 1.0f);
    
    // Perform perspective division to get NDC
    if (qAbs(clipPos.w()) > 0.00001f) {
        return QVector3D(clipPos.x() / clipPos.w(), 
                        clipPos.y() / clipPos.w(),
                        clipPos.z() / clipPos.w());
    } else {
        // Handle division by zero (rare case)
        return QVector3D(0.0f, 0.0f, 0.0f);
    }
}

void GLArenaWidget::createFloor(double radius) {
    // Clean up existing floor geometry
    if (m_floorVAO.isCreated()) {
        m_floorVAO.destroy();
    }
    
    if (m_floorVBO.isCreated()) {
        m_floorVBO.destroy();
    }
    
    if (m_floorIBO.isCreated()) {
        m_floorIBO.destroy();
    }
    
    // Create VAO for floor
    m_floorVAO.create();
    m_floorVAO.bind();
    
    // Create VBO for floor
    m_floorVBO.create();
    m_floorVBO.bind();
    
    // Create floor geometry
    const float size = radius * 2.0f;
    const float y = 0.0f;
    
    // Floor vertices: position (3) + normal (3) + texcoord (2)
    const float floorVertices[] = {
        // Position          // Normal        // TexCoord
        -size, y, -size,     0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         size, y, -size,     0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         size, y,  size,     0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        -size, y,  size,     0.0f, 1.0f, 0.0f,  0.0f, 1.0f
    };
    
    // Upload vertices
    m_floorVBO.allocate(floorVertices, sizeof(floorVertices));
    
    // Create IBO for floor
    m_floorIBO.create();
    m_floorIBO.bind();
    
    // Floor indices (2 triangles)
    const GLuint floorIndices[] = {
        0, 1, 2,  // Triangle 1
        0, 2, 3   // Triangle 2
    };
    
    // Upload indices
    m_floorIBO.allocate(floorIndices, sizeof(floorIndices));
    
    // Store index count for rendering
    m_floorIndexCount = 6;
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    
    glEnableVertexAttribArray(1); // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 
                         reinterpret_cast<void*>(3 * sizeof(float)));
    
    glEnableVertexAttribArray(2); // texcoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 
                         reinterpret_cast<void*>(6 * sizeof(float)));
    
    // Release bindings
    m_floorIBO.release();
    m_floorVBO.release();
    m_floorVAO.release();
}

void GLArenaWidget::createGrid(double size, int divisions) {
    // Clean up existing grid geometry
    if (m_gridVAO.isCreated()) {
        m_gridVAO.destroy();
    }
    
    if (m_gridVBO.isCreated()) {
        m_gridVBO.destroy();
    }
    
    // Create VAO for grid
    m_gridVAO.create();
    m_gridVAO.bind();
    
    // Create VBO for grid
    m_gridVBO.create();
    m_gridVBO.bind();
    
    // Create grid geometry
    QVector<float> gridVertices;
    
    // Compute step size
    float step = size / divisions;
    float y = 0.01f; // Slightly above floor to avoid z-fighting
    
    // Create grid lines along X-axis
    for (int i = 0; i <= divisions; ++i) {
        float x = -size / 2.0f + i * step;
        
        // Line from (x, y, -size/2) to (x, y, size/2)
        gridVertices.append(x);
        gridVertices.append(y);
        gridVertices.append(-size / 2.0f);
        
        gridVertices.append(x);
        gridVertices.append(y);
        gridVertices.append(size / 2.0f);
    }
    
    // Create grid lines along Z-axis
    for (int i = 0; i <= divisions; ++i) {
        float z = -size / 2.0f + i * step;
        
        // Line from (-size/2, y, z) to (size/2, y, z)
        gridVertices.append(-size / 2.0f);
        gridVertices.append(y);
        gridVertices.append(z);
        
        gridVertices.append(size / 2.0f);
        gridVertices.append(y);
        gridVertices.append(z);
    }
    
    // Upload vertices
    m_gridVBO.allocate(gridVertices.constData(), gridVertices.size() * sizeof(float));
    
    // Store vertex count for rendering
    m_gridVertexCount = gridVertices.size() / 3;
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    
    // Release bindings
    m_gridVBO.release();
    m_gridVAO.release();
}

void GLArenaWidget::createArena(double radius, double wallHeight) {
    // Clean up existing walls
    for (auto& wall : m_walls) {
        if (wall.vao && wall.vao->isCreated()) {
            wall.vao->destroy();
        }
        
        if (wall.vbo && wall.vbo->isCreated()) {
            wall.vbo->destroy();
        }
        
        if (wall.ibo && wall.ibo->isCreated()) {
            wall.ibo->destroy();
        }
    }
    
    m_walls.clear();
    
    // Create floor
    createFloor(radius);
    
    // Create grid
    createGrid(radius * 2.0, 20);
    
    // Create walls for a rectangular arena
    const float size = radius;
    const float height = wallHeight;
    
    // Wall positions and dimensions (center position, width, depth)
    struct WallSpec {
        QVector3D position;
        float width;
        float depth;
    };
    
    // Define wall specifications
    WallSpec wallSpecs[] = {
        { QVector3D(0, height / 2, -size), size * 2, 0.1f },  // North wall
        { QVector3D(0, height / 2, size),  size * 2, 0.1f },  // South wall
        { QVector3D(-size, height / 2, 0), 0.1f, size * 2 },  // East wall
        { QVector3D(size, height / 2, 0),  0.1f, size * 2 }   // West wall
    };
    
    // Create wall geometries
    for (const auto& spec : wallSpecs) {
        // Create a new wall geometry
        WallGeometry wall;
        wall.vao = std::make_unique<QOpenGLVertexArrayObject>();
        wall.vbo = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
        wall.ibo = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer);
        
        // Create and bind VAO
        wall.vao->create();
        wall.vao->bind();
        
        // Create and bind VBO
        wall.vbo->create();
        wall.vbo->bind();
        
        // Create wall geometry
        const float halfWidth = spec.width / 2.0f;
        const float halfHeight = height / 2.0f;
        const float halfDepth = spec.depth / 2.0f;
        
        // Wall vertices: position (3) + normal (3) + texcoord (2)
        // We need to calculate vertex positions based on wall position and dimensions
        const float wallVertices[] = {
            // Front face
            -halfWidth, -halfHeight, -halfDepth,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
             halfWidth, -halfHeight, -halfDepth,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
             halfWidth,  halfHeight, -halfDepth,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
            -halfWidth,  halfHeight, -halfDepth,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
            
            // Back face
            -halfWidth, -halfHeight,  halfDepth,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
             halfWidth, -halfHeight,  halfDepth,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
             halfWidth,  halfHeight,  halfDepth,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
            -halfWidth,  halfHeight,  halfDepth,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
            
            // Left face
            -halfWidth, -halfHeight, -halfDepth,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
            -halfWidth, -halfHeight,  halfDepth,  -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
            -halfWidth,  halfHeight,  halfDepth,  -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
            -halfWidth,  halfHeight, -halfDepth,  -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
            
            // Right face
             halfWidth, -halfHeight, -halfDepth,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
             halfWidth, -halfHeight,  halfDepth,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
             halfWidth,  halfHeight,  halfDepth,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
             halfWidth,  halfHeight, -halfDepth,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
            
            // Bottom face
            -halfWidth, -halfHeight, -halfDepth,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
             halfWidth, -halfHeight, -halfDepth,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
             halfWidth, -halfHeight,  halfDepth,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
            -halfWidth, -halfHeight,  halfDepth,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
            
            // Top face
            -halfWidth,  halfHeight, -halfDepth,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
             halfWidth,  halfHeight, -halfDepth,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
             halfWidth,  halfHeight,  halfDepth,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
            -halfWidth,  halfHeight,  halfDepth,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f
        };
        
        // Upload vertices
        wall.vbo->allocate(wallVertices, sizeof(wallVertices));
        
        // Create and bind IBO
        wall.ibo->create();
        wall.ibo->bind();
        
        // Wall indices (2 triangles per face, 6 faces)
        const GLuint wallIndices[] = {
            0, 1, 2,    0, 2, 3,    // Front face
            4, 5, 6,    4, 6, 7,    // Back face
            8, 9, 10,   8, 10, 11,  // Left face
            12, 13, 14, 12, 14, 15, // Right face
            16, 17, 18, 16, 18, 19, // Bottom face
            20, 21, 22, 20, 22, 23  // Top face
        };
        
        // Upload indices
        wall.ibo->allocate(wallIndices, sizeof(wallIndices));
        
        // Store index count for rendering
        wall.indexCount = 36;
        
        // Set up vertex attributes
        glEnableVertexAttribArray(0); // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
        
        glEnableVertexAttribArray(1); // normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 
                             reinterpret_cast<void*>(3 * sizeof(float)));
        
        glEnableVertexAttribArray(2); // texcoord
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 
                             reinterpret_cast<void*>(6 * sizeof(float)));
        
        // Release bindings
        wall.ibo->release();
        wall.vbo->release();
        wall.vao->release();
        
        // Transform the wall to its position
        QMatrix4x4 transform;
        transform.translate(spec.position);
        
        // Add to wall list
        m_walls.push_back(std::move(wall));
    }
    
    // Create game scene entities
    if (m_gameScene) {
        // Create floor entity
        GameEntity floor;
        floor.id = "arena_floor";
        floor.type = "arena_floor";
        floor.position = QVector3D(0, 0, 0);
        floor.dimensions = QVector3D(radius * 2, 0.1f, radius * 2);
        floor.isStatic = true;
        
        m_gameScene->addEntity(floor);
        
        // Create wall entities
        for (int i = 0; i < 4; ++i) {
            GameEntity wall;
            wall.id = "arena_wall_" + QString::number(i);
            wall.type = "arena_wall";
            wall.position = wallSpecs[i].position;
            
            // Width and depth are swapped for East/West walls
            if (i < 2) {
                wall.dimensions = QVector3D(wallSpecs[i].width, height, wallSpecs[i].depth);
            } else {
                wall.dimensions = QVector3D(wallSpecs[i].depth, height, wallSpecs[i].width);
            }
            
            wall.isStatic = true;
            
            m_gameScene->addEntity(wall);
        }
    }
}