// src/arena/ui/gl_widgets/gl_arena_widget_renderer.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"

#include <QtMath>
#include <QDateTime>
#include <QDebug>
#include <QApplication>
#include <QCursor>
#include <QElapsedTimer>

// Rendering method implementations for GLArenaWidget

void GLArenaWidget::renderFloor() {
    if (!m_floorVAO.isCreated() || m_floorIndexCount <= 0) {
        return; // Don't render if geometry isn't created
    }

    // Ensure shader program is bound
    if (!m_billboardProgram->bind()) {
        qWarning() << "Failed to bind shader program in renderFloor";
        return;
    }

    // Set up shader uniforms
    m_billboardProgram->setUniformValue("model", QMatrix4x4()); // Identity model matrix
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    m_billboardProgram->setUniformValue("color", QVector4D(0.5f, 0.5f, 0.5f, 1.0f)); // Gray color

    // Bind the floor VAO
    m_floorVAO.bind();

    // Ensure the index buffer is bound before drawing
    m_floorIBO.bind();

    // Draw using glDrawElements
    glDrawElements(GL_TRIANGLES, m_floorIndexCount, GL_UNSIGNED_INT, nullptr);

    // Unbind buffers and shader
    m_floorIBO.release();
    m_floorVAO.release();
    m_billboardProgram->release();
}

void GLArenaWidget::renderGrid() {
    if (!m_gridVAO.isCreated() || m_gridVertexCount <= 0) {
        return; // Don't render if geometry isn't created
    }

    // Bind shader program
    m_billboardProgram->bind();
    
    // Set up shader uniforms
    m_billboardProgram->setUniformValue("model", QMatrix4x4());
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    m_billboardProgram->setUniformValue("color", QVector4D(0.3f, 0.3f, 0.3f, 0.5f)); // Semi-transparent dark gray
    
    // Enable blending for semi-transparent grid
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Bind VAO and draw
    m_gridVAO.bind();
    glDrawArrays(GL_LINES, 0, m_gridVertexCount);
    m_gridVAO.release();
    
    // Restore OpenGL state
    glDisable(GL_BLEND);
    
    // Unbind shader
    m_billboardProgram->release();
}

void GLArenaWidget::renderWalls() {
    if (m_walls.empty()) {
        return; // No walls to render
    }
    
    // Bind shader program
    m_billboardProgram->bind();
    
    // Set up shader uniforms
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    m_billboardProgram->setUniformValue("color", QVector4D(0.7f, 0.7f, 0.7f, 1.0f)); // Gray walls
    
    // Render each wall
    for (const auto& wall : m_walls) {
        if (wall.vao && wall.vao->isCreated() && wall.ibo && wall.ibo->isCreated() && wall.indexCount > 0) {
            // Set model matrix (identity for walls - already positioned)
            m_billboardProgram->setUniformValue("model", QMatrix4x4());
            
            // Bind VAO and IBO
            wall.vao->bind();
            wall.ibo->bind();
            
            // Draw elements
            glDrawElements(GL_TRIANGLES, wall.indexCount, GL_UNSIGNED_INT, nullptr);
            
            // Unbind
            wall.ibo->release();
            wall.vao->release();
        }
    }
    
    // Unbind shader
    m_billboardProgram->release();
}

void GLArenaWidget::renderCharacters() {
    if (m_characterSprites.isEmpty()) {
        return; // No characters to render
    }
    
    // Get current time for animation
    uint64_t now = QDateTime::currentMSecsSinceEpoch();
    
    // Bind shader program
    m_billboardProgram->bind();
    
    // Enable blending for transparent textures
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set up view and projection matrices
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    
    // Calculate camera right and up vectors for billboarding
    QVector3D cameraRight = QVector3D::crossProduct(QVector3D(0, 1, 0), 
        QVector3D(cos(m_playerController->getRotation()), 0, sin(m_playerController->getRotation())));
    QVector3D cameraUp(0, 1, 0); // World up vector
    
    if (cameraRight.length() < 0.01f) {
        cameraRight = QVector3D(1, 0, 0); // Fallback if looking straight up/down
    } else {
        cameraRight.normalize();
    }
    
    // Set camera vectors for billboarding
    m_billboardProgram->setUniformValue("cameraRight", cameraRight);
    m_billboardProgram->setUniformValue("cameraUp", cameraUp);
    
    // Render each character as a billboard
    for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
        CharacterSprite* sprite = it.value();
        
        // Skip invalid sprites
        if (!sprite || !sprite->hasValidTexture() || !sprite->hasValidVAO()) {
            continue;
        }
        
        // Get entity position from GameScene for this character
        GameEntity entity = m_gameScene->getEntity(it.key());
        QVector3D position = entity.position;
        
        // Add a slight bounce animation
        float bounceAmount = 0.05f * sin(now / 500.0f);
        position.setY(position.y() + bounceAmount);
        
        // Set billboard position and size
        m_billboardProgram->setUniformValue("billboardPos", position);
        m_billboardProgram->setUniformValue("billboardSize", QVector2D(sprite->width(), sprite->height()));
        
        // Bind texture
        glActiveTexture(GL_TEXTURE0);
        sprite->getTexture()->bind();
        m_billboardProgram->setUniformValue("textureSampler", 0);
        
        // Bind VAO and draw
        sprite->getVAO()->bind();
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        sprite->getVAO()->release();
        
        // Unbind texture
        sprite->getTexture()->release();
    }
    
    // Restore OpenGL state
    glDisable(GL_BLEND);
    
    // Unbind shader
    m_billboardProgram->release();
}

void GLArenaWidget::renderCharactersSimple() {
    if (m_characterSprites.isEmpty()) {
        return; // No characters to render
    }
    
    // Enable blending for transparent textures
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Render each character as a simple quad
    for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
        CharacterSprite* sprite = it.value();
        
        // Skip invalid sprites
        if (!sprite || !sprite->hasValidTexture()) {
            continue;
        }
        
        // Get entity position from GameScene for this character
        GameEntity entity = m_gameScene->getEntity(it.key());
        QVector3D position = entity.position;
        float width = sprite->width();
        float height = sprite->height();
        
        // Draw the character quad directly
        drawCharacterQuad(sprite->getTexture(), position.x(), position.y(), position.z(), width, height);
    }
    
    // Restore OpenGL state
    glDisable(GL_BLEND);
}

void GLArenaWidget::renderCharactersFallback() {
    if (m_characterSprites.isEmpty()) {
        return; // No characters to render
    }
    
    // Bind shader program
    if (!m_billboardProgram->bind()) {
        qWarning() << "Failed to bind shader in renderCharactersFallback";
        return;
    }
    
    // Enable blending for transparent textures
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set up projection and view matrices
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    
    // Iterate through sprites and draw them
    for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
        QString characterName = it.key();
        CharacterSprite* sprite = it.value();
        
        if (!sprite) {
            qWarning() << "Null sprite for character" << characterName;
            continue;
        }
        
        // Get entity position from GameScene for this character
        GameEntity entity = m_gameScene->getEntity(it.key());
        QVector3D worldPos = entity.position;
        QVector3D ndcPos = worldToNDC(worldPos);
        
        // Only draw if in front of camera
        if (ndcPos.z() <= 1.0f) {
            // Compute screen-space coordinates
            float x = ndcPos.x() * 0.5f + 0.5f;
            float y = ndcPos.y() * 0.5f + 0.5f;
            
            // Add debug output
            qDebug() << "Drawing character" << characterName << "at screen position" 
                     << x << y << "from world position" << worldPos.x() << worldPos.y() << worldPos.z();
            
            // Skip rest of drawing since this is just a debug fallback
        }
    }
    
    // Restore OpenGL state
    glDisable(GL_BLEND);
    
    // Unbind shader
    m_billboardProgram->release();
}

void GLArenaWidget::drawCharacterQuad(QOpenGLTexture* texture, float x, float y, float z, float width, float height) {
    if (!texture || !texture->isCreated() || !m_billboardProgram->bind()) {
        return;
    }
    
    // Calculate billboard orientation
    QVector3D right, up;
    QVector3D forward(cos(m_playerController->getRotation()), 0, sin(m_playerController->getRotation()));
    right = QVector3D::crossProduct(QVector3D(0, 1, 0), forward).normalized();
    up = QVector3D(0, 1, 0);
    
    // Calculate corners of the quad
    QVector3D positions[4] = {
        QVector3D(x, y, z) - right * (width/2) - up * (height/2),  // Bottom left
        QVector3D(x, y, z) + right * (width/2) - up * (height/2),  // Bottom right
        QVector3D(x, y, z) + right * (width/2) + up * (height/2),  // Top right
        QVector3D(x, y, z) - right * (width/2) + up * (height/2)   // Top left
    };
    
    // Setup texture coordinates
    QVector2D texCoords[4] = {
        QVector2D(0, 1),  // Bottom left
        QVector2D(1, 1),  // Bottom right
        QVector2D(1, 0),  // Top right
        QVector2D(0, 0)   // Top left
    };
    
    // Create and bind temporary VAO
    QOpenGLVertexArrayObject tempVAO;
    tempVAO.create();
    tempVAO.bind();
    
    // Create and bind temporary VBO for positions
    QOpenGLBuffer tempVBO(QOpenGLBuffer::VertexBuffer);
    tempVBO.create();
    tempVBO.bind();
    
    // Upload vertex data
    struct Vertex {
        QVector3D position;
        QVector2D texCoord;
    };
    
    Vertex vertices[4];
    for (int i = 0; i < 4; i++) {
        vertices[i].position = positions[i];
        vertices[i].texCoord = texCoords[i];
    }
    
    tempVBO.allocate(vertices, 4 * sizeof(Vertex));
    
    // Set vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                          reinterpret_cast<void*>(offsetof(Vertex, texCoord)));
    
    // Set shader uniforms
    m_billboardProgram->setUniformValue("model", QMatrix4x4());
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    
    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    texture->bind();
    m_billboardProgram->setUniformValue("textureSampler", 0);
    
    // Draw quad
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    // Cleanup
    texture->release();
    tempVBO.release();
    tempVAO.release();
    m_billboardProgram->release();
    
    // Destroy temporary objects
    tempVBO.destroy();
    tempVAO.destroy();
}

QVector3D GLArenaWidget::worldToNDC(const QVector3D& worldPos) {
    // Transform from world space to clip space
    QVector4D clipPos = m_projectionMatrix * m_viewMatrix * QVector4D(worldPos, 1.0f);
    
    // Perform perspective division to get NDC coordinates
    if (qAbs(clipPos.w()) > 0.00001f) {
        return QVector3D(clipPos.x() / clipPos.w(), clipPos.y() / clipPos.w(), clipPos.z() / clipPos.w());
    } else {
        return QVector3D(clipPos.x(), clipPos.y(), clipPos.z());
    }
}

void GLArenaWidget::createFloor(double radius) {
    // Create a square floor centered at the origin
    const float size = radius * 2.0f;
    const int segments = 16; // Number of segments per side
    const float segmentSize = size / segments;
    
    QVector<QVector3D> vertices;
    QVector<QVector3D> normals;
    QVector<QVector2D> texCoords;
    QVector<GLuint> indices;
    
    // Generate grid of vertices
    for (int z = 0; z <= segments; ++z) {
        for (int x = 0; x <= segments; ++x) {
            float xPos = x * segmentSize - size/2.0f;
            float zPos = z * segmentSize - size/2.0f;
            
            vertices.append(QVector3D(xPos, 0.0f, zPos));
            normals.append(QVector3D(0.0f, 1.0f, 0.0f)); // Up normal
            texCoords.append(QVector2D(float(x)/segments, float(z)/segments));
        }
    }
    
    // Generate indices for triangle strips
    for (int z = 0; z < segments; ++z) {
        for (int x = 0; x < segments; ++x) {
            GLuint topLeft = z * (segments+1) + x;
            GLuint topRight = topLeft + 1;
            GLuint bottomLeft = (z+1) * (segments+1) + x;
            GLuint bottomRight = bottomLeft + 1;
            
            // First triangle (top-left, bottom-left, top-right)
            indices.append(topLeft);
            indices.append(bottomLeft);
            indices.append(topRight);
            
            // Second triangle (top-right, bottom-left, bottom-right)
            indices.append(topRight);
            indices.append(bottomLeft);
            indices.append(bottomRight);
        }
    }
    
    // Store index count for rendering
    m_floorIndexCount = indices.size();
    
    // Create and bind VAO
    if (!m_floorVAO.isCreated()) {
        if (!m_floorVAO.create()) {
            qCritical() << "Failed to create floor VAO";
            return;
        }
    }
    m_floorVAO.bind();
    
    // Create, bind and fill VBO
    if (!m_floorVBO.isCreated()) {
        if (!m_floorVBO.create()) {
            qCritical() << "Failed to create floor VBO";
            m_floorVAO.release();
            return;
        }
    }
    m_floorVBO.bind();
    
    // Calculate buffer size and offsets
    int vertexCount = vertices.size();
    int vertexDataSize = vertexCount * (3 + 3 + 2) * sizeof(float); // position + normal + texcoord
    
    // Allocate buffer
    m_floorVBO.allocate(vertexDataSize);
    
    // Upload data in three parts
    m_floorVBO.write(0, vertices.constData(), vertexCount * 3 * sizeof(float));
    m_floorVBO.write(vertexCount * 3 * sizeof(float), normals.constData(), vertexCount * 3 * sizeof(float));
    m_floorVBO.write(vertexCount * 6 * sizeof(float), texCoords.constData(), vertexCount * 2 * sizeof(float));
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    
    glEnableVertexAttribArray(1); // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 
                          reinterpret_cast<void*>(vertexCount * 3 * sizeof(float)));
    
    glEnableVertexAttribArray(2); // texcoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 
                          reinterpret_cast<void*>(vertexCount * 6 * sizeof(float)));
    
    // Create, bind and fill IBO
    if (!m_floorIBO.isCreated()) {
        if (!m_floorIBO.create()) {
            qCritical() << "Failed to create floor IBO";
            m_floorVBO.release();
            m_floorVAO.release();
            return;
        }
    }
    m_floorIBO.bind();
    m_floorIBO.allocate(indices.constData(), indices.size() * sizeof(GLuint));
    
    // Unbind buffers
    m_floorIBO.release();
    m_floorVBO.release();
    m_floorVAO.release();
    
    qDebug() << "Floor created with" << vertices.size() << "vertices and" 
             << indices.size() << "indices";
}

void GLArenaWidget::createGrid(double size, int divisions) {
    // Create a grid centered at the origin
    QVector<QVector3D> vertices;
    
    float step = size / divisions;
    float halfSize = size / 2.0f;
    
    // Create grid lines
    for (int i = 0; i <= divisions; ++i) {
        float pos = -halfSize + i * step;
        
        // X-axis lines
        vertices.append(QVector3D(pos, 0.01f, -halfSize)); // Slightly above floor
        vertices.append(QVector3D(pos, 0.01f, halfSize));
        
        // Z-axis lines
        vertices.append(QVector3D(-halfSize, 0.01f, pos));
        vertices.append(QVector3D(halfSize, 0.01f, pos));
    }
    
    // Store vertex count for rendering
    m_gridVertexCount = vertices.size();
    
    // Create and bind VAO
    if (!m_gridVAO.isCreated()) {
        if (!m_gridVAO.create()) {
            qWarning() << "Failed to create grid VAO";
            return;
        }
    }
    m_gridVAO.bind();
    
    // Create, bind and fill VBO
    if (!m_gridVBO.isCreated()) {
        if (!m_gridVBO.create()) {
            qWarning() << "Failed to create grid VBO";
            m_gridVAO.release();
            return;
        }
    }
    m_gridVBO.bind();
    
    // Upload data
    m_gridVBO.allocate(vertices.constData(), vertices.size() * 3 * sizeof(float));
    
    // Set up vertex attribute
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    
    // Unbind buffers
    m_gridVBO.release();
    m_gridVAO.release();
    
    qDebug() << "Grid created with" << vertices.size() << "vertices";
}

void GLArenaWidget::createArena(double radius, double wallHeight) {
    // Store arena parameters
    m_arenaRadius = radius;
    m_wallHeight = wallHeight;
    
    // Create floor and walls using GameScene
    if (m_gameScene) {
        m_gameScene->createRectangularArena(radius, wallHeight);
    }
    
    // Create floor geometry for rendering
    createFloor(radius);
    
    // Create grid for visual reference
    createGrid(radius * 2.0, 20);
}